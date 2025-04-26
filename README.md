# Computer Graphics Assignment

This project implements various fundamental computer graphics algorithms including:
1. Mesh slicing with arbitrary planes
2. Line rasterization 
3. Polygon scan conversion
4. Ray tracing with shadows and reflections

## Features

### Mesh Slicing
- Slice 3D meshes with 1-4 arbitrary planes
- Interactive UI to define plane equations
- Visualize the sliced mesh in real-time

### Rasterization
- Line drawing algorithm that handles all slope cases
- Works across all quadrants
- Optimized for efficiency

### Scan Conversion
- Fill polygons using scan-line algorithm
- Properly handles concave and convex polygons
- Efficient edge sorting and active edge list management

### Ray Tracing
- Basic ray tracing engine
- Shadow computation
- Support for spheres, cubes, and polygonal meshes
- Optional reflections

## Requirements
- C++17 compatible compiler
- OpenGL 4.3+
- GLFW3
- GLM
- Dear ImGui

## Building
```bash
# Clone the repository
git clone <repository-url>
cd <repository-directory>

# Build the project
make

# Run the program
./graphics_app
```

## Usage
- Use W/A/S/D keys to navigate the camera
- Use mouse to look around
- UI controls for:
  - Switching between visualization modes
  - Setting plane equations for slicing
  - Configuring ray tracing parameters
  - Loading different models

## Implementation Details

### Mesh Slicing
The mesh slicing algorithm works by:
1. Computing the signed distance of each vertex to the slicing plane
2. Determining which edges are intersected by the plane
3. Computing the intersection points
4. Creating new polygons along the intersection

### Rasterization
The line rasterization algorithm implements Bresenham's line algorithm with modifications to handle all cases of slopes and quadrants.

- Handles all possible combinations of slopes (+ve/-ve) and quadrants
- Works with any two endpoints within the viewport
- The implementation includes:
  - Special case handling for horizontal and vertical lines
  - Proper step direction calculation for all quadrants
  - Error accumulation and adjustment for accurate pixel placement
  - High-contrast visualization with adjustable line thickness
  - Support for arbitrary line colors
- Implementation uses normalized coordinates in the UI, which are converted to pixel coordinates for actual rasterization
- For testing purposes, both horizontal/vertical lines and diagonal lines at various angles are supported

### Scan Conversion
The scan-line fill algorithm processes horizontal spans between edges:
1. Create a sorted edge table
2. Process active edges for each scanline
3. Fill spans between pairs of intersections

### Ray Tracing
The ray tracing engine implements:
1. Ray-object intersection tests
2. Shadow ray computation
3. Basic lighting model
4. Recursive ray tracing for reflections