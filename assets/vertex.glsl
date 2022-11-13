#version 460 core

layout (location=0) in vec2 in_pos;
layout (location=0) out vec2 out_uv;

void main()
{
	out_uv = in_pos / 2 + 0.5;
	gl_Position = vec4(in_pos, 0.0, 1.0);
}
