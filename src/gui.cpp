#include "gui.h"
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h> // Add GLFW header
#include <glm/glm.hpp>
#include <string>

// External camera variables from main.cpp
extern float camera_pos[3];
extern glm::vec3 camera_front;
extern glm::vec3 camera_up;
extern glm::vec3 camera_right; // Add this
extern glm::vec3 world_up;     // Add this
extern GLFWwindow* window;     // Add this

GUI::GUI() {
    // Initialize GUI state
    showDemoWindow = false;
    showMetricsWindow = false;
    showAppMainMenuBar = true;
}

void GUI::render(ViewMode* currentView, Mesh* mesh, MeshSlicer* slicer, 
                Rasterizer* rasterizer, ScanLineRenderer* scanline, 
                RayTracer* raytracer) {
    // Main menu bar at the top of the window
    if (showAppMainMenuBar) {
        renderMainMenuBar();
    }
    
    // Main control panel
    ImGui::Begin("Computer Graphics Tools");
    
    // View selection
    const char* viewNames[] = { "3D View", "Mesh Slicing", "Line Rasterization", "Scan-line Fill", "Ray Tracing" };
    int currentViewIndex = static_cast<int>(*currentView);
    if (ImGui::Combo("View Mode", &currentViewIndex, viewNames, IM_ARRAYSIZE(viewNames))) {
        *currentView = static_cast<ViewMode>(currentViewIndex);
    }
    
    // Divider
    ImGui::Separator();
    
    // View-specific controls
    switch (*currentView) {
        case VIEW_SLICE:
            renderSlicingControls(slicer);
            break;
            
        case VIEW_RASTER:
            renderRasterizationControls(rasterizer, 1280, 720, currentView);
            break;
            
        case VIEW_SCANLINE:
            renderScanConversionControls(scanline, 1280, 720);
            break;
            
        case VIEW_RAYTRACE:
            renderRayTracingControls(raytracer, mesh);
            break;
            
        default: // VIEW_3D
            // Basic mesh controls
            float position[3] = {
                mesh->getPosition().x,
                mesh->getPosition().y,
                mesh->getPosition().z
            };
            
            float rotation[3] = {
                mesh->getRotation().x,
                mesh->getRotation().y,
                mesh->getRotation().z
            };
            
            float scale[3] = {
                mesh->getScale().x,
                mesh->getScale().y,
                mesh->getScale().z
            };
            
            if (ImGui::DragFloat3("Position", position, 0.1f)) {
                mesh->setPosition(glm::vec3(position[0], position[1], position[2]));
            }
            
            if (ImGui::DragFloat3("Rotation", rotation, 1.0f)) {
                mesh->setRotation(glm::vec3(rotation[0], rotation[1], rotation[2]));
            }
            
            if (ImGui::DragFloat3("Scale", scale, 0.1f, 0.1f, 10.0f)) {
                mesh->setScale(glm::vec3(scale[0], scale[1], scale[2]));
            }
            
            break;
    }
    
    // Add a help section
    ImGui::Separator();
    ImGui::Text("Drone Camera Controls:");
    ImGui::BulletText("Tab: Toggle between camera mode and UI mode");
    ImGui::BulletText("W/S: Move forward/backward");
    ImGui::BulletText("A/D: Strafe left/right");
    ImGui::BulletText("Q/E: Move up/down");
    ImGui::BulletText("Mouse: Look around (in camera mode)");
    ImGui::BulletText("1-5: Switch between view modes");
    
    // Add coordinate axes debug info
    ImGui::Separator();
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", camera_pos[0], camera_pos[1], camera_pos[2]);
    ImGui::Text("Looking at: (%.1f, %.1f, %.1f)", 
        camera_pos[0] + camera_front.x, 
        camera_pos[1] + camera_front.y, 
        camera_pos[2] + camera_front.z);
    
    ImGui::End();
    
    // Optionally show ImGui demo window for development
    if (showDemoWindow) {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
    
    // Optionally show metrics window
    if (showMetricsWindow) {
        ImGui::ShowMetricsWindow(&showMetricsWindow);
    }
}

void GUI::renderMainMenuBar() {
    static bool isFullscreen = false;
    
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            // Add mesh loading option in the File menu
            if (ImGui::MenuItem("Load Mesh...", "Ctrl+O")) {
                ImGui::OpenPopup("Load Mesh File");
            }
            
            if (ImGui::MenuItem("Exit", "Esc")) {
                // Signal to close the application
                // This needs to be handled in the main application
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Toggle Fullscreen", "F11")) {
                isFullscreen = !isFullscreen;
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                
                if (isFullscreen) {
                    // Save current window position and size
                    glfwGetWindowPos(window, &windowPosX, &windowPosY);
                    glfwGetWindowSize(window, &windowWidth, &windowHeight);
                    
                    // Switch to fullscreen
                    glfwSetWindowMonitor(window, monitor, 0, 0, 
                                        mode->width, mode->height, 
                                        mode->refreshRate);
                } else {
                    // Return to windowed mode with previous size
                    glfwSetWindowMonitor(window, nullptr, windowPosX, windowPosY,
                                        windowWidth, windowHeight, 0);
                }
            }
            
            ImGui::MenuItem("Show Demo Window", NULL, &showDemoWindow);
            ImGui::MenuItem("Show Metrics", NULL, &showMetricsWindow);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // Show about dialog
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    // Render the mesh loading dialog when open
    renderMeshLoadingDialog();
}

void GUI::renderMeshLoadingDialog() {
    // Define the available mesh files
    static const char* meshFiles[] = {
        "models/1grm.off",
        "models/cube.off",
        "models/teapot.off",
        "models/bunny.off",
        "models/sphere.off"
    };
    static int selectedMeshIndex = 0;
    static char customMeshPath[256] = "";
    
    if (ImGui::BeginPopupModal("Load Mesh File", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Select a mesh file to load:");
        
        // Predefined mesh files
        ImGui::Combo("Available Models", &selectedMeshIndex, meshFiles, IM_ARRAYSIZE(meshFiles));
        
        // Option for custom path
        ImGui::Separator();
        ImGui::Text("Or enter a custom path:");
        ImGui::InputText("Mesh Path", customMeshPath, 256);
        
        ImGui::Separator();
        
        if (ImGui::Button("Load Selected Model", ImVec2(150, 0))) {
            // Use the selected model path
            loadMeshRequested = true;
            meshPathToLoad = meshFiles[selectedMeshIndex];
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Load Custom Path", ImVec2(150, 0))) {
            if (strlen(customMeshPath) > 0) {
                loadMeshRequested = true;
                meshPathToLoad = customMeshPath;
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void GUI::renderSlicingControls(MeshSlicer* slicer) {
    ImGui::Text("Mesh Slicing Controls");
    ImGui::Text("Slice a mesh with 1-4 arbitrary planes");
    
    // Number of planes slider
    if (ImGui::SliderInt("Number of Planes", &numPlanes, 1, 4)) {
        // Ensure we have the correct number of planes in the slicer
        while (slicer->getPlaneCount() < numPlanes) {
            // Add default planes
            Plane plane;
            if (slicer->getPlaneCount() == 0) {
                plane = Plane(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f); // XZ plane
            } else if (slicer->getPlaneCount() == 1) {
                plane = Plane(glm::vec3(1.0f, 0.0f, 0.0f), 0.0f); // YZ plane
            } else if (slicer->getPlaneCount() == 2) {
                plane = Plane(glm::vec3(0.0f, 0.0f, 1.0f), 0.0f); // XY plane
            } else {
                plane = Plane(glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)), 0.0f); // Diagonal plane
            }
            slicer->addPlane(plane);
        }
        
        while (slicer->getPlaneCount() > numPlanes) {
            slicer->removePlane(slicer->getPlaneCount() - 1);
        }
    }
    
    // Plane selection
    if (ImGui::SliderInt("Active Plane", &activePlaneIndex, 0, numPlanes - 1)) {
        slicer->setActivePlane(activePlaneIndex);
    }
    
    // Plane parameters for the active plane
    if (activePlaneIndex < numPlanes) {
        // Get the current plane
        Plane currentPlane = slicer->getPlane(activePlaneIndex);
        
        // Set the UI values to match the current plane
        planeNormal[activePlaneIndex][0] = currentPlane.normal.x;
        planeNormal[activePlaneIndex][1] = currentPlane.normal.y;
        planeNormal[activePlaneIndex][2] = currentPlane.normal.z;
        planeDistance[activePlaneIndex] = currentPlane.distance;
        
        // Allow editing of the plane parameters
        bool paramsChanged = false;
        
        if (ImGui::DragFloat3("Normal", planeNormal[activePlaneIndex], 0.01f, -1.0f, 1.0f)) {
            paramsChanged = true;
        }
        
        if (ImGui::DragFloat("Distance", &planeDistance[activePlaneIndex], 0.1f, -10.0f, 10.0f)) {
            paramsChanged = true;
        }
        
        // Update the plane if parameters were changed
        if (paramsChanged) {
            // Normalize the normal vector
            glm::vec3 normal = glm::normalize(glm::vec3(
                planeNormal[activePlaneIndex][0],
                planeNormal[activePlaneIndex][1],
                planeNormal[activePlaneIndex][2]
            ));
            
            // Update the plane
            Plane newPlane(normal, planeDistance[activePlaneIndex]);
            slicer->updatePlane(activePlaneIndex, newPlane);
        }
    }
    
    // Display all current plane equations with colors
    ImGui::Separator();
    ImGui::Text("All Planes:");
    
    const char* planeNames[] = { "Plane 1 (Red)", "Plane 2 (Green)", "Plane 3 (Blue)", "Plane 4 (Yellow)" };
    ImVec4 planeColors[] = {
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
        ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
        ImVec4(0.0f, 0.0f, 1.0f, 1.0f),
        ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
    };
    
    for (int i = 0; i < numPlanes; i++) {
        Plane plane = slicer->getPlane(i);
        float a = plane.normal.x;
        float b = plane.normal.y;
        float c = plane.normal.z;
        float d = -plane.distance;
        
        ImGui::TextColored(planeColors[i], "%s: %.2fx + %.2fy + %.2fz + %.2f = 0", 
                           planeNames[i], a, b, c, d);
        
        // Highlight active plane
        if (i == activePlaneIndex) {
            ImGui::SameLine();
            ImGui::Text(" (Active)");
        }
    }
    
}

void GUI::renderRasterizationControls(Rasterizer* rasterizer, int width, int height, ViewMode* currentView) {
    ImGui::Text("Line Rasterization Controls");
    ImGui::Text("Draw lines using Bresenham's algorithm");
    
    // Convert normalized coordinates to pixels
    int startX = static_cast<int>(lineStart[0] * width);
    int startY = static_cast<int>(lineStart[1] * height);
    int endX = static_cast<int>(lineEnd[0] * width);
    int endY = static_cast<int>(lineEnd[1] * height);
    
    // Update from current rasterizer state
    glm::vec2 currentStart = rasterizer->getStartPoint();
    glm::vec2 currentEnd = rasterizer->getEndPoint();
    
    startX = static_cast<int>(currentStart.x);
    startY = static_cast<int>(currentStart.y);
    endX = static_cast<int>(currentEnd.x);
    endY = static_cast<int>(currentEnd.y);
    
    // Convert back to normalized for UI
    lineStart[0] = static_cast<float>(startX) / width;
    lineStart[1] = static_cast<float>(startY) / height;
    lineEnd[0] = static_cast<float>(endX) / width;
    lineEnd[1] = static_cast<float>(endY) / height;
    
    // Line controls
    bool paramsChanged = false;
    
    if (ImGui::DragFloat2("Start Point", lineStart, 0.01f, 0.0f, 1.0f)) {
        paramsChanged = true;
    }
    
    if (ImGui::DragFloat2("End Point", lineEnd, 0.01f, 0.0f, 1.0f)) {
        paramsChanged = true;
    }
    
    if (ImGui::ColorEdit3("Line Color", lineColor)) {
        paramsChanged = true;
    }
    
    if (ImGui::Button("Reset Line")) {
        // Reset to default line
        lineStart[0] = 0.25f; lineStart[1] = 0.5f;
        lineEnd[0] = 0.75f; lineEnd[1] = 0.5f;
        lineColor[0] = 1.0f; lineColor[1] = 0.0f; lineColor[2] = 0.0f;
        
        // Apply changes
        int pixelStartX = static_cast<int>(lineStart[0] * width);
        int pixelStartY = static_cast<int>(lineStart[1] * height);
        int pixelEndX = static_cast<int>(lineEnd[0] * width);
        int pixelEndY = static_cast<int>(lineEnd[1] * height);
        
        rasterizer->setStartPoint(glm::vec2(pixelStartX, pixelStartY));
        rasterizer->setEndPoint(glm::vec2(pixelEndX, pixelEndY));
        rasterizer->setLineColor(glm::vec3(lineColor[0], lineColor[1], lineColor[2]));
        rasterizer->clear();
        rasterizer->update();
    }
    
    // Update the line if parameters were changed
    if (paramsChanged) {
        // Convert normalized coordinates back to pixels
        int pixelStartX = static_cast<int>(lineStart[0] * width);
        int pixelStartY = static_cast<int>(lineStart[1] * height);
        int pixelEndX = static_cast<int>(lineEnd[0] * width);
        int pixelEndY = static_cast<int>(lineEnd[1] * height);
        
        // Update line parameters
        rasterizer->setStartPoint(glm::vec2(pixelStartX, pixelStartY));
        rasterizer->setEndPoint(glm::vec2(pixelEndX, pixelEndY));
        rasterizer->setLineColor(glm::vec3(lineColor[0], lineColor[1], lineColor[2]));
        
        // Important: Call update after changing parameters
        rasterizer->update();
    }

    ImGui::Separator();
    if (ImGui::Button("Focus on Line")) {
        // Reset to a large, centered, high-contrast line
        lineStart[0] = 0.25f; lineStart[1] = 0.5f;
        lineEnd[0] = 0.75f; lineEnd[1] = 0.5f;
        
        // Very bright neon green line on black background for maximum contrast
        lineColor[0] = 0.0f; lineColor[1] = 1.0f; lineColor[2] = 0.0f;
        
        // Apply changes
        int pixelStartX = static_cast<int>(lineStart[0] * width);
        int pixelStartY = static_cast<int>(lineStart[1] * height);
        int pixelEndX = static_cast<int>(lineEnd[0] * width);
        int pixelEndY = static_cast<int>(lineEnd[1] * height);
        
        rasterizer->setStartPoint(glm::vec2(pixelStartX, pixelStartY));
        rasterizer->setEndPoint(glm::vec2(pixelEndX, pixelEndY));
        rasterizer->setLineColor(glm::vec3(lineColor[0], lineColor[1], lineColor[2]));
        
        // Clear with black background for maximum contrast
        rasterizer->clear(glm::vec3(0.0f, 0.0f, 0.0f));
        
        // Force update
        rasterizer->update();
        
        // Make sure we're in rasterization view mode
        *currentView = VIEW_RASTER;
        
        // Display a message to confirm the action
        ImGui::OpenPopup("Line Reset");
    }

    if (ImGui::BeginPopupModal("Line Reset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Line has been reset to center of screen with high contrast colors.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void GUI::renderScanConversionControls(ScanLineRenderer* scanline, int width, int height) {
    ImGui::Text("Scan-line Polygon Fill Controls");
    ImGui::Text("Fill polygons using scan-line algorithm");
    
    // Polygon controls
    ImGui::SliderInt("Number of Vertices", &numPolygonVertices, 3, 10);
    
    // Ensure we have a valid number of vertices
    numPolygonVertices = std::max(3, std::min(numPolygonVertices, 10));
    
    // Vertex controls
    bool verticesChanged = false;
    
    ImGui::Text("Polygon Vertices (normalized coordinates):");
    for (int i = 0; i < numPolygonVertices; i++) {
        char label[32];
        sprintf(label, "Vertex %d", i + 1);
        if (ImGui::DragFloat2(label, polygonVertices[i], 0.01f, 0.0f, 1.0f)) {
            verticesChanged = true;
        }
    }
    
    // Fill color
    if (ImGui::ColorEdit3("Fill Color", fillColor)) {
        scanline->setFillColor(glm::vec3(fillColor[0], fillColor[1], fillColor[2]));
    }
    
    // Update polygon if vertices changed
    if (verticesChanged) {
        // Clear the current polygon
        scanline->clearPolygon();
        
        // Add the vertices (converting from normalized to pixel coordinates)
        for (int i = 0; i < numPolygonVertices; i++) {
            int x = static_cast<int>(polygonVertices[i][0] * width);
            int y = static_cast<int>(polygonVertices[i][1] * height);
            scanline->addVertex(glm::vec2(x, y));
        }
    }
    
    // Button to reset to a rectangle
    if (ImGui::Button("Reset to Rectangle")) {
        numPolygonVertices = 4;
        polygonVertices[0][0] = 0.3f; polygonVertices[0][1] = 0.3f;
        polygonVertices[1][0] = 0.7f; polygonVertices[1][1] = 0.3f;
        polygonVertices[2][0] = 0.7f; polygonVertices[2][1] = 0.7f;
        polygonVertices[3][0] = 0.3f; polygonVertices[3][1] = 0.7f;
        
        // Update the scanline renderer
        scanline->clearPolygon();
        for (int i = 0; i < numPolygonVertices; i++) {
            int x = static_cast<int>(polygonVertices[i][0] * width);
            int y = static_cast<int>(polygonVertices[i][1] * height);
            scanline->addVertex(glm::vec2(x, y));
        }
    }

    if (ImGui::Button("Apply Changes & Update")) {
        // Clear the current polygon
        scanline->clearPolygon();
        
        // Add the vertices (converting from normalized to pixel coordinates)
        for (int i = 0; i < numPolygonVertices; i++) {
            int x = static_cast<int>(polygonVertices[i][0] * width);
            int y = static_cast<int>(polygonVertices[i][1] * height);
            scanline->addVertex(glm::vec2(x, y));
        }
        
        // Force a scan-line fill update
        scanline->clear(glm::vec3(0.1f, 0.1f, 0.1f)); // Dark background
        scanline->update();
    }

    ImGui::SameLine();
    if (ImGui::Button("Draw Star")) {
        // Create a star shape
        numPolygonVertices = 10;
        float centerX = 0.5f;
        float centerY = 0.5f;
        float outerRadius = 0.3f;
        float innerRadius = 0.15f;
        
        for (int i = 0; i < numPolygonVertices; i++) {
            float angle = static_cast<float>(i) * (2.0f * 3.14159f / numPolygonVertices);
            float radius = (i % 2 == 0) ? outerRadius : innerRadius;
            
            polygonVertices[i][0] = centerX + radius * std::cos(angle);
            polygonVertices[i][1] = centerY + radius * std::sin(angle);
        }
        
        // Update the scanline renderer
        scanline->clearPolygon();
        for (int i = 0; i < numPolygonVertices; i++) {
            int x = static_cast<int>(polygonVertices[i][0] * width);
            int y = static_cast<int>(polygonVertices[i][1] * height);
            scanline->addVertex(glm::vec2(x, y));
        }
        
        // Force update
        scanline->clear(glm::vec3(0.1f, 0.1f, 0.1f));
        scanline->update();
    }
}

void GUI::renderRayTracingControls(RayTracer* raytracer, Mesh* mesh) {
    ImGui::Text("Ray Tracing Controls");
    
    // Ray tracing parameters
    if (ImGui::SliderInt("Max Recursion Depth", &maxDepth, 1, 10)) {
        raytracer->setMaxDepth(maxDepth);
    }
    
    if (ImGui::Checkbox("Enable Shadows", &enableShadows)) {
        raytracer->setEnableShadows(enableShadows);
    }
    
    if (ImGui::Checkbox("Enable Reflections", &enableReflections)) {
        raytracer->setEnableReflections(enableReflections);
    }
    
    // Scene objects
    if (ImGui::CollapsingHeader("Scene Objects")) {
        // Add a button to add the current mesh to the scene
        if (ImGui::Button("Add Current Mesh to Scene")) {
            Material meshMaterial;
            meshMaterial.color = glm::vec3(0.7f, 0.7f, 0.7f);
            meshMaterial.reflectivity = 0.2f;
            
            // Position it at the origin
            raytracer->addMesh(glm::vec3(0.0f), mesh, meshMaterial);
            
            // Make sure we have a light
            if (raytracer->getLights().empty()) {
                Light light(
                    glm::vec3(lightPosition[0], lightPosition[1], lightPosition[2]),
                    glm::vec3(lightColor[0], lightColor[1], lightColor[2]),
                    lightIntensity
                );
                raytracer->addLight(light);
            }
            
            // Force an update
            raytracer->trace();
        }
        
        // Add tab control for different object types
        static int objectType = 0;
        ImGui::Text("Object Type:");
        ImGui::SameLine();
        if (ImGui::RadioButton("Sphere", &objectType, 0)) {}
        ImGui::SameLine();
        if (ImGui::RadioButton("Cube", &objectType, 1)) {}

        // Sphere controls
        if (objectType == 0) {
            ImGui::Text("Sphere Parameters:");
            bool sphereChanged = false;
            
            if (ImGui::DragFloat3("Position", spherePosition, 0.1f)) {
                sphereChanged = true;
            }
            
            if (ImGui::DragFloat("Radius", &sphereRadius, 0.1f, 0.1f, 10.0f)) {
                sphereChanged = true;
            }
            
            if (ImGui::ColorEdit3("Color", sphereColor)) {
                sphereChanged = true;
            }
            
            if (sphereChanged) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Parameters changed, click Add Sphere to create");
            }
            
            // Add sphere button
            if (ImGui::Button("Add Sphere")) {
                // Create material
                Material sphereMaterial;
                sphereMaterial.color = glm::vec3(sphereColor[0], sphereColor[1], sphereColor[2]);
                sphereMaterial.reflectivity = 0.3f; // Default reflectivity
                
                // Add the sphere (without clearing scene)
                raytracer->addSphere(
                    glm::vec3(spherePosition[0], spherePosition[1], spherePosition[2]),
                    sphereRadius,
                    sphereMaterial
                );
                
                // Make sure we have a light
                if (raytracer->getLights().empty()) {
                    Light light(
                        glm::vec3(lightPosition[0], lightPosition[1], lightPosition[2]),
                        glm::vec3(lightColor[0], lightColor[1], lightColor[2]),
                        lightIntensity
                    );
                    raytracer->addLight(light);
                }
                
                // Update the scene
                raytracer->trace();
            }
        }
        // Cube controls
        else if (objectType == 1) {
            ImGui::Text("Cube Parameters:");
            bool cubeChanged = false;
            
            if (ImGui::DragFloat3("Position", cubePosition, 0.1f)) {
                cubeChanged = true;
            }
            
            if (ImGui::DragFloat3("Size", cubeSize, 0.1f, 0.1f, 10.0f)) {
                cubeChanged = true;
            }
            
            if (ImGui::ColorEdit3("Color", cubeColor)) {
                cubeChanged = true;
            }
            
            if (cubeChanged) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Parameters changed, click Add Cube to create");
            }
            
            // Add cube button
            if (ImGui::Button("Add Cube")) {
                // Create material
                Material cubeMaterial;
                cubeMaterial.color = glm::vec3(cubeColor[0], cubeColor[1], cubeColor[2]);
                cubeMaterial.reflectivity = 0.2f; // Default reflectivity
                
                // Add the cube (without clearing scene)
                raytracer->addCube(
                    glm::vec3(cubePosition[0], cubePosition[1], cubePosition[2]),
                    glm::vec3(cubeSize[0], cubeSize[1], cubeSize[2]),
                    cubeMaterial
                );
                
                // Make sure we have a light
                if (raytracer->getLights().empty()) {
                    Light light(
                        glm::vec3(lightPosition[0], lightPosition[1], lightPosition[2]),
                        glm::vec3(lightColor[0], lightColor[1], lightColor[2]),
                        lightIntensity
                    );
                    raytracer->addLight(light);
                }
                
                // Update the scene
                raytracer->trace();
            }
        }
    }
    
    // Object viewer
    if (ImGui::CollapsingHeader("Scene Viewer")) {
        ImGui::Text("Objects in scene: %d", static_cast<int>(raytracer->getObjects().size()));
        ImGui::Text("Lights in scene: %d", static_cast<int>(raytracer->getLights().size()));
        
        if (ImGui::Button("Clear All Objects")) {
            raytracer->clearScene();
            ImGui::OpenPopup("Scene Cleared");
        }
        
        if (ImGui::BeginPopupModal("Scene Cleared", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("All objects and lights have been removed from the scene.");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        // List all objects
        if (!raytracer->getObjects().empty()) {
            ImGui::Separator();
            ImGui::Text("Object List:");
            for (size_t i = 0; i < raytracer->getObjects().size(); i++) {
                auto& obj = raytracer->getObjects()[i];
                auto pos = obj->getPosition();
                auto mat = obj->getMaterial();
                
                // Determine object type
                std::string type = "Unknown";
                if (dynamic_cast<Sphere*>(obj.get())) {
                    type = "Sphere";
                }
                else if (dynamic_cast<Cube*>(obj.get())) {
                    type = "Cube";
                }
                else if (dynamic_cast<MeshObject*>(obj.get())) {
                    type = "Mesh";
                }
                
                ImGui::Text("%d: %s at (%.1f, %.1f, %.1f), Color: (%.1f, %.1f, %.1f)", 
                           (int)i, type.c_str(), pos.x, pos.y, pos.z, 
                           mat.color.r, mat.color.g, mat.color.b);
            }
        }
    }
    
    // Lighting controls
    if (ImGui::CollapsingHeader("Lighting")) {
        ImGui::Text("Light Parameters:");
        bool lightChanged = false;
        
        if (ImGui::DragFloat3("Light Position", lightPosition, 0.1f)) {
            lightChanged = true;
        }
        
        if (ImGui::ColorEdit3("Light Color", lightColor)) {
            lightChanged = true;
        }
        
        if (ImGui::DragFloat("Intensity", &lightIntensity, 0.1f, 0.0f, 10.0f)) {
            lightChanged = true;
        }
        
        // Update light if changed
        if (ImGui::Button("Update Light") || lightChanged) {
            // Check if there are already lights
            if (!raytracer->getLights().empty()) {
                // Clear existing lights
                raytracer->clearLights();
            }
            
            // Add the updated light
            Light light(
                glm::vec3(lightPosition[0], lightPosition[1], lightPosition[2]),
                glm::vec3(lightColor[0], lightColor[1], lightColor[2]),
                lightIntensity
            );
            raytracer->addLight(light);
            
            // Only force a retrace if we have objects
            if (!raytracer->getObjects().empty()) {
                raytracer->trace();
            }
        }
    }
    
    // Camera controls
    if (ImGui::CollapsingHeader("Camera")) {
        Camera& camera = raytracer->getCamera();
        
        glm::vec3 cameraPos = camera.getPosition();
        float position[3] = { cameraPos.x, cameraPos.y, cameraPos.z };
        
        glm::vec3 lookAt = camera.getLookAt();
        float target[3] = { lookAt.x, lookAt.y, lookAt.z };
        
        float fov = camera.getFOV();
        
        bool cameraChanged = false;
        
        if (ImGui::DragFloat3("Camera Position", position, 0.1f)) {
            cameraChanged = true;
        }
        
        if (ImGui::DragFloat3("Look At", target, 0.1f)) {
            cameraChanged = true;
        }
        
        if (ImGui::SliderFloat("Field of View", &fov, 10.0f, 120.0f)) {
            cameraChanged = true;
        }
        
        // Update camera if changed
        if (cameraChanged) {
            camera.setPosition(glm::vec3(position[0], position[1], position[2]));
            camera.setLookAt(glm::vec3(target[0], target[1], target[2]));
            camera.setFOV(fov);
        }
    }
    
    // Render button and info
    ImGui::Separator();
    if (ImGui::Button("Render", ImVec2(120, 30))) {
        ImGui::OpenPopup("Rendering...");
        raytracer->trace();
        ImGui::CloseCurrentPopup();
    }
    
    ImGui::SameLine();
    ImGui::Text("Rendering Info:");
    ImGui::Text("- Simple scenes: 1-3 seconds");
    ImGui::Text("- Complex scenes may take longer");
    ImGui::Text("- Higher recursion depth = longer render times");
    ImGui::Text("- Resolution: %dx%d", raytracer->getWidth(), raytracer->getHeight());
}