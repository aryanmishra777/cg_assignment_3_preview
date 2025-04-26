#ifndef MESH_H
#define MESH_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "OFFReader.h"

// Create a separate vertex structure for the mesh
struct MeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;  // Add color attribute
};

// Triangle structure for ray tracing and slicing
struct Triangle {
    MeshVertex v0, v1, v2;
    glm::vec3 centroid;
    glm::vec3 normal;
};

class Mesh {
private:
    // OpenGL objects
    GLuint VAO, VBO, EBO;
    
    // Mesh data
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Triangle> triangles;
    
    // Transform
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    
    // Matrix
    glm::mat4 modelMatrix;
    
    // Shader
    GLuint shaderProgram;
    
    // Setup methods
    void setupMesh();
    void setupShaders();
    
public:
    Mesh(OffModel* model);
    ~Mesh();
    
    // Getters
    const std::vector<MeshVertex>& getVertices() const { return vertices; }
    const std::vector<unsigned int>& getIndices() const { return indices; }
    const std::vector<Triangle>& getTriangles() const { return triangles; }
    
    // Editable vertices
    std::vector<MeshVertex>& getEditableVertices() { return vertices; }
    
    // Transform methods
    void setPosition(const glm::vec3& pos) { position = pos; updateModelMatrix(); }
    void setRotation(const glm::vec3& rot) { rotation = rot; updateModelMatrix(); }
    void setScale(const glm::vec3& s) { scale = s; updateModelMatrix(); }
    
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getRotation() const { return rotation; }
    glm::vec3 getScale() const { return scale; }
    
    // Matrix
    void updateModelMatrix();
    glm::mat4 getModelMatrix() const { return modelMatrix; }
    
    // Rendering methods
    void update();
    void render();
    
    // Vertex buffer update
    void updateVertexBuffer();
};

#endif // MESH_H