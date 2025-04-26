#version 430 core

// Input vertex data
layout(location = 0) in vec3 position;

// Output data to fragment shader
out vec3 FragPos;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Calculate world-space position
    FragPos = vec3(model * vec4(position, 1.0));
    
    // Transform vertex to clip space
    gl_Position = projection * view * vec4(FragPos, 1.0);
}