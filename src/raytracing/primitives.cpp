#include "primitives.h"
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <algorithm>

// Sphere implementation
Sphere::Sphere(const glm::vec3& center, float radius) 
    : center(center), radius(radius) {
}

HitInfo Sphere::intersect(const Ray& ray) const {
    HitInfo result;
    
    glm::vec3 oc = ray.origin - center;
    float a = glm::dot(ray.direction, ray.direction);
    float b = 2.0f * glm::dot(oc, ray.direction);
    float c = glm::dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) {
        return result; // No intersection
    }
    
    float sqrtd = std::sqrt(discriminant);
    float t1 = (-b - sqrtd) / (2.0f * a);
    float t2 = (-b + sqrtd) / (2.0f * a);
    
    float t = t1;
    if (t < 0.001f) {
        t = t2; // Check if t1 is behind the ray
        if (t < 0.001f) {
            return result; // Both intersections are behind the ray
        }
    }
    
    result.hit = true;
    result.distance = t;
    result.point = ray.origin + t * ray.direction;
    result.normal = glm::normalize(result.point - center);
    result.material = material;
    
    return result;
}

void Sphere::setTransform(const glm::mat4& transform) {
    glm::vec4 transformedCenter = transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    center = glm::vec3(transformedCenter);
    
    // For simplicity, assume uniform scaling
    glm::vec4 scale = transform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    radius *= glm::length(glm::vec3(scale));
}

// Box implementation
Box::Box(const glm::vec3& min, const glm::vec3& max) 
    : min(min), max(max), transform(glm::mat4(1.0f)), invTransform(glm::mat4(1.0f)) {
}

HitInfo Box::intersect(const Ray& ray) const {
    HitInfo result;
    
    // Transform ray to object space
    glm::vec3 rayOrigin = glm::vec3(invTransform * glm::vec4(ray.origin, 1.0f));
    glm::vec3 rayDirection = glm::normalize(glm::vec3(invTransform * glm::vec4(ray.direction, 0.0f)));
    
    Ray localRay(rayOrigin, rayDirection);
    
    float tMin = -std::numeric_limits<float>::infinity();
    float tMax = std::numeric_limits<float>::infinity();
    
    // Check intersection with each slab
    for (int i = 0; i < 3; i++) {
        float invD = 1.0f / localRay.direction[i];
        float t0 = (min[i] - localRay.origin[i]) * invD;
        float t1 = (max[i] - localRay.origin[i]) * invD;
        
        if (invD < 0.0f) {
            std::swap(t0, t1);
        }
        
        tMin = t0 > tMin ? t0 : tMin;
        tMax = t1 < tMax ? t1 : tMax;
        
        if (tMax < tMin) {
            return result; // No intersection
        }
    }
    
    // Ensure we're not inside the box or hitting it from behind
    if (tMin < 0.001f) {
        if (tMax < 0.001f) {
            return result; // Box is behind ray
        }
        tMin = tMax; // We're inside the box
    }
    
    result.hit = true;
    result.distance = tMin;
    result.point = ray.origin + tMin * ray.direction;
    
    // Calculate the normal in object space
    glm::vec3 localPoint = localRay.origin + tMin * localRay.direction;
    glm::vec3 localNormal(0.0f);
    
    // Determine which face was hit
    float epsilon = 0.001f;
    if (std::abs(localPoint.x - min.x) < epsilon) localNormal = glm::vec3(-1, 0, 0);
    else if (std::abs(localPoint.x - max.x) < epsilon) localNormal = glm::vec3(1, 0, 0);
    else if (std::abs(localPoint.y - min.y) < epsilon) localNormal = glm::vec3(0, -1, 0);
    else if (std::abs(localPoint.y - max.y) < epsilon) localNormal = glm::vec3(0, 1, 0);
    else if (std::abs(localPoint.z - min.z) < epsilon) localNormal = glm::vec3(0, 0, -1);
    else if (std::abs(localPoint.z - max.z) < epsilon) localNormal = glm::vec3(0, 0, 1);
    
    // Transform normal to world space
    glm::mat4 normalMatrix = glm::transpose(invTransform);
    result.normal = glm::normalize(glm::vec3(normalMatrix * glm::vec4(localNormal, 0.0f)));
    result.material = material;
    
    return result;
}

void Box::setTransform(const glm::mat4& transform) {
    this->transform = transform;
    this->invTransform = glm::inverse(transform);
}

// Triangle implementation
Triangle::Triangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
    : v0(v0), v1(v1), v2(v2) {
    // Calculate face normal
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    normal = glm::normalize(glm::cross(edge1, edge2));
}

HitInfo Triangle::intersect(const Ray& ray) const {
    HitInfo result;
    
    // Möller–Trumbore algorithm
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(ray.direction, edge2);
    float a = glm::dot(edge1, h);
    
    // Ray is parallel to triangle
    if (a > -0.0001f && a < 0.0001f) {
        return result;
    }
    
    float f = 1.0f / a;
    glm::vec3 s = ray.origin - v0;
    float u = f * glm::dot(s, h);
    
    // Hit is outside triangle
    if (u < 0.0f || u > 1.0f) {
        return result;
    }
    
    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(ray.direction, q);
    
    // Hit is outside triangle
    if (v < 0.0f || u + v > 1.0f) {
        return result;
    }
    
    float t = f * glm::dot(edge2, q);
    
    // Hit is behind ray origin
    if (t < 0.001f) {
        return result;
    }
    
    result.hit = true;
    result.distance = t;
    result.point = ray.origin + t * ray.direction;
    result.normal = normal;
    result.material = material;
    
    return result;
}

void Triangle::setTransform(const glm::mat4& transform) {
    v0 = glm::vec3(transform * glm::vec4(v0, 1.0f));
    v1 = glm::vec3(transform * glm::vec4(v1, 1.0f));
    v2 = glm::vec3(transform * glm::vec4(v2, 1.0f));
    
    // Recalculate normal
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    normal = glm::normalize(glm::cross(edge1, edge2));
}

// MeshObject implementation
MeshObject::MeshObject(const Mesh& mesh) {
    const std::vector<Vertex>& vertices = mesh.getVertices();
    const std::vector<Face>& faces = mesh.getFaces();
    
    // Create triangles from faces
    for (const auto& face : faces) {
        if (face.indices.size() >= 3) {
            // Triangulate face
            for (size_t i = 1; i < face.indices.size() - 1; i++) {
                glm::vec3 v0 = vertices[face.indices[0]].position;
                glm::vec3 v1 = vertices[face.indices[i]].position;
                glm::vec3 v2 = vertices[face.indices[i + 1]].position;
                
                triangles.push_back(std::make_unique<Triangle>(v0, v1, v2));
                triangles.back()->setMaterial(material);
            }
        }
    }
}

HitInfo MeshObject::intersect(const Ray& ray) const {
    HitInfo result;
    
    // Find closest intersection with any triangle
    for (const auto& triangle : triangles) {
        HitInfo hitInfo = triangle->intersect(ray);
        
        if (hitInfo.hit && hitInfo.distance < result.distance) {
            result = hitInfo;
        }
    }
    
    return result;
}

void MeshObject::setTransform(const glm::mat4& transform) {
    // Apply transform to all triangles
    for (auto& triangle : triangles) {
        triangle->setTransform(transform);
    }
}