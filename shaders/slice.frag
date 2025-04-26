#version 430 core

// Input data from vertex shader
in vec3 FragPos;

// Output color
out vec4 FragColor;

// Uniform for slice color
uniform vec3 sliceColor;

void main() {
    // Use the provided slice color
    FragColor = vec4(sliceColor, 1.0);
}