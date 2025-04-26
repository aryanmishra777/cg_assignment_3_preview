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

enum ObjectType {
    SPHERE,
    CUBE,
    MESH
};

struct Material {
    glm::vec3 color;
    float ambient, diffuse, specular, shininess, reflectivity;
    Material() : color(1.0f), ambient(0.1f), diffuse(0.7f), specular(0.5f), shininess(32.0f), reflectivity(0.0f) {}
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    Ray(const glm::vec3& o, const glm::vec3& d) : origin(o), direction(glm::normalize(d)) {}
};

struct RayHit {
    bool hit;
    float distance;
    glm::vec3 point, normal;
    Material material;
    std::shared_ptr<Object> object;
    RayHit() : hit(false), distance(std::numeric_limits<float>::max()) {}
};

struct Light {
    glm::vec3 position, color;
    float intensity;
    Light(const glm::vec3& pos, const glm::vec3& col = glm::vec3(1.0f), float intens = 1.0f)
        : position(pos), color(col), intensity(intens) {}
};

class Object {
protected:
    glm::vec3 position;
    Material material;
public:
    Object(const glm::vec3& pos, const Material& mat) : position(pos), material(mat) {}
    virtual ~Object() {}
    virtual RayHit intersect(const Ray& ray) const = 0;
    glm::vec3 getPosition() const { return position; }
    void setPosition(const glm::vec3& pos) { position = pos; }
    Material getMaterial() const { return material; }
    void setMaterial(const Material& mat) { material = mat; }
};

class Sphere : public Object {
    float radius;
public:
    Sphere(const glm::vec3& pos, float r, const Material& mat) : Object(pos, mat), radius(r) {}
    RayHit intersect(const Ray& ray) const override;
    float getRadius() const { return radius; }
    void setRadius(float r) { radius = r; }
};

class Cube : public Object {
    glm::vec3 size;
    glm::mat4 rotation;
public:
    Cube(const glm::vec3& pos, const glm::vec3& s, const Material& mat)
        : Object(pos, mat), size(s), rotation(1.0f) {}
    RayHit intersect(const Ray& ray) const override;
    glm::vec3 getSize() const { return size; }
    void setSize(const glm::vec3& s) { size = s; }
    glm::mat4 getRotation() const { return rotation; }
    void setRotation(const glm::mat4& rot) { rotation = rot; }
};

class MeshObject : public Object {
    std::vector<Triangle> triangles;
public:
    MeshObject(const glm::vec3& pos, const std::vector<Triangle>& tris, const Material& mat)
        : Object(pos, mat), triangles(tris) {}
    RayHit intersect(const Ray& ray) const override;
    const std::vector<Triangle>& getTriangles() const { return triangles; }
};

class Camera {
    glm::vec3 position, lookAt, up;
    float fov, aspectRatio;
public:
    Camera(const glm::vec3& pos = glm::vec3(0,0,5), const glm::vec3& look = glm::vec3(0,0,0),
        const glm::vec3& up = glm::vec3(0,1,0), float fov = 45.0f, float aspect = 1.0f)
        : position(pos), lookAt(look), up(up), fov(fov), aspectRatio(aspect) {}
    Ray generateRay(float x, float y) const;
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
    int width, height;
    GLuint framebufferTexture, framebufferFBO, displayShader, quadVAO, quadVBO;
    std::vector<glm::vec3> frameBuffer;
    bool framebufferDirty;
    std::vector<std::shared_ptr<Object>> objects;
    std::vector<Light> lights;
    Camera camera;
    int maxDepth;
    bool enableShadows, enableReflections;
    bool debugShadowView; // Added shadow debug view flag
    void setupFramebuffer();
    void setupQuad();
    void setupShaders();
    void updateFramebuffer();
    glm::vec3 traceRay(const Ray& ray, int depth);
    RayHit findClosestIntersection(const Ray& ray);
    bool isInShadow(const glm::vec3& point, const Light& light);
    void setPixel(int x, int y, const glm::vec3& color);
public:
    RayTracer(int w, int h);
    ~RayTracer();
    void resize(int w, int h);
    void addSphere(const glm::vec3& position, float radius, const Material& material);
    void addCube(const glm::vec3& position, const glm::vec3& size, const Material& material);
    void addMesh(const glm::vec3& position, const Mesh* mesh, const Material& material);
    void addLight(const Light& light);
    void clearLights() { lights.clear(); }
    void clearScene();
    const std::vector<std::shared_ptr<Object>>& getObjects() const { return objects; }
    const std::vector<Light>& getLights() const { return lights; }
    Camera& getCamera() { return camera; }
    void setMaxDepth(int depth) { maxDepth = depth; }
    int getMaxDepth() const { return maxDepth; }
    void setEnableShadows(bool enable) { enableShadows = enable; }
    bool isShadowsEnabled() const { return enableShadows; }
    void setEnableReflections(bool enable) { enableReflections = enable; }
    bool isReflectionsEnabled() const { return enableReflections; }
    void setDebugShadowView(bool enable) { debugShadowView = enable; } // Added getter/setter
    bool getDebugShadowView() const { return debugShadowView; }
    void trace();
    void clear(const glm::vec3& color = glm::vec3(0.0f));
    void update();
    void render();
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

#endif // RAYTRACER_H