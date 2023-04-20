#version 450

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;

uniform mat4 _Projection;
uniform mat4 _View;
uniform mat4 _Model;

void main()
{
	vec3 currentPos = vec3(_Model * vec4(vPos, 1));
	gl_Position = _Projection * _View * vec4(currentPos, 1.0f);
}