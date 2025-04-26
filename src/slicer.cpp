#include "slicer.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

// Shader paths
const char* sliceVertexShaderPath = "shaders/slice.vert";
const char* sliceFragmentShaderPath = "shaders/slice.frag";

// Define colors for different mesh regions
const glm::vec3 REGION_COLORS[] = {
    glm::vec3(0.9f, 0.2f, 0.2f), // Red region
    glm::vec3(0.2f, 0.7f, 0.2f), // Green region
    glm::vec3(0.2f, 0.3f, 0.9f), // Blue region
    glm::vec3(0.9f, 0.9f, 0.2f), // Yellow region
    glm::vec3(0.9f, 0.4f, 0.9f), // Pink region
    glm::vec3(0.4f, 0.9f, 0.9f)  // Cyan region
};

// Utility function to read shader source (same as in mesh.cpp)
std::string readSliceShaderFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filePath << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

MeshSlicer::MeshSlicer(Mesh* m) : mesh(m), showSlice(true), activeSlicePlane(0) {
    // Add a default horizontal plane
    planes.push_back(Plane(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f));
    
    // Setup visualization
    setupSliceVisualization();
    
    // Compute initial slice
    computeSlice();
    updateMeshColors();
}

MeshSlicer::~MeshSlicer() {
    // Cleanup OpenGL resources
    glDeleteVertexArrays(1, &sliceVAO);
    glDeleteBuffers(1, &sliceVBO);
    glDeleteProgram(sliceShaderProgram);
}

void MeshSlicer::setupSliceVisualization() {
    // Create buffers for slice visualization
    glGenVertexArrays(1, &sliceVAO);
    glGenBuffers(1, &sliceVBO);
    
    // Create shaders for slice visualization
    std::string vertexShaderSource = readSliceShaderFile(sliceVertexShaderPath);
    std::string fragmentShaderSource = readSliceShaderFile(sliceFragmentShaderPath);
    
    const char* vShaderCode = vertexShaderSource.c_str();
    const char* fShaderCode = fragmentShaderSource.c_str();
    
    // Compile shaders
    GLuint vertex, fragment;
    int success;
    char infoLog[512];
    
    // Vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Shader program
    sliceShaderProgram = glCreateProgram();
    glAttachShader(sliceShaderProgram, vertex);
    glAttachShader(sliceShaderProgram, fragment);
    glLinkProgram(sliceShaderProgram);
    
    glGetProgramiv(sliceShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(sliceShaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    // Delete the shaders after linking
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void MeshSlicer::addPlane(const Plane& plane) {
    if (planes.size() < 4) {
        planes.push_back(plane);
        computeSlice();
        updateMeshColors();
    }
}

void MeshSlicer::removePlane(int index) {
    if (index >= 0 && index < planes.size()) {
        planes.erase(planes.begin() + index);
        if (activeSlicePlane >= planes.size()) {
            activeSlicePlane = planes.size() - 1;
        }
        computeSlice();
        updateMeshColors();
    }
}

void MeshSlicer::updatePlane(int index, const Plane& plane) {
    if (index >= 0 && index < planes.size()) {
        planes[index] = plane;
        computeSlice();
        updateMeshColors();
    }
}

void MeshSlicer::clearPlanes() {
    planes.clear();
    activeSlicePlane = 0;
    sliceVertices.clear();
}

void MeshSlicer::computeSlice() {
    // Clear existing slice
    sliceVertices.clear();
    
    // Apply all planes one by one
    for (const auto& plane : planes) {
        sliceWithPlane(plane);
    }
    
    // Upload slice vertices to GPU
    glBindVertexArray(sliceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sliceVBO);
    glBufferData(GL_ARRAY_BUFFER, sliceVertices.size() * sizeof(glm::vec3), 
                 sliceVertices.data(), GL_STATIC_DRAW);
    
    // Set vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    
    glBindVertexArray(0);
}

void MeshSlicer::sliceWithPlane(const Plane& plane) {
    // Get triangles from mesh
    const std::vector<Triangle>& triangles = mesh->getTriangles();
    
    // Slice each triangle with the plane
    for (const auto& triangle : triangles) {
        // Compute signed distances from vertices to plane
        float d0 = plane.signedDistance(triangle.v0.position);
        float d1 = plane.signedDistance(triangle.v1.position);
        float d2 = plane.signedDistance(triangle.v2.position);
        
        // Check if triangle intersects with plane
        if ((d0 * d1 <= 0.0f) || (d0 * d2 <= 0.0f) || (d1 * d2 <= 0.0f)) {
            // Find intersections
            std::vector<glm::vec3> intersections;
            
            if (d0 * d1 <= 0.0f && d0 != 0.0f && d1 != 0.0f) {
                glm::vec3 intersection;
                findIntersection(triangle.v0.position, triangle.v1.position, d0, d1, intersection);
                intersections.push_back(intersection);
            }
            
            if (d0 * d2 <= 0.0f && d0 != 0.0f && d2 != 0.0f) {
                glm::vec3 intersection;
                findIntersection(triangle.v0.position, triangle.v2.position, d0, d2, intersection);
                intersections.push_back(intersection);
            }
            
            if (d1 * d2 <= 0.0f && d1 != 0.0f && d2 != 0.0f) {
                glm::vec3 intersection;
                findIntersection(triangle.v1.position, triangle.v2.position, d1, d2, intersection);
                intersections.push_back(intersection);
            }
            
            // Handle vertices exactly on the plane
            if (d0 == 0.0f) {
                intersections.push_back(triangle.v0.position);
            }
            if (d1 == 0.0f) {
                intersections.push_back(triangle.v1.position);
            }
            if (d2 == 0.0f) {
                intersections.push_back(triangle.v2.position);
            }
            
            // If we have 2 intersections, add a line segment to the slice
            if (intersections.size() >= 2) {
                sliceVertices.push_back(intersections[0]);
                sliceVertices.push_back(intersections[1]);
            }
        }
    }
}

void MeshSlicer::findIntersection(const glm::vec3& v0, const glm::vec3& v1, 
                                  float d0, float d1, glm::vec3& intersection) {
    // Compute parametric value t where the line intersects the plane
    float t = d0 / (d0 - d1);
    
    // Interpolate position
    intersection = v0 + t * (v1 - v0);
}

void MeshSlicer::updateMeshColors() {
    if (planes.empty()) {
        // Set all vertices to default color
        std::vector<MeshVertex>& meshVertices = mesh->getEditableVertices();
        for (auto& vertex : meshVertices) {
            vertex.color = glm::vec3(0.8f, 0.8f, 0.8f); // Default light gray
        }
        return;
    }
    
    // Get a reference to the mesh vertices for editing
    std::vector<MeshVertex>& meshVertices = mesh->getEditableVertices();
    
    // Initialize all vertices with region code 0
    std::vector<int> vertexRegions(meshVertices.size(), 0);
    
    // For each plane, update region codes
    for (size_t planeIdx = 0; planeIdx < planes.size(); planeIdx++) {
        const Plane& plane = planes[planeIdx];
        
        // For each vertex, determine which side of the plane it's on
        for (size_t i = 0; i < meshVertices.size(); i++) {
            float signedDist = plane.signedDistance(meshVertices[i].position);
            
            // If vertex is on the positive side, add a bit to its region code
            if (signedDist > 0.0f) {
                vertexRegions[i] |= (1 << planeIdx);
            }
        }
    }
    
    // Map region codes to colors (number of regions is 2^numPlanes)
    for (size_t i = 0; i < meshVertices.size(); i++) {
        int regionCode = vertexRegions[i];
        meshVertices[i].color = REGION_COLORS[regionCode % 6]; // Cycle through available colors
    }
    
    // Update mesh VBO to reflect color changes
    mesh->updateVertexBuffer();
}

void MeshSlicer::update() {
    // Update could be used for animations or dynamic slicing
}

void MeshSlicer::render() {
    // First render the mesh
    mesh->render();
    
    // Then render the slice if enabled
    if (showSlice && !sliceVertices.empty()) {
        glUseProgram(sliceShaderProgram);
        
        // Set uniforms (view, projection, etc.)
        glm::mat4 model = mesh->getModelMatrix();
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                               (float)1280/(float)720, 0.1f, 100.0f);
        
        // Setup view matrix (same as in mesh rendering)
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));
        
        // Set matrices
        glUniformMatrix4fv(glGetUniformLocation(sliceShaderProgram, "model"), 1, GL_FALSE, 
                          glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(sliceShaderProgram, "view"), 1, GL_FALSE, 
                          glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(sliceShaderProgram, "projection"), 1, GL_FALSE, 
                          glm::value_ptr(projection));
        
        // Set slice color
        glm::vec3 sliceColor(1.0f, 0.0f, 0.0f); // Red slice
        glUniform3fv(glGetUniformLocation(sliceShaderProgram, "sliceColor"), 1, 
                    glm::value_ptr(sliceColor));
        
        // Draw slice lines
        glBindVertexArray(sliceVAO);
        glLineWidth(2.0f); // Thicker lines for better visibility
        glDrawArrays(GL_LINES, 0, sliceVertices.size());
        glLineWidth(1.0f); // Reset line width
        glBindVertexArray(0);
        
        // Reset OpenGL state
        glUseProgram(0);
    }
}