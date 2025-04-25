#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../rasterization/line.h"

struct Edge {
    int yMin;      // Minimum y-coordinate of edge
    int yMax;      // Maximum y-coordinate of edge
    float xOfYMin; // x-coordinate at yMin
    float invSlope; // Inverse of the slope (dx/dy)
    
    Edge(int y0, int x0, int y1, int x1) {
        if (y0 < y1) {
            yMin = y0;
            yMax = y1;
            xOfYMin = (float)x0;
        } else {
            yMin = y1;
            yMax = y0;
            xOfYMin = (float)x1;
        }
        
        // Calculate inverse slope (avoid division by zero)
        if (y1 != y0) {
            invSlope = (float)(x1 - x0) / (float)(y1 - y0);
        } else {
            invSlope = 0.0f; // Horizontal edge, won't be used
        }
    }
};

class PolygonFill {
public:
    PolygonFill();
    ~PolygonFill();
    
    // Set polygon vertices
    void setPolygon(const std::vector<glm::vec2>& vertices);
    
    // Fill the polygon using scan-line algorithm
    std::vector<Pixel> fillPolygon();
    
    // Render the filled polygon
    void renderFilledPolygon(const std::vector<Pixel>& pixels);
    
    // Clear polygon data
    void clear();
    
private:
    // Build edge table from vertices
    std::vector<Edge> buildEdgeTable(const std::vector<glm::vec2>& vertices);
    
    std::vector<glm::vec2> vertices;
    
    // OpenGL buffer objects
    unsigned int polyVAO, polyVBO;
    void setupBuffers();
};