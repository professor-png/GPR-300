#version 450
out vec4 FragColor;

uniform vec3 _OutlineColor;

void main()
{
	FragColor = vec4(_OutlineColor, 1.0f);
}