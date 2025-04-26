#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "mesh.h"

// Forward declarations
struct Ray;
struct RayHit;
class Object;

// Types of objects for ray tracing
enum ObjectType {
    SPHERE,
    CUBE,
    MESH
};

// Material properties
struct Material {
    glm::vec3 color;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
    float reflectivity;
    
    Material() : 
        color(glm::vec3(1.0f)),
        ambient(0.1f),
        diffuse(0.7f),
        specular(0.5f),
        shininess(32.0f),
        reflectivity(0.0f) {}
};

// Ray structure
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    
    Ray(const glm::vec3& o, const glm::vec3& d) : 
        origin(o), direction(glm::normalize(d)) {}
};

// Ray-object intersection result
struct RayHit {
    bool hit;
    float distance;
    glm::vec3 point;
    glm::vec3 normal;
    Material material;
    std::shared_ptr<Object> object;
    
    RayHit() : hit(false), distance(std::numeric_limits<float>::max()) {}
};

// Light source
struct Light {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    
    Light(const glm::vec3& pos, const glm::vec3& col = glm::vec3(1.0f), float intens = 1.0f) : 
        position(pos), color(col), intensity(intens) {}
};

// Base class for ray-traceable objects
class Object {
protected:
    glm::vec3 position;
    Material material;
    
public:
    Object(const glm::vec3& pos, const Material& mat) : 
        position(pos), material(mat) {}
    virtual ~Object() {}
    
    // Ray intersection test
    virtual RayHit intersect(const Ray& ray) const = 0;
    
    // Getters and setters
    glm::vec3 getPosition() const { return position; }
    void setPosition(const glm::vec3& pos) { position = pos; }
    
    Material getMaterial() const { return material; }
    void setMaterial(const Material& mat) { material = mat; }
};

// Sphere object
class Sphere : public Object {
private:
    float radius;
    
public:
    Sphere(const glm::vec3& pos, float r, const Material& mat) : 
        Object(pos, mat), radius(r) {}
    
    RayHit intersect(const Ray& ray) const override;
    
    float getRadius() const { return radius; }
    void setRadius(float r) { radius = r; }
};

// Cube object
class Cube : public Object {
private:
    glm::vec3 size;
    glm::mat4 rotation;
    
public:
    Cube(const glm::vec3& pos, const glm::vec3& s, const Material& mat) : 
        Object(pos, mat), size(s), rotation(1.0f) {}
    
    RayHit intersect(const Ray& ray) const override;
    
    glm::vec3 getSize() const { return size; }
    void setSize(const glm::vec3& s) { size = s; }
    
    glm::mat4 getRotation() const { return rotation; }
    void setRotation(const glm::mat4& rot) { rotation = rot; }
};

// Triangle mesh object
class MeshObject : public Object {
private:
    std::vector<Triangle> triangles;
    
public:
    MeshObject(const glm::vec3& pos, const std::vector<Triangle>& tris, const Material& mat) : 
        Object(pos, mat), triangles(tris) {}
    
    RayHit intersect(const Ray& ray) const override;
    
    const std::vector<Triangle>& getTriangles() const { return triangles; }
};

// Camera for ray tracing
class Camera {
private:
    glm::vec3 position;
    glm::vec3 lookAt;
    glm::vec3 up;
    float fov;
    float aspectRatio;
    
public:
    Camera(const glm::vec3& pos = glm::vec3(0.0f, 0.0f, 5.0f),
           const glm::vec3& look = glm::vec3(0.0f),
           const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f),
           float fov = 45.0f,
           float aspect = 1.0f) : 
        position(pos), lookAt(look), up(up), fov(fov), aspectRatio(aspect) {}
    
    // Generate primary ray for a pixel
    Ray generateRay(float x, float y) const;
    
    // Getters and setters
    glm::vec3 getPosition() const { return position; }
    void setPosition(const glm::vec3& pos) { position = pos; }
    
    glm::vec3 getLookAt() const { return lookAt; }
    void setLookAt(const glm::vec3& look) { lookAt = look; }
    
    float getFOV() const { return fov; }
    void setFOV(float f) { fov = f; }
    
    float getAspectRatio() const { return aspectRatio; }
    void setAspectRatio(float aspect) { aspectRatio = aspect; }
};

class RayTracer {
private:
    // Canvas dimensions
    int width, height;
    
    // Framebuffer for ray tracing output
    GLuint framebufferTexture;
    GLuint framebufferFBO;
    
    // Shader for displaying the framebuffer
    GLuint displayShader;
    GLuint quadVAO, quadVBO;
    
    // Scene objects
    std::vector<std::shared_ptr<Object>> objects;
    std::vector<Light> lights;
    Camera camera;
    
    // Ray tracing parameters
    int maxDepth;
    bool enableShadows;
    bool enableReflections;
    
    // Methods
    void setupFramebuffer();
    void setupQuad();
    void setupShaders();
    
    // Ray tracing algorithms
    glm::vec3 traceRay(const Ray& ray, int depth);
    RayHit findClosestIntersection(const Ray& ray);
    bool isInShadow(const glm::vec3& point, const Light& light);
    
    // Pixel setting
    void setPixel(int x, int y, const glm::vec3& color);
    
public:
    RayTracer(int w, int h);
    ~RayTracer();
    
    // Resize the ray tracer canvas
    void resize(int w, int h);
    
    // Scene management
    void addSphere(const glm::vec3& position, float radius, const Material& material);
    void addCube(const glm::vec3& position, const glm::vec3& size, const Material& material);
    void addMesh(const glm::vec3& position, const Mesh* mesh, const Material& material);
    void addLight(const Light& light);
    
    void clearLights() {
        lights.clear();
    }
    
    void clearScene();
    
    // Get scene objects
    const std::vector<std::shared_ptr<Object>>& getObjects() const { return objects; }
    const std::vector<Light>& getLights() const { return lights; }
    
    // Camera control
    Camera& getCamera() { return camera; }
    
    // Ray tracing parameters
    void setMaxDepth(int depth) { maxDepth = depth; }
    int getMaxDepth() const { return maxDepth; }
    
    void setEnableShadows(bool enable) { enableShadows = enable; }
    bool isShadowsEnabled() const { return enableShadows; }
    
    void setEnableReflections(bool enable) { enableReflections = enable; }
    bool isReflectionsEnabled() const { return enableReflections; }
    
    // Execute ray tracing
    void trace();
    
    // Clear the framebuffer
    void clear(const glm::vec3& color = glm::vec3(0.0f));
    
    // Update and render
    void update();
    void render();
    
    // Get canvas dimensions
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

#endif // RAYTRACER_H