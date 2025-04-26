#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <cmath>
#include <limits>

namespace MathUtils {
    // Floating point comparison (with epsilon)
    inline bool nearEqual(float a, float b, float epsilon = 1e-5f) {
        return std::abs(a - b) < epsilon;
    }
    
    // Vector comparison (with epsilon)
    inline bool nearEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = 1e-5f) {
        return nearEqual(a.x, b.x, epsilon) && 
               nearEqual(a.y, b.y, epsilon) && 
               nearEqual(a.z, b.z, epsilon);
    }
    
    // Plane equation utilities
    inline float signedDistanceToPlane(const glm::vec3& point, const glm::vec3& planeNormal, float planeDistance) {
        return glm::dot(point, planeNormal) - planeDistance;
    }
    
    // Ray-plane intersection
    inline bool rayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                    const glm::vec3& planeNormal, float planeDistance,
                                    float& t, glm::vec3& intersection) {
        float denom = glm::dot(rayDirection, planeNormal);
        
        // Check if ray is parallel to plane
        if (std::abs(denom) < 1e-5f) {
            return false;
        }
        
        t = (planeDistance - glm::dot(rayOrigin, planeNormal)) / denom;
        
        // Check if intersection is behind the ray
        if (t < 0) {
            return false;
        }
        
        // Calculate intersection point
        intersection = rayOrigin + t * rayDirection;
        return true;
    }
    
    // Ray-triangle intersection (Möller–Trumbore algorithm)
    inline bool rayTriangleIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                                      const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                                      float& t, glm::vec3& intersection) {
        // Calculate edges
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        
        // Calculate determinant
        glm::vec3 h = glm::cross(rayDirection, edge2);
        float a = glm::dot(edge1, h);
        
        // If determinant is near zero, ray lies in plane of triangle or ray is parallel to plane
        if (a > -1e-5f && a < 1e-5f) {
            return false;
        }
        
        float f = 1.0f / a;
        glm::vec3 s = rayOrigin - v0;
        float u = f * glm::dot(s, h);
        
        // Check if intersection is outside triangle
        if (u < 0.0f || u > 1.0f) {
            return false;
        }
        
        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(rayDirection, q);
        
        // Check if intersection is outside triangle
        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }
        
        // Calculate t
        t = f * glm::dot(edge2, q);
        
        // Check if intersection is behind the ray
        if (t < 0.0f) {
            return false;
        }
        
        // Calculate intersection point
        intersection = rayOrigin + t * rayDirection;
        return true;
    }
    
    // Barycentric coordinates calculation
    inline glm::vec3 calculateBarycentricCoordinates(const glm::vec2& p, 
                                                   const glm::vec2& a, 
                                                   const glm::vec2& b, 
                                                   const glm::vec2& c) {
        glm::vec2 v0 = b - a;
        glm::vec2 v1 = c - a;
        glm::vec2 v2 = p - a;
        
        float d00 = glm::dot(v0, v0);
        float d01 = glm::dot(v0, v1);
        float d11 = glm::dot(v1, v1);
        float d20 = glm::dot(v2, v0);
        float d21 = glm::dot(v2, v1);
        
        float denom = d00 * d11 - d01 * d01;
        
        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;
        
        return glm::vec3(u, v, w);
    }
    
    // Check if point is inside triangle using barycentric coordinates
    inline bool isPointInTriangle(const glm::vec2& p, 
                                const glm::vec2& a, 
                                const glm::vec2& b, 
                                const glm::vec2& c) {
        glm::vec3 barycentric = calculateBarycentricCoordinates(p, a, b, c);
        return barycentric.x >= 0 && barycentric.y >= 0 && barycentric.z >= 0;
    }
    
    // Interpolate values using barycentric coordinates
    template<typename T>
    inline T interpolateWithBarycentric(const T& v0, const T& v1, const T& v2, const glm::vec3& barycentric) {
        return v0 * barycentric.x + v1 * barycentric.y + v2 * barycentric.z;
    }
}

#endif // MATH_UTILS_H