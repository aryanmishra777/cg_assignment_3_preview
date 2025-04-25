#include "polygon_fill.h"
#include <vector>
#include <glm/glm.hpp>
#include "../rasterization/line.h"
#include <GL/glew.h>
#include <algorithm>
#include <iostream>
#include <climits> // For INT_MAX, INT_MIN
#include <cmath> // For std::round

PolygonFill::PolygonFill() {
    setupBuffers();
}

PolygonFill::~PolygonFill() {
    // Clean up OpenGL buffers
    glDeleteVertexArrays(1, &polyVAO);
    glDeleteBuffers(1, &polyVBO);
}

void PolygonFill::setupBuffers() {
    // Create and set up vertex array object and buffer for filled polygon
    glGenVertexArrays(1, &polyVAO);
    glGenBuffers(1, &polyVBO);
    
    glBindVertexArray(polyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, polyVBO);
    
    // Configure vertex attributes
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PolygonFill::setPolygon(const std::vector<glm::vec2>& verts) {
    vertices = verts;
}

std::vector<Pixel> PolygonFill::fillPolygon() {
    std::vector<Pixel> filledPixels;
    
    if (vertices.size() < 3) {
        return filledPixels; // Need at least 3 points for a polygon
    }
    
    // Build edge table
    std::vector<Edge> edgeTable = buildEdgeTable(vertices);
    
    // Find min and max y for scan lines
    int minY = INT_MAX;
    int maxY = INT_MIN;
    
    for (const auto& edge : edgeTable) {
        minY = std::min(minY, edge.yMin);
        maxY = std::max(maxY, edge.yMax);
    }
    
    // Process each scan line
    std::vector<Edge> activeEdgeList;
    
    for (int y = minY; y <= maxY; y++) {
        // Remove edges that are no longer active
        activeEdgeList.erase(
            std::remove_if(activeEdgeList.begin(), activeEdgeList.end(),
                [y](const Edge& edge) { return y >= edge.yMax; }),
            activeEdgeList.end()
        );
        
        // Add new edges that become active at this scan line
        for (const auto& edge : edgeTable) {
            if (edge.yMin == y) {
                activeEdgeList.push_back(edge);
            }
        }
        
        // Sort active edges by their x coordinate
        std::sort(activeEdgeList.begin(), activeEdgeList.end(),
            [](const Edge& e1, const Edge& e2) {
                return e1.xOfYMin < e2.xOfYMin;
            });
        
        // Fill pixels between pairs of edges
        for (size_t i = 0; i < activeEdgeList.size(); i += 2) {
            if (i + 1 >= activeEdgeList.size()) break;
            
            int startX = std::round(activeEdgeList[i].xOfYMin);
            int endX = std::round(activeEdgeList[i + 1].xOfYMin);
            
            for (int x = startX; x <= endX; x++) {
                filledPixels.push_back(Pixel{x, y});
            }
        }
        
        // Update x coordinates for the next scan line
        for (auto& edge : activeEdgeList) {
            edge.xOfYMin += edge.invSlope;
        }
    }
    
    return filledPixels;
}

std::vector<Edge> PolygonFill::buildEdgeTable(const std::vector<glm::vec2>& vertices) {
    std::vector<Edge> edgeTable;
    
    for (size_t i = 0; i < vertices.size(); i++) {
        size_t j = (i + 1) % vertices.size(); // Next vertex (wraps around)
        
        int x0 = static_cast<int>(vertices[i].x);
        int y0 = static_cast<int>(vertices[i].y);
        int x1 = static_cast<int>(vertices[j].x);
        int y1 = static_cast<int>(vertices[j].y);
        
        // Skip horizontal edges
        if (y0 == y1) continue;
        
        Edge edge(y0, x0, y1, x1);
        edgeTable.push_back(edge);
    }
    
    return edgeTable;
}

void PolygonFill::renderFilledPolygon(const std::vector<Pixel>& pixels) {
    if (pixels.empty()) return;
    
    // Prepare points for rendering
    std::vector<float> points;
    for (const auto& pixel : pixels) {
        points.push_back(static_cast<float>(pixel.x));
        points.push_back(static_cast<float>(pixel.y));
    }
    
    // Upload points to GPU
    glBindVertexArray(polyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, polyVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float), points.data(), GL_STATIC_DRAW);
    
    // Draw points
    glPointSize(1.0f);
    glDrawArrays(GL_POINTS, 0, points.size() / 2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PolygonFill::clear() {
    vertices.clear();
}