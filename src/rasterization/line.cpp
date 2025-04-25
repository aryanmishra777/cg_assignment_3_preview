#include "line.h"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>

LineRasterizer::LineRasterizer() : pixelVAO(0), pixelVBO(0) {
    setupBuffers();
}

LineRasterizer::~LineRasterizer() {
    if (pixelVAO) {
        glDeleteVertexArrays(1, &pixelVAO);
    }
    if (pixelVBO) {
        glDeleteBuffers(1, &pixelVBO);
    }
}

void LineRasterizer::setupBuffers() {
    glGenVertexArrays(1, &pixelVAO);
    glGenBuffers(1, &pixelVBO);
    
    glBindVertexArray(pixelVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pixelVBO);
    
    // Position attribute (x, y)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    
    // Color attribute (r, g, b)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

std::vector<Pixel> LineRasterizer::rasterizeLine(int x0, int y0, int x1, int y1) {
    // Determine which octant the line is in and call appropriate function
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    
    if (dy <= dx) {
        // Line has slope between -1 and 1 (low slope)
        if (x0 > x1) {
            // Ensure x0 <= x1
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        return rasterizeLowSlope(x0, y0, x1, y1);
    } else {
        // Line has slope < -1 or > 1 (high slope)
        if (y0 > y1) {
            // Ensure y0 <= y1
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        return rasterizeHighSlope(x0, y0, x1, y1);
    }
}

std::vector<Pixel> LineRasterizer::rasterizeLowSlope(int x0, int y0, int x1, int y1) {
    std::vector<Pixel> pixels;
    
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yi = 1;
    
    // Handle negative slope
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }
    
    int error = 2 * dy - dx;
    int y = y0;
    
    // Bresenham's line algorithm
    for (int x = x0; x <= x1; x++) {
        pixels.push_back(Pixel(x, y));
        
        if (error > 0) {
            y += yi;
            error -= 2 * dx;
        }
        
        error += 2 * dy;
    }
    
    return pixels;
}

std::vector<Pixel> LineRasterizer::rasterizeHighSlope(int x0, int y0, int x1, int y1) {
    std::vector<Pixel> pixels;
    
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;
    
    // Handle leftward direction
    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }
    
    int error = 2 * dx - dy;
    int x = x0;
    
    // Bresenham's line algorithm (swapped x/y roles)
    for (int y = y0; y <= y1; y++) {
        pixels.push_back(Pixel(x, y));
        
        if (error > 0) {
            x += xi;
            error -= 2 * dy;
        }
        
        error += 2 * dx;
    }
    
    return pixels;
}

void LineRasterizer::renderPixels(const std::vector<Pixel>& pixels) {
    if (pixels.empty()) return;
    
    // Prepare vertex data for rendering: [x, y, r, g, b] for each pixel
    std::vector<float> vertexData;
    vertexData.reserve(pixels.size() * 5);
    
    for (const auto& pixel : pixels) {
        vertexData.push_back(pixel.x);
        vertexData.push_back(pixel.y);
        vertexData.push_back(pixel.r);
        vertexData.push_back(pixel.g);
        vertexData.push_back(pixel.b);
    }
    
    // Update VBO
    glBindBuffer(GL_ARRAY_BUFFER, pixelVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), 
                vertexData.data(), GL_DYNAMIC_DRAW);
    
    // Draw points
    glBindVertexArray(pixelVAO);
    glPointSize(1.0f);
    glDrawArrays(GL_POINTS, 0, pixels.size());
    glBindVertexArray(0);
}

void LineRasterizer::clear() {
    // No persistent data to clear
}