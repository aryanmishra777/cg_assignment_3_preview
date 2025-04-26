#ifndef GUI_H
#define GUI_H

#include "../imgui/imgui.h"
#include <string> // Add this include for std::string
#include "mesh.h"
#include "slicer.h"
#include "rasterizer.h"
#include "scanline.h"
#include "raytracer.h"

enum ViewMode {
    VIEW_3D,
    VIEW_SLICE,
    VIEW_RASTER,
    VIEW_SCANLINE,
    VIEW_RAYTRACE
};

class GUI {
public:
    // GUI state
    bool showDemoWindow;
    bool showMetricsWindow;
    bool showAppMainMenuBar;
    
    // View-specific GUI state
    // Slicing parameters
    float planeNormal[4][3] = {
        {0.0f, 1.0f, 0.0f},  // Default is horizontal plane
        {1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f}
    };
    float planeDistance[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    int activePlaneIndex = 0;
    int numPlanes = 1;
    
    // Rasterization parameters
    float lineStart[2] = {0.25f, 0.5f};  // Default line start position (as fraction of screen)
    float lineEnd[2] = {0.75f, 0.5f};    // Default line end position (as fraction of screen)
    float lineColor[3] = {1.0f, 1.0f, 1.0f}; // White
    
    // Scan conversion parameters
    float polygonVertices[10][2] = {
        {0.3f, 0.3f},
        {0.7f, 0.3f},
        {0.7f, 0.7f},
        {0.3f, 0.7f}
    };
    int numPolygonVertices = 4;
    float fillColor[3] = {0.0f, 1.0f, 0.0f}; // Green
    
    // Ray tracing parameters
    int maxDepth = 3;
    bool enableShadows = true;
    bool enableReflections = true;
    float spherePosition[3] = {0.0f, 0.0f, 0.0f};
    float sphereRadius = 1.0f;
    float sphereColor[3] = {1.0f, 0.0f, 0.0f}; // Red
    float lightPosition[3] = {5.0f, 5.0f, 5.0f};
    float lightColor[3] = {1.0f, 1.0f, 1.0f}; // White
    float lightIntensity = 1.0f;

    // Additional GUI state
    bool enableRegionColoring = true;

    // Cube parameters
    float cubePosition[3] = {0.0f, 0.0f, 0.0f};
    float cubeSize[3] = {1.0f, 1.0f, 1.0f};
    float cubeColor[3] = {0.0f, 1.0f, 0.0f}; // Green

    // Window size/position tracking for fullscreen toggle
    int windowPosX = 100;
    int windowPosY = 100;
    int windowWidth = 1280;
    int windowHeight = 720;

    // Mesh loading
    bool loadMeshRequested = false;
    std::string meshPathToLoad = "";
    
    // Methods
    void renderMainMenuBar();
    void renderSlicingControls(MeshSlicer* slicer);
    void renderRasterizationControls(Rasterizer* rasterizer, int width, int height, ViewMode* currentView);
    void renderScanConversionControls(ScanLineRenderer* scanline, int width, int height);
    void renderRayTracingControls(RayTracer* raytracer, Mesh* mesh);
    void renderMeshLoadingDialog();

    GUI();
    
    void render(ViewMode* currentView, Mesh* mesh, MeshSlicer* slicer, 
                Rasterizer* rasterizer, ScanLineRenderer* scanline, 
                RayTracer* raytracer);
};

#endif // GUI_H