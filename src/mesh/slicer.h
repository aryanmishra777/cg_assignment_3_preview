#pragma once

#include "mesh.h"
#include <vector>
#include <glm/glm.hpp>

struct Plane {
    float a, b, c, d;  // ax + by + cz + d = 0

    Plane(float a = 0.0f, float b = 0.0f, float c = 1.0f, float d = 0.0f) 
        : a(a), b(b), c(c), d(d) {}

    // Get the normal of the plane
    glm::vec3 getNormal() const {
        return glm::normalize(glm::vec3(a, b, c));
    }

    // Calculate distance from a point to the plane
    float distance(const glm::vec3& point) const {
        return a * point.x + b * point.y + c * point.z + d;
    }
};

class MeshSlicer {
public:
    MeshSlicer();
    ~MeshSlicer();

    // Set the input mesh to slice
    void setMesh(const Mesh& mesh);
    
    // Add a slicing plane
    void addPlane(const Plane& plane);
    
    // Clear all slicing planes
    void clearPlanes();
    
    // Get all current planes
    std::vector<Plane> getPlanes() const;
    
    // Perform the slicing operation with all added planes
    Mesh sliceMesh();
    
    // Visualization methods
    void renderSlicePlanes() const;

private:
    // Helper methods for mesh slicing
    glm::vec3 intersectEdgeWithPlane(const glm::vec3& v1, const glm::vec3& v2, const Plane& plane);
    Mesh clipMeshWithPlane(const Mesh& mesh, const Plane& plane);
    
    std::vector<Plane> planes;
    Mesh inputMesh;
    Mesh resultMesh;
    
    // OpenGL resources for visualizing planes
    unsigned int planeVAO, planeVBO;
    void setupPlaneVisuals();
};