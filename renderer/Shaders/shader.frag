#version 450

layout(location = 0) in vec3 fragCol;
layout(location = 1) in vec2 fragUV;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outCol;

void main()
{
	outCol = texture(texSampler, fragUV);
}