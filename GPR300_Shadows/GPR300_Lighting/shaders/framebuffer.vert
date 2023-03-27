#version 450

layout (location = 0) in vec3 vPos;  
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

out vec2 texCoords;

void main()
{
    gl_Position = vec4(vPos.x, vPos.y, 0.0, 1.0); 
    texCoords = vTexCoord;
}  