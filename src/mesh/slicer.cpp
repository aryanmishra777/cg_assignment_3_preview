#include "slicer.h"
#include <GL/glew.h>
#include <iostream>
#include <algorithm>

MeshSlicer::MeshSlicer() : planeVAO(0), planeVBO(0) {
    setupPlaneVisuals();
}

MeshSlicer::~MeshSlicer() {
    if (planeVAO) {
        glDeleteVertexArrays(1, &planeVAO);
    }
    if (planeVBO) {
        glDeleteBuffers(1, &planeVBO);
    }
}

void MeshSlicer::setupPlaneVisuals() {
    // Create VAO and VBO for plane visualization
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);

    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void MeshSlicer::setMesh(const Mesh& mesh) {
    inputMesh = mesh;
    resultMesh = mesh;
}

void MeshSlicer::addPlane(const Plane& plane) {
    // Limit to 4 planes as per assignment requirements
    if (planes.size() < 4) {
        planes.push_back(plane);
    } else {
        std::cerr << "Maximum 4 slicing planes are supported." << std::endl;
    }
}

void MeshSlicer::clearPlanes() {
    planes.clear();
    resultMesh = inputMesh;
}

std::vector<Plane> MeshSlicer::getPlanes() const {
    return planes;
}

Mesh MeshSlicer::sliceMesh() {
    resultMesh = inputMesh;
    
    // Apply each plane sequentially
    for (const auto& plane : planes) {
        resultMesh = clipMeshWithPlane(resultMesh, plane);
    }
    
    return resultMesh;
}

glm::vec3 MeshSlicer::intersectEdgeWithPlane(const glm::vec3& v1, const glm::vec3& v2, const Plane& plane) {
    float d1 = plane.distance(v1);
    float d2 = plane.distance(v2);
    
    // Calculate interpolation factor
    float t = d1 / (d1 - d2);
    
    // Linear interpolation between vertices
    return v1 + t * (v2 - v1);
}

Mesh MeshSlicer::clipMeshWithPlane(const Mesh& mesh, const Plane& plane) {
    Mesh result;
    std::vector<Vertex> outputVertices;
    std::vector<Face> outputFaces;
    
    // Process each face
    for (const auto& face : mesh.getFaces()) {
        if (face.indices.size() < 3) continue;
        
        std::vector<Vertex> faceVertices;
        std::vector<float> distances;
        
        // Get vertices of this face
        for (unsigned int idx : face.indices) {
            faceVertices.push_back(mesh.getVertices()[idx]);
            distances.push_back(plane.distance(mesh.getVertices()[idx].position));
        }
        
        // Check if face is entirely on positive or negative side
        bool allPositive = true;
        bool allNegative = true;
        
        for (float d : distances) {
            if (d < 0) allPositive = false;
            if (d > 0) allNegative = false;
        }
        
        // Face is entirely on positive side (keep it)
        if (allPositive) {
            outputFaces.push_back(face);
            continue;
        }
        
        // Face is entirely on negative side (discard it)
        if (allNegative) {
            continue;
        }
        
        // Face intersects the plane, need to clip
        std::vector<Vertex> newVertices;
        
        // Process each edge of the face
        size_t vertCount = faceVertices.size();
        for (size_t i = 0; i < vertCount; i++) {
            size_t j = (i + 1) % vertCount;
            
            Vertex& currentVertex = faceVertices[i];
            Vertex& nextVertex = faceVertices[j];
            
            float currentDist = distances[i];
            float nextDist = distances[j];
            
            // Current vertex is on positive side
            if (currentDist >= 0) {
                newVertices.push_back(currentVertex);
            }
            
            // Edge intersects the plane
            if ((currentDist * nextDist) < 0) {
                glm::vec3 intersectionPoint = intersectEdgeWithPlane(
                    currentVertex.position, nextVertex.position, plane);
                
                Vertex newVertex;
                newVertex.position = intersectionPoint;
                
                // Interpolate other attributes
                float t = currentDist / (currentDist - nextDist);
                newVertex.normal = glm::normalize(
                    currentVertex.normal + t * (nextVertex.normal - currentVertex.normal));
                newVertex.texCoord = currentVertex.texCoord + 
                    t * (nextVertex.texCoord - currentVertex.texCoord);
                
                newVertices.push_back(newVertex);
            }
        }
        
        // Create new face if we have enough vertices
        if (newVertices.size() >= 3) {
            Face newFace;
            
            // Add new vertices to global vertex list and create indices
            for (const auto& v : newVertices) {
                newFace.indices.push_back(outputVertices.size());
                outputVertices.push_back(v);
            }
            
            outputFaces.push_back(newFace);
        }
    }
    
    // Create new mesh from output data
    Mesh newMesh;
    // TODO: Create a method in Mesh class to build from vertices and faces
    
    return newMesh;
}

void MeshSlicer::renderSlicePlanes() const {
    // For each plane, calculate corners of a square to visualize it
    for (const auto& plane : planes) {
        glm::vec3 normal = plane.getNormal();
        
        // Calculate basis vectors for the plane
        glm::vec3 u, v;
        if (std::abs(normal.x) < std::abs(normal.y) && std::abs(normal.x) < std::abs(normal.z)) {
            u = glm::normalize(glm::cross(glm::vec3(1, 0, 0), normal));
        } else if (std::abs(normal.y) < std::abs(normal.z)) {
            u = glm::normalize(glm::cross(glm::vec3(0, 1, 0), normal));
        } else {
            u = glm::normalize(glm::cross(glm::vec3(0, 0, 1), normal));
        }
        v = glm::normalize(glm::cross(normal, u));
        
        // Calculate a point on the plane
        glm::vec3 planePoint = -plane.d * normal;
        
        // Define plane vertices with a reasonable size
        const float size = 5.0f;
        std::vector<glm::vec3> corners = {
            planePoint + size * (-u - v),
            planePoint + size * (u - v),
            planePoint + size * (u + v),
            planePoint + size * (-u + v)
        };
        
        // Update VBO with new data
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, corners.size() * sizeof(glm::vec3), 
                    corners.data(), GL_DYNAMIC_DRAW);
        
        // Draw the plane
        glBindVertexArray(planeVAO);
        glDrawArrays(GL_LINE_LOOP, 0, corners.size());
        glBindVertexArray(0);
    }
}