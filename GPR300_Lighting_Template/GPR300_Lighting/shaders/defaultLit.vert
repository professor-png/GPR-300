#version 450                          
layout (location = 0) in vec3 vPos;  
layout (location = 1) in vec3 vNormal;

uniform mat4 _Model;
uniform mat4 _View;
uniform mat4 _Projection;

out vec3 Normal; //local space
out vec3 WorldNormal;
out vec3 WorldPos;

void main(){    
    Normal = vNormal;
    WorldPos = vec3(_Model * vec4(vPos, 1));
    WorldNormal = mat3(transpose(inverse(_Model))) * vNormal;

    gl_Position = _Projection * _View * _Model * vec4(vPos,1);
}
