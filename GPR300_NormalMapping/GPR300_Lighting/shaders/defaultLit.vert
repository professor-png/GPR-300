#version 450                          
layout (location = 0) in vec3 vPos;  
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;

uniform mat4 _Model;
uniform mat4 _View;
uniform mat4 _Projection;

out struct Vertex
{
    vec3 Normal;
    vec3 WorldNormal;
    vec3 WorldPosition;
    vec2 UV;
    mat3 TBN;
}vs_out;

void main(){    
    vs_out.WorldNormal = mat3(transpose(inverse(_Model))) * vNormal;
    vs_out.WorldPosition = vec3(_Model * vec4(vPos, 1));

    vs_out.Normal = vNormal;
    vs_out.UV = vTexCoord;

    vec3 bitan = cross(vs_out.WorldNormal, vTangent);
    vs_out.TBN = mat3(vTangent, bitan, vs_out.WorldNormal);

    gl_Position = _Projection * _View * _Model * vec4(vPos,1);
}
