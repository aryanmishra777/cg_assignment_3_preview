#version 430 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

struct Material {
    vec3 color;
    float ambient;
    float diffuse;
    float specular;
    float shininess;
};

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform Material material;
uniform Light lights[4]; // Support up to 4 lights
uniform int numLights;
uniform vec3 viewPos;

void main() {
    vec3 norm = normalize(Normal);
    
    // Ambient
    vec3 ambient = material.color * material.ambient;
    
    vec3 result = ambient;
    
    // Calculate lighting contribution from each light
    for(int i = 0; i < numLights; i++) {
        // Diffuse
        vec3 lightDir = normalize(lights[i].position - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = material.color * diff * material.diffuse;
        
        // Specular
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specular = lights[i].color * spec * material.specular;
        
        // Attenuation
        float distance = length(lights[i].position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        
        // Combine
        result += (diffuse + specular) * lights[i].color * lights[i].intensity * attenuation;
    }
    
    FragColor = vec4(result, 1.0);
}