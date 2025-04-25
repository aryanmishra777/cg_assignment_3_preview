#include "mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <GL/glew.h>

Mesh::Mesh() : VAO(0), VBO(0), EBO(0) {
}

Mesh::~Mesh() {
    clear();
}

bool Mesh::loadFromFile(const std::string& filename) {
    clear();
    
    // Check file extension
    if (filename.substr(filename.find_last_of(".") + 1) == "off") {
        return loadOFF(filename);
    }
    
    std::cerr << "Unsupported file format: " << filename << std::endl;
    return false;
}

bool Mesh::loadOFF(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::string line;
    std::getline(file, line);
    if (line != "OFF") {
        std::cerr << "Not a valid OFF file" << std::endl;
        return false;
    }

    int numVertices, numFaces, numEdges;
    file >> numVertices >> numFaces >> numEdges;

    // Read vertices
    vertices.resize(numVertices);
    for (int i = 0; i < numVertices; i++) {
        float x, y, z;
        file >> x >> y >> z;
        vertices[i].position = glm::vec3(x, y, z);
        vertices[i].texCoord = glm::vec2(0.0f); // Default tex coords
    }

    // Read faces
    faces.resize(numFaces);
    for (int i = 0; i < numFaces; i++) {
        int vertCount;
        file >> vertCount;

        faces[i].indices.resize(vertCount);
        for (int j = 0; j < vertCount; j++) {
            unsigned int index;
            file >> index;
            faces[i].indices[j] = index;
        }

        // Add face indices to the global index buffer
        // For triangulated mesh rendering
        for (int j = 1; j < vertCount - 1; j++) {
            indices.push_back(faces[i].indices[0]);
            indices.push_back(faces[i].indices[j]);
            indices.push_back(faces[i].indices[j + 1]);
        }
    }

    file.close();

    // Calculate normals and setup OpenGL buffers
    calculateNormals();
    setupBuffers();

    return true;
}

void Mesh::calculateNormals() {
    // Initialize normals to zero
    for (auto& vertex : vertices) {
        vertex.normal = glm::vec3(0.0f);
    }

    // Calculate face normals and add to vertices
    for (const auto& face : faces) {
        if (face.indices.size() >= 3) {
            // Get three vertices of the face
            const glm::vec3& v0 = vertices[face.indices[0]].position;
            const glm::vec3& v1 = vertices[face.indices[1]].position;
            const glm::vec3& v2 = vertices[face.indices[2]].position;

            // Calculate face normal
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            // Add to all vertices of the face
            for (unsigned int index : face.indices) {
                vertices[index].normal += normal;
            }
        }
    }

    // Normalize all vertex normals
    for (auto& vertex : vertices) {
        vertex.normal = glm::normalize(vertex.normal);
    }
}

void Mesh::setupBuffers() {
    // Create buffers/arrays
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
  
    glBindVertexArray(VAO);
    
    // Load vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);  

    // Load index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Set the vertex attribute pointers
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    
    // Texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

    glBindVertexArray(0);
}

void Mesh::render() const {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::clear() {
    vertices.clear();
    faces.clear();
    indices.clear();

    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
}