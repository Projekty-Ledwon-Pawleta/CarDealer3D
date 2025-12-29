#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float tiling;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    
    // Mnożymy UV przez tiling. 
    // Jeśli tiling = 1.0 -> normalna tekstura
    // Jeśli tiling = 10.0 -> tekstura powtórzona 10 razy (bardzo gęsta)
    TexCoord = aTexCoord * tiling;
}