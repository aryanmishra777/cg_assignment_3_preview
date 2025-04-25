#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Face {
    std::vector<unsigned int> indices;
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    bool loadFromFile(const std::string& filename);
    void render() const;
    void clear();

    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<Face>& getFaces() const { return faces; }
    const std::vector<unsigned int>& getIndices() const { return indices; }

private:
    bool loadOFF(const std::string& filename);
    void calculateNormals();
    void setupBuffers();

    std::vector<Vertex> vertices;
    std::vector<Face> faces;
    std::vector<unsigned int> indices;

    // OpenGL buffers
    unsigned int VAO, VBO, EBO;
};