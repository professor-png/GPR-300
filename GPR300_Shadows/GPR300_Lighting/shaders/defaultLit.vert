#version 450                          
layout (location = 0) in vec3 vPos;  
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;

uniform mat4 _Model;
uniform mat4 _View;
uniform mat4 _Projection;
uniform mat4 _LightSpaceMatrix;

out struct Vertex
{
    vec3 Normal;
    vec3 WorldPosition;
    vec2 UV;
    vec4 FragPosLightSpace;
}vs_out;

void main(){    
    vs_out.WorldPosition = vec3(_Model * vec4(vPos, 1));

    vs_out.Normal = vNormal;
    vs_out.UV = vTexCoord;

    vs_out.FragPosLightSpace = _LightSpaceMatrix * vec4(vs_out.WorldPosition, 1.0);
    gl_Position = _Projection * _View * _Model * vec4(vPos,1);
}