#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Pixel {
    int x, y;
    float r, g, b;
    
    Pixel(int x, int y, float r = 1.0f, float g = 1.0f, float b = 1.0f)
        : x(x), y(y), r(r), g(g), b(b) {}
};

class LineRasterizer {
public:
    LineRasterizer();
    ~LineRasterizer();
    
    // Rasterize a line from (x0, y0) to (x1, y1)
    std::vector<Pixel> rasterizeLine(int x0, int y0, int x1, int y1);
    
    // Render the rasterized line to the screen
    void renderPixels(const std::vector<Pixel>& pixels);
    
    // Clear the rasterization buffer
    void clear();
    
private:
    // Helper methods for various line cases
    std::vector<Pixel> rasterizeLowSlope(int x0, int y0, int x1, int y1);
    std::vector<Pixel> rasterizeHighSlope(int x0, int y0, int x1, int y1);
    
    // OpenGL buffer objects
    unsigned int pixelVAO, pixelVBO;
    void setupBuffers();
};