#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include <iostream>
#include <string>
#include <vector>

#include "OFFReader.h"
#include "mesh.h"
#include "slicer.h"
#include "rasterizer.h"
#include "scanline.h"
#include "raytracer.h"
#include "gui.h"

// Global variables
GLFWwindow* window;
int window_width = 1280;
int window_height = 720;
std::string model_path = "models/1grm.off";

ViewMode current_view = VIEW_3D;

// Application state
OffModel* off_model = nullptr;
Mesh* mesh = nullptr;
MeshSlicer* slicer = nullptr;
Rasterizer* rasterizer = nullptr;
ScanLineRenderer* scanline = nullptr;
RayTracer* raytracer = nullptr;
GUI* gui = nullptr;

// Camera state
float camera_pos[3] = {0.0f, 0.0f, 3.0f}; // Move a bit closer
float camera_rot[3] = {0.0f, 0.0f, 0.0f};
float camera_speed = 0.05f;
float mouse_sensitivity = 0.1f;
double last_mouse_x, last_mouse_y;
bool first_mouse = true;

// Camera vectors
glm::vec3 camera_front = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 camera_right = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);

// Camera control flags
bool camera_mouselook_enabled = true;

// Function prototypes
void init();
void update();
void render();
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cleanup();

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
    window = glfwCreateWindow(window_width, window_height, "Computer Graphics Assignment", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    
    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
    ImGui::StyleColorsDark();
    
    // Initialize application
    init();
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        update();
        render();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    cleanup();
    
    // Shutdown ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    // Terminate GLFW
    glfwTerminate();
    return 0;
}

void init() {
    // Load the mesh
    off_model = readOffFile(const_cast<char*>(model_path.c_str()));
    if (!off_model) {
        std::cerr << "Failed to load model: " << model_path << std::endl;
        glfwTerminate();
        exit(-1);
    }
    
    // Compute normals
    computeNormals(off_model);
    
    // Create application components
    mesh = new Mesh(off_model);
    slicer = new MeshSlicer(mesh);
    rasterizer = new Rasterizer(window_width, window_height);
    scanline = new ScanLineRenderer(window_width, window_height);
    raytracer = new RayTracer(window_width, window_height);
    gui = new GUI();
    
    // Initialize OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    
    // Initialize camera vectors based on initial rotation
    glm::vec3 front;
    front.x = cos(glm::radians(camera_rot[1])) * cos(glm::radians(camera_rot[0]));
    front.y = sin(glm::radians(camera_rot[0]));
    front.z = sin(glm::radians(camera_rot[1])) * cos(glm::radians(camera_rot[0]));
    camera_front = glm::normalize(front);
    camera_right = glm::normalize(glm::cross(camera_front, world_up));
    camera_up = glm::normalize(glm::cross(camera_right, camera_front));
    
    // Start with UI mode enabled instead of camera mode
    camera_mouselook_enabled = false;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void update() {
    // Check if we need to load a new mesh
    if (gui->loadMeshRequested) {
        // Load the new mesh
        if (!gui->meshPathToLoad.empty()) {
            // Clean up old model
            if (mesh) delete mesh;
            if (slicer) delete slicer;
            if (off_model) FreeOffModel(off_model);
            
            // Load new model
            model_path = gui->meshPathToLoad;
            off_model = readOffFile(const_cast<char*>(model_path.c_str()));
            
            if (off_model) {
                // Compute normals
                computeNormals(off_model);
                
                // Create new mesh and slicer
                mesh = new Mesh(off_model);
                slicer = new MeshSlicer(mesh);
                
                // Update mesh for ray tracing if in that view
                if (current_view == VIEW_RAYTRACE) {
                    // Clear existing scene and add the mesh
                    Material meshMaterial;
                    meshMaterial.color = glm::vec3(0.7f, 0.7f, 0.7f);
                    meshMaterial.reflectivity = 0.2f;
                    
                    raytracer->clearScene();
                    raytracer->addMesh(glm::vec3(0.0f), mesh, meshMaterial);
                    
                    // Make sure we have a light
                    if (raytracer->getLights().empty()) {
                        Light light(
                            glm::vec3(gui->lightPosition[0], gui->lightPosition[1], gui->lightPosition[2]),
                            glm::vec3(gui->lightColor[0], gui->lightColor[1], gui->lightColor[2]),
                            gui->lightIntensity
                        );
                        raytracer->addLight(light);
                    }
                    
                    // Force an update
                    raytracer->trace();
                }
            } else {
                std::cerr << "Failed to load model: " << model_path << std::endl;
            }
        }
        
        // Reset the flag
        gui->loadMeshRequested = false;
    }
    
    // Update based on current view
    switch (current_view) {
        case VIEW_3D:
            mesh->update();
            break;
            
        case VIEW_SLICE:
            slicer->update();
            break;
            
        case VIEW_RASTER:
            rasterizer->update();
            break;
            
        case VIEW_SCANLINE:
            scanline->update();
            break;
            
        case VIEW_RAYTRACE:
            // Synchronize raytracer camera with the "drone" camera
            glm::vec3 pos(camera_pos[0], camera_pos[1], camera_pos[2]);
            glm::vec3 look = pos + camera_front; // Where the OpenGL camera is looking
            glm::vec3 up = camera_up;
            raytracer->getCamera().setPosition(pos);
            raytracer->getCamera().setLookAt(look);
            raytracer->getCamera().setFOV(45.0f); // Or whatever your standard FOV is
            // Optionally, set aspect ratio as well
            raytracer->getCamera().setAspectRatio((float)window_width / (float)window_height);
            
            raytracer->update();
            break;
    }
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Render based on current view
    switch (current_view) {
        case VIEW_3D:
            mesh->render();
            break;
            
        case VIEW_SLICE:
            slicer->render();
            break;
            
        case VIEW_RASTER:
            rasterizer->render();
            break;
            
        case VIEW_SCANLINE:
            scanline->render();
            break;
            
        case VIEW_RAYTRACE:
            raytracer->render();
            break;
    }
    
    // Render GUI
    gui->render(&current_view, mesh, slicer, rasterizer, scanline, raytracer);
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Get camera position as glm::vec3 for easier manipulation
    glm::vec3 pos = glm::vec3(camera_pos[0], camera_pos[1], camera_pos[2]);
    
    // Camera movement - drone style
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        pos += camera_front * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        pos -= camera_front * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        pos -= camera_right * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        pos += camera_right * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        pos += camera_up * camera_speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        pos -= camera_up * camera_speed;
    
    // Update the camera position array
    camera_pos[0] = pos.x;
    camera_pos[1] = pos.y;
    camera_pos[2] = pos.z;
    
    // Toggle mouse-look when Tab is pressed
    static bool tab_pressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tab_pressed) {
            camera_mouselook_enabled = !camera_mouselook_enabled;
            glfwSetInputMode(window, GLFW_CURSOR, 
                             camera_mouselook_enabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            tab_pressed = true;
        }
    } else {
        tab_pressed = false;
    }
    
    // Fullscreen toggle with F11
    static bool f11_pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
        if (!f11_pressed) {
            static bool isFullscreen = false;
            isFullscreen = !isFullscreen;
            
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            
            if (isFullscreen) {
                // Switch to fullscreen
                glfwSetWindowMonitor(window, monitor, 0, 0, 
                                     mode->width, mode->height, 
                                     mode->refreshRate);
            } else {
                // Return to windowed mode
                glfwSetWindowMonitor(window, nullptr, 100, 100,
                                     window_width, window_height, 0);
            }
            
            f11_pressed = true;
        }
    } else {
        f11_pressed = false;
    }
    
    // Only process number keys for view switching if ImGui isn't capturing keyboard input
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        // View switching 
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
            current_view = VIEW_3D;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
            current_view = VIEW_SLICE;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
            current_view = VIEW_RASTER;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
            current_view = VIEW_SCANLINE;
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
            current_view = VIEW_RAYTRACE;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!camera_mouselook_enabled) {
        first_mouse = true;
        return;
    }
    
    if (first_mouse) {
        last_mouse_x = xpos;
        last_mouse_y = ypos;
        first_mouse = false;
    }
    
    float xoffset = (float)(xpos - last_mouse_x);
    float yoffset = (float)(last_mouse_y - ypos); // Reversed: y ranges bottom to top
    last_mouse_x = xpos;
    last_mouse_y = ypos;
    
    xoffset *= mouse_sensitivity;
    yoffset *= mouse_sensitivity;
    
    camera_rot[1] += xoffset; // Yaw
    camera_rot[0] += yoffset; // Pitch
    
    // Constrain pitch to avoid flipping
    if (camera_rot[0] > 89.0f)
        camera_rot[0] = 89.0f;
    if (camera_rot[0] < -89.0f)
        camera_rot[0] = -89.0f;
    
    // Update front, right and up vectors using the updated Euler angles
    glm::vec3 front;
    front.x = cos(glm::radians(camera_rot[1])) * cos(glm::radians(camera_rot[0]));
    front.y = sin(glm::radians(camera_rot[0]));
    front.z = sin(glm::radians(camera_rot[1])) * cos(glm::radians(camera_rot[0]));
    camera_front = glm::normalize(front);
    
    // Also re-calculate the Right and Up vector
    camera_right = glm::normalize(glm::cross(camera_front, world_up));
    camera_up = glm::normalize(glm::cross(camera_right, camera_front));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    window_width = width;
    window_height = height;
    glViewport(0, 0, width, height);
    
    // Update components with new sizes
    if (rasterizer) rasterizer->resize(width, height);
    if (scanline) scanline->resize(width, height);
    if (raytracer) raytracer->resize(width, height);
}

void cleanup() {
    // Clean up resources
    if (gui) delete gui;
    if (raytracer) delete raytracer;
    if (scanline) delete scanline;
    if (rasterizer) delete rasterizer;
    if (slicer) delete slicer;
    if (mesh) delete mesh;
    if (off_model) FreeOffModel(off_model);
}