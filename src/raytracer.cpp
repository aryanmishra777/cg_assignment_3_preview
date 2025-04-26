#include "raytracer.h"
#include "math_utils.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>

// Shader sources for displaying the framebuffer (same as in scanline.cpp)
const char* raytraceVertexShaderSource = R"(
    #version 430 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;
    
    out vec2 TexCoord;
    
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char* raytraceFragmentShaderSource = R"(
    #version 430 core
    in vec2 TexCoord;
    
    out vec4 FragColor;
    
    uniform sampler2D screenTexture;
    
    void main() {
        FragColor = texture(screenTexture, TexCoord);
    }
)";

// Sphere intersection implementation
RayHit Sphere::intersect(const Ray& ray) const {
    RayHit hit;
    
    // Vector from ray origin to sphere center
    glm::vec3 oc = ray.origin - position;
    
    // Quadratic coefficients
    float a = glm::dot(ray.direction, ray.direction);
    float b = 2.0f * glm::dot(oc, ray.direction);
    float c = glm::dot(oc, oc) - radius * radius;
    
    // Discriminant
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant >= 0) {
        // Calculate intersection distances
        float t1 = (-b - sqrt(discriminant)) / (2.0f * a);
        float t2 = (-b + sqrt(discriminant)) / (2.0f * a);
        
        // Find closest positive intersection
        float t = (t1 > 0) ? t1 : ((t2 > 0) ? t2 : -1.0f);
        
        if (t > 0) {
            hit.hit = true;
            hit.distance = t;
            hit.point = ray.origin + t * ray.direction;
            hit.normal = glm::normalize(hit.point - position);
            hit.material = material;
        }
    }
    
    return hit;
}

// Cube intersection implementation
RayHit Cube::intersect(const Ray& ray) const {
    RayHit hit;
    
    // Transform ray to object space
    glm::mat4 invRotation = glm::inverse(rotation);
    glm::vec3 localOrigin = glm::vec3(invRotation * glm::vec4(ray.origin - position, 1.0f));
    glm::vec3 localDirection = glm::vec3(invRotation * glm::vec4(ray.direction, 0.0f));
    
    // Bounds of the cube in local space
    glm::vec3 min = -size * 0.5f;
    glm::vec3 max = size * 0.5f;
    
    // Ray-box intersection using slab method
    float tmin = -std::numeric_limits<float>::infinity();
    float tmax = std::numeric_limits<float>::infinity();
    int hitAxis = -1;
    bool hitPositive = false;
    
    // Check each axis
    for (int i = 0; i < 3; i++) {
        if (std::abs(localDirection[i]) < 1e-5) {
            // Ray is parallel to this slab, check if origin is within bounds
            if (localOrigin[i] < min[i] || localOrigin[i] > max[i]) {
                return hit; // No intersection
            }
        } else {
            // Compute intersection distances with the slabs
            float ood = 1.0f / localDirection[i];
            float t1 = (min[i] - localOrigin[i]) * ood;
            float t2 = (max[i] - localOrigin[i]) * ood;
            
            // Swap if needed
            if (t1 > t2) std::swap(t1, t2);
            
            // Update tmin and tmax
            if (t1 > tmin) {
                tmin = t1;
                hitAxis = i;
                hitPositive = localDirection[i] < 0;
            }
            if (t2 < tmax) tmax = t2;
            
            // Check if there's a valid intersection interval
            if (tmin > tmax) return hit;
        }
    }
    
    // Check if intersection is in front of the ray
    if (tmin < 0) {
        if (tmax < 0) return hit; // Both intersections behind the ray
        
        // Use tmax as the intersection point
        tmin = tmax;
        hitPositive = !hitPositive;
        
        // Find the axis for the outgoing intersection
        float maxT = -std::numeric_limits<float>::infinity();
        for (int i = 0; i < 3; i++) {
            if (std::abs(localDirection[i]) < 1e-5) continue;
            
            float ood = 1.0f / localDirection[i];
            float t = (localDirection[i] > 0 ? max[i] : min[i] - localOrigin[i]) * ood;
            
            if (t > maxT) {
                maxT = t;
                hitAxis = i;
                hitPositive = localDirection[i] < 0;
            }
        }
    }
    
    // Set hit information
    hit.hit = true;
    hit.distance = tmin;
    hit.point = ray.origin + tmin * ray.direction;
    
    // Compute normal in world space
    glm::vec3 localNormal(0.0f);
    localNormal[hitAxis] = hitPositive ? 1.0f : -1.0f;
    hit.normal = glm::normalize(glm::vec3(glm::transpose(invRotation) * glm::vec4(localNormal, 0.0f)));
    
    hit.material = material;
    
    return hit;
}

// Mesh intersection implementation
RayHit MeshObject::intersect(const Ray& ray) const {
    RayHit hit;
    
    for (const auto& triangle : triangles) {
        // Transform triangle vertices to world space
        glm::vec3 v0 = triangle.v0.position + position;
        glm::vec3 v1 = triangle.v1.position + position;
        glm::vec3 v2 = triangle.v2.position + position;
        
        // Möller–Trumbore algorithm for ray-triangle intersection
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(ray.direction, edge2);
        float a = glm::dot(edge1, h);
        
        // If ray is parallel to triangle
        if (a > -1e-5 && a < 1e-5) continue;
        
        float f = 1.0f / a;
        glm::vec3 s = ray.origin - v0;
        float u = f * glm::dot(s, h);
        
        // Check if intersection is outside triangle
        if (u < 0.0f || u > 1.0f) continue;
        
        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(ray.direction, q);
        
        // Check if intersection is outside triangle
        if (v < 0.0f || u + v > 1.0f) continue;
        
        // Compute distance to intersection
        float t = f * glm::dot(edge2, q);
        
        // Check if intersection is behind the ray or farther than current closest
        if (t < 1e-5 || t > hit.distance) continue;
        
        // Valid intersection
        hit.hit = true;
        hit.distance = t;
        hit.point = ray.origin + t * ray.direction;
        
        // Compute normal - use the triangle normal or interpolate vertex normals
        hit.normal = glm::normalize(glm::cross(edge1, edge2));
        
        // Set material
        hit.material = material;
    }
    
    return hit;
}

// Camera ray generation
Ray Camera::generateRay(float x, float y) const {
    // Convert to NDC space
    float ndc_x = (2.0f * x) - 1.0f;
    float ndc_y = 1.0f - (2.0f * y); // Flip y coordinate
    
    // Calculate view direction vectors
    glm::vec3 forward = glm::normalize(lookAt - position);
    glm::vec3 right = glm::normalize(glm::cross(forward, up));
    glm::vec3 upVector = glm::cross(right, forward);
    
    // Calculate field of view
    float fovRadians = glm::radians(fov);
    float tanFov = tan(fovRadians / 2.0f);
    
    // Calculate ray direction
    glm::vec3 direction = glm::normalize(
        forward
        + (ndc_x * tanFov * aspectRatio) * right
        + (ndc_y * tanFov) * upVector
    );
    
    return Ray(position, direction);
}

RayTracer::RayTracer(int w, int h) : width(w), height(h) {
    // Initialize ray tracing parameters
    maxDepth = 3;
    enableShadows = true;
    enableReflections = true;
    
    // Setup camera
    camera = Camera(
        glm::vec3(0.0f, 0.0f, 5.0f),   // Position
        glm::vec3(0.0f, 0.0f, 0.0f),   // Look at
        glm::vec3(0.0f, 1.0f, 0.0f),   // Up vector
        45.0f,                          // FOV
        static_cast<float>(width) / static_cast<float>(height) // Aspect ratio
    );
    
    // Setup a default light
    lights.push_back(Light(glm::vec3(5.0f, 5.0f, 5.0f)));
    
    // Setup OpenGL objects
    setupFramebuffer();
    setupQuad();
    setupShaders();
    
    // Clear the framebuffer initially
    clear(glm::vec3(0.2f, 0.2f, 0.3f)); // Dark blue-gray background
}

RayTracer::~RayTracer() {
    // Cleanup OpenGL resources
    glDeleteTextures(1, &framebufferTexture);
    glDeleteFramebuffers(1, &framebufferFBO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteProgram(displayShader);
    
    // Clear scene
    objects.clear();
    lights.clear();
}

void RayTracer::setupFramebuffer() {
    // Create a texture for the framebuffer
    glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Create a framebuffer object
    glGenFramebuffers(1, &framebufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);
    
    // Check framebuffer status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RayTracer::setupQuad() {
    // Create a quad for displaying the framebuffer
    float quadVertices[] = {
        // Positions     // Texture Coords
        -1.0f,  1.0f,    0.0f, 1.0f,
        -1.0f, -1.0f,    0.0f, 0.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
        
        -1.0f,  1.0f,    0.0f, 1.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
         1.0f,  1.0f,    1.0f, 1.0f
    };
    
    // Create VAO and VBO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(quadVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void RayTracer::setupShaders() {
    // Create and compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &raytraceVertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for vertex shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }
    
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &raytraceFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Check for fragment shader compilation errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }
    
    // Create shader program
    displayShader = glCreateProgram();
    glAttachShader(displayShader, vertexShader);
    glAttachShader(displayShader, fragmentShader);
    glLinkProgram(displayShader);
    
    // Check for linking errors
    glGetProgramiv(displayShader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(displayShader, 512, NULL, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }
    
    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void RayTracer::resize(int w, int h) {
    // Update dimensions
    width = w;
    height = h;
    
    // Update camera aspect ratio
    camera.setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    
    // Recreate framebuffer texture with new size
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RayTracer::addSphere(const glm::vec3& position, float radius, const Material& material) {
    objects.push_back(std::make_shared<Sphere>(position, radius, material));
}

void RayTracer::addCube(const glm::vec3& position, const glm::vec3& size, const Material& material) {
    objects.push_back(std::make_shared<Cube>(position, size, material));
}

void RayTracer::addMesh(const glm::vec3& position, const Mesh* mesh, const Material& material) {
    objects.push_back(std::make_shared<MeshObject>(position, mesh->getTriangles(), material));
}

void RayTracer::addLight(const Light& light) {
    lights.push_back(light);
}

void RayTracer::clearScene() {
    objects.clear();
    lights.clear();
}

void RayTracer::setPixel(int x, int y, const glm::vec3& color) {
    // Check if the pixel is within bounds
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }
    
    // Bind framebuffer for pixel writing
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferFBO);
    
    // Set the pixel color
    glm::vec3 pixelColor = color;
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    
    // Write the pixel
    glReadPixels(x, y, 1, 1, GL_RGB, GL_FLOAT, &pixelColor);
    glDrawPixels(1, 1, GL_RGB, GL_FLOAT, &color);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RayTracer::clear(const glm::vec3& color) {
    // Bind framebuffer for clearing
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferFBO);
    glClearColor(color.r, color.g, color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

RayHit RayTracer::findClosestIntersection(const Ray& ray) {
    RayHit closestHit;
    
    // Check intersection with all objects
    for (const auto& object : objects) {
        RayHit hit = object->intersect(ray);
        
        // If there's a hit and it's closer than the current closest
        if (hit.hit && hit.distance < closestHit.distance) {
            closestHit = hit;
            closestHit.object = object;
        }
    }
    
    return closestHit;
}

bool RayTracer::isInShadow(const glm::vec3& point, const Light& light) {
    if (!enableShadows) return false;
    
    // Direction from point to light
    glm::vec3 lightDir = light.position - point;
    float lightDistance = glm::length(lightDir);
    lightDir = glm::normalize(lightDir);
    
    // Create shadow ray (add a small offset to avoid self-intersection)
    Ray shadowRay(point + 0.001f * lightDir, lightDir);
    
    // Check if any object blocks the light
    for (const auto& object : objects) {
        RayHit hit = object->intersect(shadowRay);
        
        // If there's a hit between the point and the light
        if (hit.hit && hit.distance < lightDistance) {
            return true; // Point is in shadow
        }
    }
    
    return false; // No shadow
}

glm::vec3 RayTracer::traceRay(const Ray& ray, int depth) {
    // Base case for recursion
    if (depth <= 0) {
        return glm::vec3(0.0f);
    }
    
    // Find closest intersection
    RayHit hit = findClosestIntersection(ray);
    
    // If no intersection, return background color
    if (!hit.hit) {
        return glm::vec3(0.2f, 0.2f, 0.3f); // Dark blue-gray background
    }
    
    // Get material properties
    glm::vec3 materialColor = hit.material.color;
    float ambient = hit.material.ambient;
    float diffuse = hit.material.diffuse;
    float specular = hit.material.specular;
    float shininess = hit.material.shininess;
    float reflectivity = hit.material.reflectivity;
    
    // Calculate lighting
    glm::vec3 color = ambient * materialColor; // Ambient component
    
    for (const auto& light : lights) {
        // Skip if in shadow
        if (isInShadow(hit.point, light)) {
            continue;
        }
        
        // Direction to light
        glm::vec3 lightDir = glm::normalize(light.position - hit.point);
        
        // Diffuse component
        float diff = std::max(glm::dot(hit.normal, lightDir), 0.0f);
        glm::vec3 diffuseColor = diffuse * diff * materialColor * light.color * light.intensity;
        
        // Specular component
        glm::vec3 viewDir = glm::normalize(-ray.direction);
        glm::vec3 reflectDir = glm::reflect(-lightDir, hit.normal);
        float spec = std::pow(std::max(glm::dot(viewDir, reflectDir), 0.0f), shininess);
        glm::vec3 specularColor = specular * spec * light.color * light.intensity;
        
        // Add to final color
        color += diffuseColor + specularColor;
    }
    
    // Add reflection
    if (enableReflections && reflectivity > 0.0f) {
        // Calculate reflection ray
        glm::vec3 reflectDir = glm::reflect(ray.direction, hit.normal);
        Ray reflectionRay(hit.point + 0.001f * reflectDir, reflectDir);
        
        // Recursively trace reflection ray
        glm::vec3 reflectionColor = traceRay(reflectionRay, depth - 1);
        
        // Add reflection to final color
        color = color * (1.0f - reflectivity) + reflectionColor * reflectivity;
    }
    
    // Clamp color values to [0, 1]
    color = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
    
    return color;
}

void RayTracer::trace() {
    // Skip if there are no objects or lights
    if (objects.empty() || lights.empty()) {
        return;
    }
    
    // Multithreaded ray tracing
    const int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    std::mutex mtx;
    
    // Define function for tracing a section of the image
    auto trace_section = [this, &mtx](int start_y, int end_y) {
        for (int y = start_y; y < end_y; y++) {
            for (int x = 0; x < width; x++) {
                // Calculate normalized device coordinates
                float u = static_cast<float>(x) / static_cast<float>(width);
                float v = static_cast<float>(y) / static_cast<float>(height);
                
                // Generate ray from camera
                Ray ray = camera.generateRay(u, v);
                
                // Trace the ray
                glm::vec3 color = traceRay(ray, maxDepth);
                
                // Set the pixel color
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    setPixel(x, y, color);
                }
            }
        }
    };
    
    // Divide the image into sections for each thread
    int section_height = height / num_threads;
    for (int i = 0; i < num_threads; i++) {
        int start_y = i * section_height;
        int end_y = (i == num_threads - 1) ? height : (i + 1) * section_height;
        
        threads.emplace_back(trace_section, start_y, end_y);
    }
    
    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
}

void RayTracer::update() {
    // Add a default scene if empty
    if (objects.empty()) {
        // Add some default objects
        Material redMaterial;
        redMaterial.color = glm::vec3(1.0f, 0.1f, 0.1f);
        redMaterial.reflectivity = 0.3f;
        
        Material blueMaterial;
        blueMaterial.color = glm::vec3(0.1f, 0.1f, 1.0f);
        blueMaterial.reflectivity = 0.3f;
        
        Material greenMaterial;
        greenMaterial.color = glm::vec3(0.1f, 0.8f, 0.1f);
        greenMaterial.reflectivity = 0.1f;
        
        // Ground plane as a large flat cube
        addCube(glm::vec3(0.0f, -1.5f, 0.0f), glm::vec3(10.0f, 0.1f, 10.0f), greenMaterial);
        
        // Add two spheres
        addSphere(glm::vec3(-1.0f, 0.0f, -1.0f), 1.0f, redMaterial);
        addSphere(glm::vec3(1.5f, 0.0f, 0.0f), 1.0f, blueMaterial);
    }
    
    // If no lights, add a default light
    if (lights.empty()) {
        addLight(Light(glm::vec3(5.0f, 5.0f, 5.0f)));
    }
    
    // Trace the scene
    trace();
}

void RayTracer::render() {
    // Bind back to the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Clear default framebuffer
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Use the display shader
    glUseProgram(displayShader);
    
    // Bind the framebuffer texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glUniform1i(glGetUniformLocation(displayShader, "screenTexture"), 0);
    
    // Render the quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Reset OpenGL state
    glUseProgram(0);
}