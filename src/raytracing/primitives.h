#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "../mesh/mesh.h"

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    
    Ray(const glm::vec3& origin, const glm::vec3& direction)
        : origin(origin), direction(glm::normalize(direction)) {}
};

struct Material {
    glm::vec3 color;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
    float reflectivity;
    
    Material(const glm::vec3& color = glm::vec3(1.0f),
             float ambient = 0.1f, float diffuse = 0.7f, 
             float specular = 0.3f, float shininess = 32.0f,
             float reflectivity = 0.0f)
        : color(color), ambient(ambient), diffuse(diffuse), 
          specular(specular), shininess(shininess), reflectivity(reflectivity) {}
};

struct HitInfo {
    bool hit;
    float distance;
    glm::vec3 point;
    glm::vec3 normal;
    Material material;
    
    HitInfo() : hit(false), distance(std::numeric_limits<float>::max()) {}
};

class Primitive {
public:
    virtual ~Primitive() {}
    virtual HitInfo intersect(const Ray& ray) const = 0;
    virtual void setTransform(const glm::mat4& transform) = 0;
    virtual Material getMaterial() const = 0;
    virtual void setMaterial(const Material& material) = 0;
};

class Sphere : public Primitive {
public:
    Sphere(const glm::vec3& center = glm::vec3(0.0f), float radius = 1.0f);
    HitInfo intersect(const Ray& ray) const override;
    void setTransform(const glm::mat4& transform) override;
    Material getMaterial() const override { return material; }
    void setMaterial(const Material& material) override { this->material = material; }

private:
    glm::vec3 center;
    float radius;
    Material material;
};

class Box : public Primitive {
public:
    Box(const glm::vec3& min = glm::vec3(-1.0f), const glm::vec3& max = glm::vec3(1.0f));
    HitInfo intersect(const Ray& ray) const override;
    void setTransform(const glm::mat4& transform) override;
    Material getMaterial() const override { return material; }
    void setMaterial(const Material& material) override { this->material = material; }

private:
    glm::vec3 min;
    glm::vec3 max;
    glm::mat4 transform;
    glm::mat4 invTransform;
    Material material;
};

class Triangle : public Primitive {
public:
    Triangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
    HitInfo intersect(const Ray& ray) const override;
    void setTransform(const glm::mat4& transform) override;
    Material getMaterial() const override { return material; }
    void setMaterial(const Material& material) override { this->material = material; }

private:
    glm::vec3 v0, v1, v2;
    glm::vec3 normal;
    Material material;
};

class MeshObject : public Primitive {
public:
    MeshObject(const Mesh& mesh);
    HitInfo intersect(const Ray& ray) const override;
    void setTransform(const glm::mat4& transform) override;
    Material getMaterial() const override { return material; }
    void setMaterial(const Material& material) override { this->material = material; }

private:
    std::vector<std::unique_ptr<Triangle>> triangles;
    Material material;
};