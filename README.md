# Computer Graphics Assignment

This project implements various computer graphics algorithms including mesh slicing, rasterization, scan conversion, and ray tracing.

## Features

1. **Mesh Slicing**
   - Slice 3D meshes using 1-4 arbitrary planes
   - Interactive UI for specifying plane equations

2. **Rasterization**
   - Line rasterization algorithm
   - Handles all slope cases and quadrants

3. **Scan Conversion**
   - Scan-line polygon filling algorithm
   - Efficient implementation for complex polygons

4. **Ray Tracing**
   - Shadows for realistic lighting
   - Support for basic primitives (cubes, spheres)
   - Support for polygonal meshes
   - Reflection effects (bonus feature)

## Requirements

- C++17 compiler
- OpenGL 4.3+
- GLFW
- GLM
- Dear ImGui (included)

## Building the Project

```bash
# Clone the repository
git clone https://github.com/yourusername/cg-assignment.git
cd cg-assignment

# Build using make
make

# Run the application
./bin/cg-app
```

## Usage

The application provides a graphical interface with different tabs for each feature:

1. **Mesh Slicing Tab**
   - Load a mesh using the "Load Mesh" button
   - Define plane equations (ax + by + cz + d = 0)
   - Apply slicing and visualize results

2. **Rasterization Tab**
   - Draw lines by specifying endpoints
   - Visualize the rasterization process

3. **Scan Conversion Tab**
   - Define polygons by adding vertices
   - Apply scan-line fill algorithm
   - Visualize the filled polygon

4. **Ray Tracing Tab**
   - Set up a scene with primitives and meshes
   - Adjust camera position and parameters
   - Render the scene with ray tracing
   - Toggle reflections on/off

## Controls

- **Camera Navigation**
  - Left Mouse: Rotate camera
  - Middle Mouse: Pan camera
  - Right Mouse: Zoom in/out
  - W/A/S/D: Move camera

- **Object Manipulation**
  - Select objects by clicking
  - G: Grab (translate)
  - R: Rotate
  - S: Scale

## License

This project is for educational purposes only.