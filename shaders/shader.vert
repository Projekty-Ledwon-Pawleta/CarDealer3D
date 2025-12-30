#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float tiling;

void main() {
    // Obliczamy pozycję fragmentu w świecie 3D
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Obliczamy wektor normalny (poprawka na skalowanie modelu - WAŻNE!)
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    
    // Przekazujemy UV z tilingiem
    TexCoord = aTexCoord * tiling;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}