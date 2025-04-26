#include "rasterizer.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>

// Shader sources for displaying the framebuffer
const char* displayVertexShaderSource = R"(
    #version 430 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;
    
    out vec2 TexCoord;
    
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char* displayFragmentShaderSource = R"(
    #version 430 core
    in vec2 TexCoord;
    
    out vec4 FragColor;
    
    uniform sampler2D screenTexture;
    
    void main() {
        FragColor = texture(screenTexture, TexCoord);
    }
)";

Rasterizer::Rasterizer(int w, int h) : width(w), height(h) {
    // Initialize start and end points for line drawing
    startPoint = glm::vec2(width * 0.25f, height * 0.5f);
    endPoint = glm::vec2(width * 0.75f, height * 0.5f);
    lineColor = glm::vec3(1.0f, 0.0f, 0.0f); // Bright red for better visibility
    
    // Initialize framebuffer
    frameBuffer.resize(width * height * 3, 0.0f);
    framebufferDirty = true; // Force initial update
    
    // Setup OpenGL objects
    setupFramebuffer();
    setupQuad();
    setupShaders();
    
    // Clear the framebuffer initially
    clear();
    
    // Draw initial line
    drawLine(startPoint, endPoint, lineColor);
}

Rasterizer::~Rasterizer() {
    // Cleanup OpenGL resources
    glDeleteTextures(1, &framebufferTexture);
    glDeleteFramebuffers(1, &framebufferFBO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteProgram(displayShader);
}

void Rasterizer::setupFramebuffer() {
    // Create a texture for the framebuffer
    glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Create a framebuffer object
    glGenFramebuffers(1, &framebufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);
    
    // Check framebuffer status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Rasterizer::setupQuad() {
    // Create a quad for displaying the framebuffer
    float quadVertices[] = {
        // Positions     // Texture Coords
        -1.0f,  1.0f,    0.0f, 1.0f,
        -1.0f, -1.0f,    0.0f, 0.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
        
        -1.0f,  1.0f,    0.0f, 1.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
         1.0f,  1.0f,    1.0f, 1.0f
    };
    
    // Create VAO and VBO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(quadVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void Rasterizer::setupShaders() {
    // Create and compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &displayVertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for vertex shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }
    
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &displayFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Check for fragment shader compilation errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }
    
    // Create shader program
    displayShader = glCreateProgram();
    glAttachShader(displayShader, vertexShader);
    glAttachShader(displayShader, fragmentShader);
    glLinkProgram(displayShader);
    
    // Check for linking errors
    glGetProgramiv(displayShader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(displayShader, 512, NULL, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }
    
    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Rasterizer::resize(int w, int h) {
    // Update dimensions
    width = w;
    height = h;
    
    // Resize the frame buffer
    frameBuffer.clear();
    frameBuffer.resize(width * height * 3, 0.0f);
    
    // Recreate framebuffer texture with new size
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Adjust start and end points if they're outside the new dimensions
    startPoint.x = std::min(startPoint.x, (float)width);
    startPoint.y = std::min(startPoint.y, (float)height);
    endPoint.x = std::min(endPoint.x, (float)width);
    endPoint.y = std::min(endPoint.y, (float)height);
    
    // Re-draw the line
    update();
}

void Rasterizer::drawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec3& color) {
    // Store line parameters
    startPoint = start;
    endPoint = end;
    lineColor = color;
    
    // Rasterize the line using Bresenham's algorithm
    bresenhamLine(static_cast<int>(start.x), static_cast<int>(start.y),
                  static_cast<int>(end.x), static_cast<int>(end.y));
}

void Rasterizer::clear(const glm::vec3& color) {
    // Bind framebuffer for clearing
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferFBO);
    glClearColor(color.r, color.g, color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Rasterizer::setPixel(int x, int y, const glm::vec3& color) {
    // Check if the pixel is within bounds
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }
    
    // Use a simple array to store framebuffer data
    int index = (y * width + x) * 3; // RGB data
    frameBuffer[index] = color.r;
    frameBuffer[index + 1] = color.g;
    frameBuffer[index + 2] = color.b;
    
    // Mark that the framebuffer needs updating
    framebufferDirty = true;
}

void Rasterizer::basicLineRasterization(int x0, int y0, int x1, int y1) {
    // Basic line rasterization for reference
    // This uses a simple DDA (Digital Differential Analyzer) algorithm
    
    int dx = x1 - x0;
    int dy = y1 - y0;
    
    // Calculate steps needed for interpolation
    int steps = std::max(std::abs(dx), std::abs(dy));
    
    // Calculate increments
    float x_inc = (float)dx / steps;
    float y_inc = (float)dy / steps;
    
    // Initial point
    float x = x0;
    float y = y0;
    
    // Draw the line pixel by pixel
    for (int i = 0; i <= steps; i++) {
        setPixel(round(x), round(y), lineColor);
        x += x_inc;
        y += y_inc;
    }
}

void Rasterizer::bresenhamLine(int x0, int y0, int x1, int y1) {
    // Bresenham's line algorithm - optimized for all quadrants
    
    // Calculate deltas
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    
    // Determine step direction
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    
    // Initial error
    int err = dx - dy;
    int err2;
    
    while (true) {
        // Set the current pixel
        setPixel(x0, y0, lineColor);
        
        // Make line MUCH thicker by setting more adjacent pixels
        // Increase thickness from 5x5 to 8x8 for better visibility
        for (int i = -4; i <= 4; i++) {
            for (int j = -4; j <= 4; j++) {
                setPixel(x0 + i, y0 + j, lineColor);
            }
        }
        
        // Check if we've reached the endpoint
        if (x0 == x1 && y0 == y1) break;
        
        // Calculate new error
        err2 = 2 * err;
        
        // Update coordinates and error
        if (err2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        
        if (err2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Rasterizer::updateFramebuffer() {
    if (framebufferDirty) {
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, frameBuffer.data());
        glBindTexture(GL_TEXTURE_2D, 0);
        framebufferDirty = false;
    }
}

void Rasterizer::update() {
    // Redraw the line with current parameters
    clear(); // Clear the framebuffer
    drawLine(startPoint, endPoint, lineColor);
}

void Rasterizer::render() {
    // Update texture if needed
    updateFramebuffer();
    
    // Bind back to the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Clear default framebuffer
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Use the display shader
    glUseProgram(displayShader);
    
    // Bind the framebuffer texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glUniform1i(glGetUniformLocation(displayShader, "screenTexture"), 0);
    
    // Render the quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Reset OpenGL state
    glUseProgram(0);
}