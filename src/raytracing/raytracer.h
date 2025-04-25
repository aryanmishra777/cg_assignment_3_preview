#pragma once

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "primitives.h"

struct Light {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    
    Light(const glm::vec3& position, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f)
        : position(position), color(color), intensity(intensity) {}
};

struct Camera {
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    float fov;
    float aspectRatio;
    
    Camera(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 5.0f),
           const glm::vec3& target = glm::vec3(0.0f),
           const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f),
           float fov = 60.0f,
           float aspectRatio = 16.0f/9.0f)
        : position(position), target(target), up(up), fov(fov), aspectRatio(aspectRatio) {}
};

class Scene {
public:
    Scene();
    ~Scene();
    
    void addPrimitive(std::unique_ptr<Primitive> primitive);
    void addLight(const Light& light);
    void setCamera(const Camera& camera);
    void setBackgroundColor(const glm::vec3& color);
    
    const std::vector<std::unique_ptr<Primitive>>& getPrimitives() const { return primitives; }
    const std::vector<Light>& getLights() const { return lights; }
    const Camera& getCamera() const { return camera; }
    glm::vec3 getBackgroundColor() const { return backgroundColor; }
    
    // Find the closest intersection in the scene
    HitInfo trace(const Ray& ray) const;

private:
    std::vector<std::unique_ptr<Primitive>> primitives;
    std::vector<Light> lights;
    Camera camera;
    glm::vec3 backgroundColor;
};

class RayTracer {
public:
    RayTracer();
    ~RayTracer();
    
    // Set up and manage the scene
    Scene& getScene() { return scene; }
    
    // Set the output image dimensions
    void setDimensions(int width, int height);
    
    // Enable/disable features
    void enableReflections(bool enable);
    void enableShadows(bool enable);
    
    // Render the scene
    void render();
    
    // Get the rendered image
    const std::vector<unsigned char>& getImage() const { return image; }
    
    // Display the rendered image
    void displayImage() const;

private:
    // Helper methods
    glm::vec3 traceRay(const Ray& ray, int depth) const;
    glm::vec3 calculateLighting(const HitInfo& hit, const Ray& ray) const;
    bool isInShadow(const glm::vec3& point, const glm::vec3& lightDir, float lightDistance) const;
    
    // Shader helper methods
    unsigned int createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);
    std::string loadShaderSource(const std::string& path);
    
    Scene scene;
    int width, height;
    bool reflectionsEnabled;
    bool shadowsEnabled;
    int maxDepth;
    std::vector<unsigned char> image; // RGBA format
    
    // OpenGL resources for displaying the image
    unsigned int textureID;
    unsigned int quadVAO, quadVBO;
    unsigned int shaderProgram;
    
    void setupOpenGL();
};