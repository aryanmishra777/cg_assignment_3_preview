#ifndef SCANLINE_H
#define SCANLINE_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <list>

// Updated Edge structure for scan-line algorithm using integer arithmetic
struct Edge {
    int ymax;       // Maximum y coordinate of the edge
    int x;          // Current x coordinate (fixed-point, scaled by 1024)
    int dx;         // Change in x for each unit y (fixed-point, scaled by 1024)
    
    Edge(int y, int x_start, int slope) : ymax(y), x(x_start), dx(slope) {}
    
    // Comparison operator for sorting edges by x coordinate
    bool operator<(const Edge& other) const {
        return x < other.x;
    }
};

class ScanLineRenderer {
private:
    // Canvas dimensions
    int width, height;
    
    // Framebuffer for scan-line rendering
    GLuint framebufferTexture;
    GLuint framebufferFBO;
    
    // Shader for displaying the framebuffer
    GLuint displayShader;
    GLuint quadVAO, quadVBO;
    
    // Polygon vertices
    std::vector<glm::vec2> polygonVertices;
    
    // Fill color
    glm::vec3 fillColor;
    
    // Edge table for scan-line algorithm (indexed by y)
    std::vector<std::list<Edge>> edgeTable;
    
    // Bounds for the algorithm
    int ymin, ymax;

    // New members
    std::vector<float> frameBuffer;
    bool framebufferDirty;
    
    // Methods
    void setupFramebuffer();
    void setupQuad();
    void setupShaders();
    
    // Scan-line fill algorithm methods
    void findYMinMax();
    void buildEdgeTable();
    void scanLineFill();
    
    // Pixel setting
    void setPixel(int x, int y, const glm::vec3& color);

    // Buffer update
    void updateFramebuffer();
    
public:
    ScanLineRenderer(int w, int h);
    ~ScanLineRenderer();
    
    // Resize the renderer canvas
    void resize(int w, int h);
    
    // Polygon management
    void addVertex(const glm::vec2& vertex);
    void clearPolygon();
    void setFillColor(const glm::vec3& color) { fillColor = color; }
    
    // Get polygon vertices
    const std::vector<glm::vec2>& getPolygonVertices() const { return polygonVertices; }
    
    // Fill the current polygon
    void fillPolygon();
    
    // Clear the framebuffer
    void clear(const glm::vec3& color = glm::vec3(0.0f));
    
    // Update and render
    void update();
    void render();
};

#endif // SCANLINE_H