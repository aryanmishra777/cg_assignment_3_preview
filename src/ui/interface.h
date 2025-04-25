#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // Add this for lookAt and perspective
#include <vector>
#include <string>
#include <memory>

// Forward declarations
class Mesh;
class MeshSlicer;
class LineRasterizer;
class PolygonFill;
class RayTracer;

class UserInterface {
public:
    // Constructor and destructor
    UserInterface(GLFWwindow* window);
    ~UserInterface();
    
    // Main render method
    void render(Mesh& mesh, MeshSlicer& slicer, LineRasterizer& rasterizer, 
                PolygonFill& polygonFill, RayTracer& rayTracer);
    
    // Camera access methods for mesh viewing
    glm::mat4 getMeshViewMatrix() const {
        return glm::lookAt(
            glm::vec3(meshCameraPos[0], meshCameraPos[1], meshCameraPos[2]),
            glm::vec3(meshCameraTarget[0], meshCameraTarget[1], meshCameraTarget[2]),
            glm::vec3(meshCameraUp[0], meshCameraUp[1], meshCameraUp[2])
        );
    }
    
    glm::mat4 getMeshProjectionMatrix() const {
        float aspectRatio = 800.0f / 600.0f; // Or use actual window dimensions
        return glm::perspective(glm::radians(meshCameraFov), aspectRatio, 0.1f, 100.0f);
    }
    
    bool isShowingMesh() const { return showMesh; }
    GLuint getMeshShaderProgram() const { return meshShaderProgram; }
    
private:
    // Constants
    static const int MAX_PLANES = 4;
    
    // Setup method
    void setupImGui(GLFWwindow* window);
    
    // UI rendering methods for each component
    void renderMeshSlicingUI(Mesh& mesh, MeshSlicer& slicer);
    void renderRasterizationUI(LineRasterizer& rasterizer);
    void renderScanConversionUI(PolygonFill& polygonFill);
    void renderRayTracingUI(RayTracer& rayTracer);
    
    // Helper methods
    void showHelpMarker(const std::string& desc);
    unsigned int createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);
    std::string loadShaderSource(const std::string& path);
    
    // UI state variables
    int currentTab;
    char filePathBuffer[256];
    int numPlanes;
    float planeEq[4][4];  // Up to 4 planes, each with 4 coefficients (a,b,c,d)
    int linePoints[2][2]; // Two points, each with x,y coordinates
    bool addingPoint;
    std::vector<glm::vec2> polygonPoints;
    
    // Ray tracing parameters
    int imageWidth;
    int imageHeight;
    bool reflectionsEnabled;
    bool shadowsEnabled;
    
    // Mesh viewing
    bool showMesh = false;
    float meshCameraPos[3] = { 0.0f, 0.0f, 5.0f };
    float meshCameraTarget[3] = { 0.0f, 0.0f, 0.0f };
    float meshCameraUp[3] = { 0.0f, 1.0f, 0.0f };
    float meshCameraFov = 60.0f;
    GLuint meshShaderProgram = 0;
};