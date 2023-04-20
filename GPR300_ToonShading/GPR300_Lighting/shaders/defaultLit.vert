#version 450                          
layout (location = 0) in vec3 vPos;  
layout (location = 1) in vec3 vNormal;

uniform mat4 _Model;
uniform mat4 _View;
uniform mat4 _Projection;

out struct Vertex
{
    vec3 WorldNormal;
    vec3 WorldPosition;
}vs_out;

void main(){    
    vs_out.WorldNormal = mat3(transpose(inverse(_Model))) * vNormal;
    vs_out.WorldPosition = vec3(_Model * vec4(vPos, 1));

    gl_Position = _Projection * _View * _Model * vec4(vPos,1);
}
