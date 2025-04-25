#include "raytracer.h"
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>

// Scene implementation
Scene::Scene() : backgroundColor(0.2f, 0.2f, 0.2f) {
}

Scene::~Scene() {
}

void Scene::addPrimitive(std::unique_ptr<Primitive> primitive) {
    primitives.push_back(std::move(primitive));
}

void Scene::addLight(const Light& light) {
    lights.push_back(light);
}

void Scene::setCamera(const Camera& camera) {
    this->camera = camera;
}

void Scene::setBackgroundColor(const glm::vec3& color) {
    backgroundColor = color;
}

HitInfo Scene::trace(const Ray& ray) const {
    HitInfo result;
    
    for (const auto& primitive : primitives) {
        HitInfo hit = primitive->intersect(ray);
        
        if (hit.hit && hit.distance < result.distance) {
            result = hit;
        }
    }
    
    return result;
}

// RayTracer implementation
RayTracer::RayTracer() 
    : width(800), height(600), 
      reflectionsEnabled(false), shadowsEnabled(true),
      maxDepth(3), textureID(0), quadVAO(0), quadVBO(0), shaderProgram(0) {
    
    image.resize(width * height * 4, 0);
    setupOpenGL();
}

RayTracer::~RayTracer() {
    if (textureID) {
        glDeleteTextures(1, &textureID);
    }
    if (quadVAO) {
        glDeleteVertexArrays(1, &quadVAO);
    }
    if (quadVBO) {
        glDeleteBuffers(1, &quadVBO);
    }
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }
}

void RayTracer::setupOpenGL() {
    // Create and compile shaders using our custom function
    shaderProgram = createShaderProgram("shaders/quad.vert", "shaders/quad.frag");
    
    // Create texture for displaying the ray-traced image
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Create quad for displaying the texture
    float quadVertices[] = {
        // positions   // texture coords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // Texture coord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

unsigned int RayTracer::createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    // Load shader source code
    std::string vertexCode = loadShaderSource(vertexPath);
    std::string fragmentCode = loadShaderSource(fragmentPath);
    
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    
    // Compile shaders
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];
    
    // Vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    
    // Check for compilation errors
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    
    // Check for compilation errors
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Shader program
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    
    // Check for linking errors
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    // Delete shaders as they're linked into the program and no longer needed
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    return program;
}

std::string RayTracer::loadShaderSource(const std::string& path) {
    std::ifstream file;
    std::stringstream stream;
    
    // Ensure ifstream can throw exceptions
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        file.open(path);
        stream << file.rdbuf();
        file.close();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << path << std::endl;
    }
    
    return stream.str();
}

void RayTracer::setDimensions(int width, int height) {
    this->width = width;
    this->height = height;
    image.resize(width * height * 4, 0);
}

void RayTracer::enableReflections(bool enable) {
    reflectionsEnabled = enable;
}

void RayTracer::enableShadows(bool enable) {
    shadowsEnabled = enable;
}

void RayTracer::render() {
    // Get camera parameters
    const Camera& camera = scene.getCamera();
    
    // Calculate camera vectors
    glm::vec3 forward = glm::normalize(camera.target - camera.position);
    glm::vec3 right = glm::normalize(glm::cross(forward, camera.up));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));
    
    float halfWidth = tan(glm::radians(camera.fov / 2.0f));
    float halfHeight = halfWidth / camera.aspectRatio;
    
    // Clear image
    std::fill(image.begin(), image.end(), 0);
    
    // Render each pixel
    #ifdef _OPENMP
        #pragma omp parallel for collapse(2)
    #endif
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Calculate ray direction
            float u = (2.0f * (x + 0.5f) / width - 1.0f) * halfWidth;
            float v = (1.0f - 2.0f * (y + 0.5f) / height) * halfHeight;
            
            glm::vec3 direction = glm::normalize(forward + u * right + v * up);
            Ray ray(camera.position, direction);
            
            // Trace ray and get color
            glm::vec3 color = traceRay(ray, 0);
            
            // Clamp and convert to 8-bit color
            color = glm::clamp(color, 0.0f, 1.0f);
            int index = (y * width + x) * 4;
            
            image[index + 0] = static_cast<unsigned char>(color.r * 255);
            image[index + 1] = static_cast<unsigned char>(color.g * 255);
            image[index + 2] = static_cast<unsigned char>(color.b * 255);
            image[index + 3] = 255; // Alpha
        }
    }
    
    // Update texture with the rendered image
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

glm::vec3 RayTracer::traceRay(const Ray& ray, int depth) const {
    // Check recursion depth
    if (depth > maxDepth) {
        return glm::vec3(0.0f);
    }
    
    // Find closest intersection
    HitInfo hit = scene.trace(ray);
    
    // No intersection, return background color
    if (!hit.hit) {
        return scene.getBackgroundColor();
    }
    
    // Calculate lighting
    glm::vec3 color = calculateLighting(hit, ray);
    
    // Calculate reflection
    if (reflectionsEnabled && hit.material.reflectivity > 0.0f) {
        glm::vec3 reflectionDir = glm::reflect(ray.direction, hit.normal);
        Ray reflectionRay(hit.point + 0.001f * hit.normal, reflectionDir);
        
        glm::vec3 reflectionColor = traceRay(reflectionRay, depth + 1);
        color = color * (1.0f - hit.material.reflectivity) + reflectionColor * hit.material.reflectivity;
    }
    
    return color;
}

glm::vec3 RayTracer::calculateLighting(const HitInfo& hit, const Ray& ray) const {
    const std::vector<Light>& lights = scene.getLights();
    glm::vec3 color = hit.material.color * hit.material.ambient; // Ambient component
    
    for (const auto& light : lights) {
        // Calculate light direction and distance
        glm::vec3 lightDir = light.position - hit.point;
        float lightDistance = glm::length(lightDir);
        lightDir = glm::normalize(lightDir);
        
        // Check for shadows
        bool inShadow = false;
        if (shadowsEnabled) {
            inShadow = isInShadow(hit.point, lightDir, lightDistance);
        }
        
        if (!inShadow) {
            // Diffuse component
            float diff = std::max(glm::dot(hit.normal, lightDir), 0.0f);
            glm::vec3 diffuse = hit.material.color * hit.material.diffuse * diff;
            
            // Specular component
            glm::vec3 viewDir = glm::normalize(-ray.direction);
            glm::vec3 reflectDir = glm::reflect(-lightDir, hit.normal);
            float spec = std::pow(std::max(glm::dot(viewDir, reflectDir), 0.0f), hit.material.shininess);
            glm::vec3 specular = light.color * hit.material.specular * spec;
            
            // Attenuation
            float attenuation = 1.0f / (1.0f + 0.09f * lightDistance + 0.032f * lightDistance * lightDistance);
            
            // Add to final color
            color += (diffuse + specular) * light.color * light.intensity * attenuation;
        }
    }
    
    return color;
}

bool RayTracer::isInShadow(const glm::vec3& point, const glm::vec3& lightDir, float lightDistance) const {
    // Create shadow ray
    Ray shadowRay(point + 0.001f * lightDir, lightDir);
    
    // Check for intersection
    HitInfo shadowHit = scene.trace(shadowRay);
    
    // Return true if an object is between the point and the light
    return shadowHit.hit && shadowHit.distance < lightDistance;
}

void RayTracer::displayImage() const {
    // Use the shader program
    glUseProgram(shaderProgram);
    
    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(shaderProgram, "rayTracedTexture"), 0);
    
    // Render quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}