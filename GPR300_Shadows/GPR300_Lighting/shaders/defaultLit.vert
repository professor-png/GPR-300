//#version 450                          
//layout (location = 0) in vec3 vPos;  
//layout (location = 1) in vec3 vNormal;
//layout (location = 2) in vec2 vTexCoord;
//layout (location = 3) in vec3 vTangent;
//
//uniform mat4 _Model;
//uniform mat4 _View;
//uniform mat4 _Projection;
//
//out struct Vertex
//{
//    vec3 Normal;
//    vec3 WorldNormal;
//    vec3 WorldPosition;
//    vec2 UV;
//    mat3 TBN;
//}vs_out;
//
//void main(){    
//    vs_out.WorldNormal = mat3(transpose(inverse(_Model))) * vNormal;
//    vs_out.WorldPosition = vec3(_Model * vec4(vPos, 1));
//
//    vs_out.Normal = vNormal;
//    vs_out.UV = vTexCoord;
//
//	//Used https://learnopengl.com/Advanced-Lighting/Normal-Mapping as example
//    vec3 T = normalize(vec3(_Model * vec4(vTangent, 0.0)));
//
//    vec3 bitan = cross(vs_out.Normal, vTangent);
//
//    vec3 B = normalize(vec3(_Model * vec4(bitan, 0.0)));
//    vec3 N = normalize(vec3(_Model * vec4(vs_out.Normal, 0.0)));
//
//    vs_out.TBN = mat3(T, B, N);
//    vs_out.TBN = mat3(transpose(inverse(_Model))) * vs_out.TBN;
//
//    gl_Position = _Projection * _View * _Model * vec4(vPos,1);
//}
//
#version 450                          
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

out struct Vertex
{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
}vs_out;

uniform mat4 _LightSpaceMatrix;
uniform mat4 _Model;
uniform mat4 _View;
uniform mat4 _Projection;

void main()
{
    vs_out.FragPos = vec3(_Model * vec4(vPos, 1.0));
    vs_out.Normal = transpose(inverse(mat3(_Model))) * vNormal;
    vs_out.TexCoords = vTexCoord;
    vs_out.FragPosLightSpace = _LightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
    gl_Position = _Projection * _View * vec4(vs_out.FragPos, 1.0);
}  