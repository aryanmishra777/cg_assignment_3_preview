#include "scanline.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>

// Shader sources for displaying the framebuffer (same as in rasterizer.cpp)
const char* scanlineVertexShaderSource = R"(
    #version 430 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;
    
    out vec2 TexCoord;
    
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char* scanlineFragmentShaderSource = R"(
    #version 430 core
    in vec2 TexCoord;
    
    out vec4 FragColor;
    
    uniform sampler2D screenTexture;
    
    void main() {
        FragColor = texture(screenTexture, TexCoord);
    }
)";

ScanLineRenderer::ScanLineRenderer(int w, int h) : width(w), height(h) {
    // Initialize with default fill color
    fillColor = glm::vec3(0.0f, 1.0f, 0.0f); // Green
    
    // Initialize frame buffer
    frameBuffer.resize(width * height * 3, 0.0f);
    framebufferDirty = true;
    
    // Setup OpenGL objects
    setupFramebuffer();
    setupQuad();
    setupShaders();
    
    // Initialize edge table
    edgeTable.resize(height);
    
    // Clear the framebuffer initially
    clear();
}

ScanLineRenderer::~ScanLineRenderer() {
    // Cleanup OpenGL resources
    glDeleteTextures(1, &framebufferTexture);
    glDeleteFramebuffers(1, &framebufferFBO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteProgram(displayShader);
}

void ScanLineRenderer::setupFramebuffer() {
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

void ScanLineRenderer::setupQuad() {
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

void ScanLineRenderer::setupShaders() {
    // Create and compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &scanlineVertexShaderSource, NULL);
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
    glShaderSource(fragmentShader, 1, &scanlineFragmentShaderSource, NULL);
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

void ScanLineRenderer::resize(int w, int h) {
    // Update dimensions
    width = w;
    height = h;
    
    // Resize the buffer
    frameBuffer.resize(width * height * 3, 0.0f);
    framebufferDirty = true;
    
    // Recreate framebuffer texture with new size
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Resize edge table
    edgeTable.clear();
    edgeTable.resize(height);
}

void ScanLineRenderer::addVertex(const glm::vec2& vertex) {
    // Add a vertex to the polygon
    polygonVertices.push_back(vertex);
}

void ScanLineRenderer::clearPolygon() {
    // Clear the polygon vertices
    polygonVertices.clear();
}

void ScanLineRenderer::findYMinMax() {
    if (polygonVertices.empty()) {
        ymin = 0;
        ymax = 0;
        return;
    }
    
    ymin = height;
    ymax = 0;
    
    for (const auto& vertex : polygonVertices) {
        int y = static_cast<int>(vertex.y);
        if (y < ymin) ymin = y;
        if (y > ymax) ymax = y;
    }
    
    // Clamp to screen bounds
    ymin = std::max(0, ymin);
    ymax = std::min(height - 1, ymax);
}

void ScanLineRenderer::buildEdgeTable() {
    if (polygonVertices.size() < 3) {
        return; // Not enough vertices to form a polygon
    }
    
    // Find y bounds
    findYMinMax();
    
    // Clear and resize the edge table to have entries from ymin to ymax
    edgeTable.clear();
    edgeTable.resize(height);
    
    const int FIXED_POINT_SCALE = 1024; // Use 2^10 for fixed-point scaling
    int n = polygonVertices.size();
    
    // Process each edge of the polygon
    for (int i = 0; i < n; i++) {
        // Get the current and next vertex
        const glm::vec2& v1 = polygonVertices[i];
        const glm::vec2& v2 = polygonVertices[(i + 1) % n];
        
        // Convert to integer coordinates
        int x1 = static_cast<int>(v1.x);
        int y1 = static_cast<int>(v1.y);
        int x2 = static_cast<int>(v2.x);
        int y2 = static_cast<int>(v2.y);
        
        // Skip horizontal edges (they don't cross a scanline)
        if (y1 == y2) continue;
        
        // Ensure y1 <= y2 (top to bottom)
        if (y1 > y2) {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }
        
        // Calculate integer-based slope (dx/dy in fixed-point)
        int dx = 0;
        if (y2 != y1) { // Avoid division by zero
            dx = ((x2 - x1) * FIXED_POINT_SCALE) / (y2 - y1);
        }
        
        // Calculate the starting scanline and x-coordinate
        int y_start = y1;
        int x_start = x1 * FIXED_POINT_SCALE; // Convert to fixed-point
        
        // If y_start is below ymin, adjust x_start
        if (y_start < ymin) {
            x_start += (ymin - y_start) * dx;
            y_start = ymin;
        }
        
        // Ensure we don't exceed the framebuffer height
        int y_end = std::min(y2, height - 1);
        
        // Add the edge to the edge table if it's in range
        if (y_start <= y_end && y_end >= 0) {
            Edge edge(y_end, x_start, dx);
            edgeTable[y_start].push_back(edge);
        }
    }
}

void ScanLineRenderer::scanLineFill() {
    const int FIXED_POINT_SCALE = 1024;
    
    // 1. Find ymin, ymax - already done in buildEdgeTable()
    
    // 2. Sorted Edge Table (SET) - already built in buildEdgeTable()
    
    // 3. Initialize Active Edge Table (AET) = {}
    std::list<Edge> activeEdges;
    
    // 4. For y = ymin to ymax:
    for (int y = ymin; y <= ymax; y++) {
        // a. Add edges from SET[y] to AET
        activeEdges.splice(activeEdges.end(), edgeTable[y]);
        
        // b. Remove edges from AET where y = ymax
        activeEdges.remove_if([y](const Edge& edge) {
            return edge.ymax <= y;
        });
        
        // c. Sort AET by x
        activeEdges.sort();
        
        // d. For every pair of intersections in AET, fill pixels between the pairs
        auto it = activeEdges.begin();
        while (it != activeEdges.end()) {
            // Get the starting x coordinate (convert from fixed-point)
            int x_start = it->x / FIXED_POINT_SCALE;
            
            // Move to the next edge
            ++it;
            
            // If we run out of edges, break (odd number of intersections)
            if (it == activeEdges.end()) {
                break;
            }
            
            // Get the ending x coordinate (convert from fixed-point)
            int x_end = it->x / FIXED_POINT_SCALE;
            
            // Fill the span
            for (int x = x_start; x < x_end; x++) {
                setPixel(x, y, fillColor);
            }
            
            // Move to the next pair
            ++it;
        }
        
        // e. y is incremented in the for loop
        
        // f. Update x for non-vertical edges using integer arithmetic
        for (auto& edge : activeEdges) {
            edge.x += edge.dx; // Integer addition of fixed-point values
        }
    }
}

void ScanLineRenderer::setPixel(int x, int y, const glm::vec3& color) {
    // Check if the pixel is within bounds
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }
    
    // Calculate index in the buffer
    size_t index = ((height - 1 - y) * width + x) * 3; // Flip y to handle OpenGL coordinate system
    
    // Set the pixel color in the buffer
    frameBuffer[index] = color.r;
    frameBuffer[index + 1] = color.g;
    frameBuffer[index + 2] = color.b;
    
    // Mark buffer as dirty so it gets updated
    framebufferDirty = true;
}

void ScanLineRenderer::updateFramebuffer() {
    if (framebufferDirty) {
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, frameBuffer.data());
        glBindTexture(GL_TEXTURE_2D, 0);
        framebufferDirty = false;
    }
}

void ScanLineRenderer::clear(const glm::vec3& color) {
    // Fill the buffer with the clear color
    for (size_t i = 0; i < width * height; i++) {
        size_t index = i * 3;
        frameBuffer[index] = color.r;
        frameBuffer[index + 1] = color.g;
        frameBuffer[index + 2] = color.b;
    }
    
    framebufferDirty = true;
}

void ScanLineRenderer::update() {
    // Clear and redraw the polygon
    clear();
    
    // Draw polygon outline for visualization
    if (polygonVertices.size() >= 2) {
        for (size_t i = 0; i < polygonVertices.size(); i++) {
            size_t next = (i + 1) % polygonVertices.size();
            
            // Draw edge
            int x0 = static_cast<int>(polygonVertices[i].x);
            int y0 = static_cast<int>(polygonVertices[i].y);
            int x1 = static_cast<int>(polygonVertices[next].x);
            int y1 = static_cast<int>(polygonVertices[next].y);
            
            // Simple line drawing
            int dx = abs(x1 - x0);
            int dy = abs(y1 - y0);
            int sx = (x0 < x1) ? 1 : -1;
            int sy = (y0 < y1) ? 1 : -1;
            int err = dx - dy;
            
            while (true) {
                setPixel(x0, y0, glm::vec3(1.0f, 1.0f, 1.0f)); // White outline
                
                if (x0 == x1 && y0 == y1) break;
                
                int e2 = 2 * err;
                if (e2 > -dy) { err -= dy; x0 += sx; }
                if (e2 < dx) { err += dx; y0 += sy; }
            }
        }
    }
    
    // Fill the polygon using scan-line algorithm
    if (polygonVertices.size() >= 3) {
        buildEdgeTable();
        scanLineFill();
    }
}

void ScanLineRenderer::render() {
    // Update the framebuffer texture if dirty
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