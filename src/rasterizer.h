#ifndef RASTERIZER_H
#define RASTERIZER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class Rasterizer {
private:
    // Window dimensions
    int width, height;
    
    // Line parameters
    glm::vec2 startPoint, endPoint;
    glm::vec3 lineColor;
    
    // Framebuffer objects
    GLuint framebufferFBO;
    GLuint framebufferTexture;
    
    // Add these new members
    std::vector<float> frameBuffer;
    bool framebufferDirty;
    
    // Display objects
    GLuint quadVAO, quadVBO;
    GLuint displayShader;
    
    // Setup methods
    void setupFramebuffer();
    void setupQuad();
    void setupShaders();
    
    // Line drawing algorithms
    void basicLineRasterization(int x0, int y0, int x1, int y1);
    void bresenhamLine(int x0, int y0, int x1, int y1);
    
    // Add this new method
    void updateFramebuffer();
    
public:
    Rasterizer(int width, int height);
    ~Rasterizer();
    
    void resize(int width, int height);
    void setPixel(int x, int y, const glm::vec3& color);
    void drawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec3& color);
    void clear(const glm::vec3& color = glm::vec3(0.0f));
    
    void update();
    void render();
    
    // Getters/setters for line parameters
    const glm::vec2& getStartPoint() const { return startPoint; }
    const glm::vec2& getEndPoint() const { return endPoint; }
    const glm::vec3& getLineColor() const { return lineColor; }
    
    void setStartPoint(const glm::vec2& start) { startPoint = start; }
    void setEndPoint(const glm::vec2& end) { endPoint = end; }
    void setLineColor(const glm::vec3& color) { lineColor = color; }
};

#endif // RASTERIZER_H