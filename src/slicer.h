#ifndef SLICER_H
#define SLICER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "mesh.h"

struct Plane {
    glm::vec3 normal;
    float distance;
    
    Plane(const glm::vec3& n = glm::vec3(0.0f, 1.0f, 0.0f), float d = 0.0f)
        : normal(glm::normalize(n)), distance(d) {}
        
    float signedDistance(const glm::vec3& point) const {
        return glm::dot(normal, point) - distance;
    }
};

class MeshSlicer {
private:
    // Reference to the mesh being sliced
    Mesh* mesh;
    
    // Slicing planes
    std::vector<Plane> planes;
    
    // Slice visualization
    GLuint sliceVAO, sliceVBO;
    std::vector<glm::vec3> sliceVertices;
    GLuint sliceShaderProgram;
    
    // UI state
    bool showSlice;
    int activeSlicePlane;
    
    // Methods
    void setupSliceVisualization();
    void computeSlice();
    void sliceWithPlane(const Plane& plane);
    void findIntersection(const glm::vec3& v0, const glm::vec3& v1, 
                          float d0, float d1, glm::vec3& intersection);
    
public:
    MeshSlicer(Mesh* m);
    ~MeshSlicer();
    
    // Plane management
    void addPlane(const Plane& plane);
    void removePlane(int index);
    void updatePlane(int index, const Plane& plane);
    void clearPlanes();
    
    int getPlaneCount() const { return planes.size(); }
    Plane getPlane(int index) const { return planes[index]; }
    
    // UI state
    void setShowSlice(bool show) { showSlice = show; }
    bool isShowingSlice() const { return showSlice; }
    void setActivePlane(int index) { activeSlicePlane = index; }
    int getActivePlane() const { return activeSlicePlane; }
    
    // Update and render
    void update();
    void render();
    
    // Mesh color update
    void updateMeshColors();
};

#endif // SLICER_H