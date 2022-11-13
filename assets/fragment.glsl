#version 460 core

uniform float Time;

layout (location=0) in vec2 uv;
out vec4 out_color;

void main()
{
	vec3 col = 0.5 + 0.5*cos(Time+uv.xyx+vec3(0,2,4));
	out_color = vec4(col,1.0);
}
