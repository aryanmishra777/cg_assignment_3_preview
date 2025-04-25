#include <iostream>
#include <memory>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // Add this line for glm::value_ptr

#include "./ui/interface.h"
#include "./mesh/mesh.h"
#include "./mesh/slicer.h"
#include "./rasterization/line.h"
#include "./scan_conversion/polygon_fill.h"
#include "./raytracing/raytracer.h"

// Window dimensions
const unsigned int WINDOW_WIDTH = 1280;
const unsigned int WINDOW_HEIGHT = 720;

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Computer Graphics Assignment", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Initialize UI
    UserInterface ui(window);

    // Create instances of our graphics modules
    Mesh mesh;
    MeshSlicer slicer;
    LineRasterizer rasterizer;
    PolygonFill scanConverter;
    RayTracer rayTracer;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll events
        glfwPollEvents();

        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render the mesh with proper transformations
        if (ui.isShowingMesh()) {
            // Set up camera view for mesh
            glm::mat4 view = ui.getMeshViewMatrix();
            glm::mat4 projection = ui.getMeshProjectionMatrix();
            
            // Apply to shader before rendering
            // This assumes you have a shader program for the mesh with uniforms
            // for the view and projection matrices
            GLuint meshShader = ui.getMeshShaderProgram();
            glUseProgram(meshShader);
            glUniformMatrix4fv(glGetUniformLocation(meshShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(meshShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            
            // Render the mesh
            mesh.render();
        }

        // Render UI
        ui.render(mesh, slicer, rasterizer, scanConverter, rayTracer);

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    glfwTerminate();
    return 0;
}