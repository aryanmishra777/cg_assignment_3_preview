#include "interface.h"
#include "../mesh/mesh.h"
#include "../mesh/slicer.h"
#include "../rasterization/line.h"
#include "../scan_conversion/polygon_fill.h"
#include "../raytracing/raytracer.h"

#include "../../imgui/imgui.h"
#include "../../imgui/backends/imgui_impl_glfw.h"
#include "../../imgui/backends/imgui_impl_opengl3.h"
#include <iostream>
#include <glm/glm.hpp> // Make sure GLM is included here as well
#include <fstream>
#include <sstream>

UserInterface::UserInterface(GLFWwindow* window)
    : currentTab(0), numPlanes(1), addingPoint(false),
      imageWidth(800), imageHeight(600), 
      reflectionsEnabled(false), shadowsEnabled(true), showMesh(false),
      meshCameraPos{0.0f, 0.0f, 5.0f}, meshCameraTarget{0.0f, 0.0f, 0.0f}, 
      meshCameraUp{0.0f, 1.0f, 0.0f}, meshCameraFov(60.0f) {
    
    // Setup ImGui
    setupImGui(window);
    
    // Initialize file path buffer
    strcpy(filePathBuffer, "models/1grm.off");
    
    // Initialize line points
    linePoints[0][0] = 100; linePoints[0][1] = 100;
    linePoints[1][0] = 400; linePoints[1][1] = 300;
    
    // Initialize plane equations
    for (int i = 0; i < MAX_PLANES; i++) {
        planeEq[i][0] = 0.0f;  // a
        planeEq[i][1] = 0.0f;  // b
        planeEq[i][2] = 1.0f;  // c
        planeEq[i][3] = 0.0f;  // d
    }
    
    // Load shader for mesh rendering
    meshShaderProgram = createShaderProgram("shaders/mesh.vert", "shaders/mesh.frag");
}

UserInterface::~UserInterface() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UserInterface::setupImGui(GLFWwindow* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
}

void UserInterface::render(Mesh& mesh, MeshSlicer& slicer, LineRasterizer& rasterizer, 
                          PolygonFill& polygonFill, RayTracer& rayTracer) {
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Main window
    ImGui::Begin("Computer Graphics Assignment");
    
    // Tab bar
    if (ImGui::BeginTabBar("TabBar")) {
        // Mesh Slicing Tab
        if (ImGui::BeginTabItem("Mesh Slicing")) {
            renderMeshSlicingUI(mesh, slicer);
            ImGui::EndTabItem();
        }
        
        // Rasterization Tab
        if (ImGui::BeginTabItem("Line Rasterization")) {
            renderRasterizationUI(rasterizer);
            ImGui::EndTabItem();
        }
        
        // Scan Conversion Tab
        if (ImGui::BeginTabItem("Scan Conversion")) {
            renderScanConversionUI(polygonFill);
            ImGui::EndTabItem();
        }
        
        // Ray Tracing Tab
        if (ImGui::BeginTabItem("Ray Tracing")) {
            renderRayTracingUI(rayTracer);
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UserInterface::renderMeshSlicingUI(Mesh& mesh, MeshSlicer& slicer) {
    ImGui::Text("Load a mesh file and define planes to slice it.");
    
    // File selection
    ImGui::InputText("File Path", filePathBuffer, sizeof(filePathBuffer));
    ImGui::SameLine();
    if (ImGui::Button("Load Mesh")) {
        if (mesh.loadFromFile(filePathBuffer)) {
            slicer.setMesh(mesh);
            showMesh = true; // Set to show the mesh
            std::cout << "Mesh loaded successfully from " << filePathBuffer << std::endl;
        } else {
            std::cerr << "Failed to load mesh from " << filePathBuffer << std::endl;
        }
    }
    
    // Add drone camera controls for mesh viewing
    if (ImGui::CollapsingHeader("Camera Controls")) {
        ImGui::Text("Camera (Drone View):");
        ImGui::DragFloat3("Position", meshCameraPos, 0.1f);
        ImGui::DragFloat3("Target", meshCameraTarget, 0.1f);
        ImGui::DragFloat3("Up Vector", meshCameraUp, 0.1f);
        ImGui::SliderFloat("Field of View", &meshCameraFov, 30.0f, 120.0f);
    }
    
    // Number of planes slider
    ImGui::SliderInt("Number of Planes", &numPlanes, 1, 4);
    
    // Plane equations
    for (int i = 0; i < numPlanes; i++) {
        ImGui::PushID(i);
        ImGui::Text("Plane %d (ax + by + cz + d = 0):", i+1);
        
        ImGui::PushItemWidth(60);
        ImGui::InputFloat("a", &planeEq[i][0], 0.1f);
        ImGui::SameLine();
        ImGui::InputFloat("b", &planeEq[i][1], 0.1f);
        ImGui::SameLine();
        ImGui::InputFloat("c", &planeEq[i][2], 0.1f);
        ImGui::SameLine();
        ImGui::InputFloat("d", &planeEq[i][3], 0.1f);
        ImGui::PopItemWidth();
        
        ImGui::PopID();
    }
    
    // Apply slicing
    if (ImGui::Button("Apply Slicing")) {
        // Clear existing planes
        slicer.clearPlanes();
        
        // Add planes with equations from UI
        for (int i = 0; i < numPlanes; i++) {
            Plane plane(planeEq[i][0], planeEq[i][1], planeEq[i][2], planeEq[i][3]);
            slicer.addPlane(plane);
        }
        
        // Perform slicing
        Mesh slicedMesh = slicer.sliceMesh();
        
        // Replace the original mesh with the sliced one
        mesh = slicedMesh;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        slicer.clearPlanes();
    }
}

void UserInterface::renderRasterizationUI(LineRasterizer& rasterizer) {
    ImGui::Text("Define a line by its endpoints to rasterize it.");
    
    // Line start and end points
    ImGui::Text("Start Point:");
    ImGui::PushItemWidth(100);
    ImGui::InputInt("X1", &linePoints[0][0]);
    ImGui::SameLine();
    ImGui::InputInt("Y1", &linePoints[0][1]);
    
    ImGui::Text("End Point:");
    ImGui::InputInt("X2", &linePoints[1][0]);
    ImGui::SameLine();
    ImGui::InputInt("Y2", &linePoints[1][1]);
    ImGui::PopItemWidth();
    
    // Rasterize button
    if (ImGui::Button("Rasterize Line")) {
        // Rasterize the line
        std::vector<Pixel> pixels = rasterizer.rasterizeLine(
            linePoints[0][0], linePoints[0][1],
            linePoints[1][0], linePoints[1][1]
        );
        
        // Display the rasterized line
        rasterizer.renderPixels(pixels);
    }
    
    // Visualization area
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    ImVec2 canvasSize(std::min(windowSize.x, 500.0f), std::min(windowSize.y, 400.0f));
    
    ImGui::BeginChild("LineCanvas", canvasSize, true);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasEnd = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
    
    // Draw canvas background
    drawList->AddRectFilled(canvasPos, canvasEnd, IM_COL32(50, 50, 50, 255));
    
    // Draw line
    drawList->AddLine(
        ImVec2(canvasPos.x + linePoints[0][0], canvasPos.y + linePoints[0][1]),
        ImVec2(canvasPos.x + linePoints[1][0], canvasPos.y + linePoints[1][1]),
        IM_COL32(255, 255, 255, 255),
        2.0f
    );
    
    ImGui::EndChild();
}

void UserInterface::renderScanConversionUI(PolygonFill& polygonFill) {
    ImGui::Text("Define a polygon and fill it using scan-line algorithm.");
    
    // Polygon definition
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    ImVec2 canvasSize(std::min(windowSize.x, 500.0f), std::min(windowSize.y, 400.0f));
    
    ImGui::BeginChild("PolygonCanvas", canvasSize, true);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasEnd = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
    
    // Draw canvas background
    drawList->AddRectFilled(canvasPos, canvasEnd, IM_COL32(50, 50, 50, 255));
    
    // Handle mouse input for adding points
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isMouseInCanvas = mousePos.x >= canvasPos.x && mousePos.x <= canvasEnd.x &&
                           mousePos.y >= canvasPos.y && mousePos.y <= canvasEnd.y;
    
    if (isMouseInCanvas && ImGui::IsMouseClicked(0)) {
        glm::vec2 newPoint(mousePos.x - canvasPos.x, mousePos.y - canvasPos.y);
        polygonPoints.push_back(newPoint);
    }
    
    // Draw polygon points and lines
    if (!polygonPoints.empty()) {
        for (size_t i = 0; i < polygonPoints.size(); i++) {
            // Draw point
            drawList->AddCircleFilled(
                ImVec2(canvasPos.x + polygonPoints[i].x, canvasPos.y + polygonPoints[i].y),
                5.0f,
                IM_COL32(255, 255, 0, 255)
            );
            
            // Draw line to next point
            if (i > 0) {
                drawList->AddLine(
                    ImVec2(canvasPos.x + polygonPoints[i-1].x, canvasPos.y + polygonPoints[i-1].y),
                    ImVec2(canvasPos.x + polygonPoints[i].x, canvasPos.y + polygonPoints[i].y),
                    IM_COL32(255, 255, 255, 255),
                    2.0f
                );
            }
        }
        
        // Connect last point to first point if we have at least 3 points
        if (polygonPoints.size() >= 3) {
            drawList->AddLine(
                ImVec2(canvasPos.x + polygonPoints.back().x, canvasPos.y + polygonPoints.back().y),
                ImVec2(canvasPos.x + polygonPoints.front().x, canvasPos.y + polygonPoints.front().y),
                IM_COL32(255, 255, 255, 255),
                2.0f
            );
        }
    }
    
    ImGui::EndChild();
    
    // Control buttons
    if (ImGui::Button("Fill Polygon")) {
        if (polygonPoints.size() >= 3) {
            polygonFill.setPolygon(polygonPoints);
            std::vector<Pixel> filledPixels = polygonFill.fillPolygon();
            polygonFill.renderFilledPolygon(filledPixels);
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear Polygon")) {
        polygonPoints.clear();
        polygonFill.clear();
    }
    
    ImGui::Text("Click in the canvas to add polygon vertices.");
    ImGui::Text("Current polygon has %zu vertices.", polygonPoints.size());
}

void UserInterface::renderRayTracingUI(RayTracer& rayTracer) {
    ImGui::Text("Configure ray tracing parameters and render the scene.");
    
    // Image dimensions
    ImGui::PushItemWidth(100);
    ImGui::InputInt("Width", &imageWidth);
    ImGui::SameLine();
    ImGui::InputInt("Height", &imageHeight);
    ImGui::PopItemWidth();
    
    // Ray tracing options
    ImGui::Checkbox("Enable Shadows", &shadowsEnabled);
    ImGui::Checkbox("Enable Reflections", &reflectionsEnabled);
    
    // Scene setup
    if (ImGui::CollapsingHeader("Scene Setup")) {
        // Camera settings
        static float cameraPos[3] = { 0.0f, 0.0f, 5.0f };
        static float cameraTarget[3] = { 0.0f, 0.0f, 0.0f };
        static float cameraFov = 60.0f;
        
        ImGui::Text("Camera:");
        ImGui::DragFloat3("Position", cameraPos, 0.1f);
        ImGui::DragFloat3("Target", cameraTarget, 0.1f);
        ImGui::SliderFloat("Field of View", &cameraFov, 30.0f, 120.0f);
        
        // Add simple objects
        static bool addSphere = false;
        static bool addBox = false;
        
        ImGui::Separator();
        ImGui::Text("Add Objects:");
        
        if (ImGui::Button("Add Sphere")) {
            addSphere = true;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Add Box")) {
            addBox = true;
        }
        
        // Handle object addition
        if (addSphere) {
            static float spherePos[3] = { 0.0f, 0.0f, 0.0f };
            static float sphereRadius = 1.0f;
            static float sphereColor[3] = { 1.0f, 0.0f, 0.0f };
            static float sphereReflectivity = 0.0f;
            
            ImGui::Begin("Add Sphere", &addSphere);
            
            ImGui::DragFloat3("Position", spherePos, 0.1f);
            ImGui::SliderFloat("Radius", &sphereRadius, 0.1f, 5.0f);
            ImGui::ColorEdit3("Color", sphereColor);
            ImGui::SliderFloat("Reflectivity", &sphereReflectivity, 0.0f, 1.0f);
            
            if (ImGui::Button("Add to Scene")) {
                // Create and add sphere to the scene
                auto sphere = std::make_unique<Sphere>(
                    glm::vec3(spherePos[0], spherePos[1], spherePos[2]),
                    sphereRadius
                );
                
                Material material;
                material.color = glm::vec3(sphereColor[0], sphereColor[1], sphereColor[2]);
                material.reflectivity = sphereReflectivity;
                sphere->setMaterial(material);
                
                rayTracer.getScene().addPrimitive(std::move(sphere));
                
                addSphere = false;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                addSphere = false;
            }
            
            ImGui::End();
        }
        
        if (addBox) {
            static float boxMin[3] = { -1.0f, -1.0f, -1.0f };
            static float boxMax[3] = { 1.0f, 1.0f, 1.0f };
            static float boxColor[3] = { 0.0f, 1.0f, 0.0f };
            static float boxReflectivity = 0.0f;
            
            ImGui::Begin("Add Box", &addBox);
            
            ImGui::DragFloat3("Min Corner", boxMin, 0.1f);
            ImGui::DragFloat3("Max Corner", boxMax, 0.1f);
            ImGui::ColorEdit3("Color", boxColor);
            ImGui::SliderFloat("Reflectivity", &boxReflectivity, 0.0f, 1.0f);
            
            if (ImGui::Button("Add to Scene")) {
                // Create and add box to the scene
                auto box = std::make_unique<Box>(
                    glm::vec3(boxMin[0], boxMin[1], boxMin[2]),
                    glm::vec3(boxMax[0], boxMax[1], boxMax[2])
                );
                
                Material material;
                material.color = glm::vec3(boxColor[0], boxColor[1], boxColor[2]);
                material.reflectivity = boxReflectivity;
                box->setMaterial(material);
                
                rayTracer.getScene().addPrimitive(std::move(box));
                
                addBox = false;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                addBox = false;
            }
            
            ImGui::End();
        }
        
        // Add lights
        static bool addLight = false;
        
        ImGui::Separator();
        ImGui::Text("Lights:");
        
        if (ImGui::Button("Add Light")) {
            addLight = true;
        }
        
        if (addLight) {
            static float lightPos[3] = { 5.0f, 5.0f, 5.0f };
            static float lightColor[3] = { 1.0f, 1.0f, 1.0f };
            static float lightIntensity = 1.0f;
            
            ImGui::Begin("Add Light", &addLight);
            
            ImGui::DragFloat3("Position", lightPos, 0.1f);
            ImGui::ColorEdit3("Color", lightColor);
            ImGui::SliderFloat("Intensity", &lightIntensity, 0.1f, 5.0f);
            
            if (ImGui::Button("Add to Scene")) {
                // Create and add light to the scene
                Light light(
                    glm::vec3(lightPos[0], lightPos[1], lightPos[2]),
                    glm::vec3(lightColor[0], lightColor[1], lightColor[2]),
                    lightIntensity
                );
                
                rayTracer.getScene().addLight(light);
                
                addLight = false;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                addLight = false;
            }
            
            ImGui::End();
        }
        
        // Update camera
        Camera camera(
            glm::vec3(cameraPos[0], cameraPos[1], cameraPos[2]),
            glm::vec3(cameraTarget[0], cameraTarget[1], cameraTarget[2]),
            glm::vec3(0.0f, 1.0f, 0.0f),
            cameraFov,
            static_cast<float>(imageWidth) / imageHeight
        );
        
        rayTracer.getScene().setCamera(camera);
    }
    
    // Render button
    if (ImGui::Button("Render")) {
        // Update ray tracer settings
        rayTracer.setDimensions(imageWidth, imageHeight);
        rayTracer.enableShadows(shadowsEnabled);
        rayTracer.enableReflections(reflectionsEnabled);
        
        // Render the scene
        rayTracer.render();
    }
    
    // Display the ray-traced image
    ImGui::Separator();
    ImGui::Text("Rendered Image:");
    
    rayTracer.displayImage();
}

void UserInterface::showHelpMarker(const std::string& desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

unsigned int UserInterface::createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
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

std::string UserInterface::loadShaderSource(const std::string& path) {
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