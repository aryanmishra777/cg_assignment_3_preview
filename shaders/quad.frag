#version 430 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D rayTracedTexture;

void main() {
    FragColor = texture(rayTracedTexture, TexCoord);
}