#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

layout(set = 0, binding = 0) uniform UboVP
{
	mat4 projection;
	mat4 view;
} uboVP;

layout(set = 0, binding = 1) uniform UboM
{
	mat4 model;
} uboM;

layout(location = 0) out vec3 fragCol;

void main()
{
	gl_Position = uboVP.projection * uboVP.view * uboM.model * vec4(pos, 1.0);
	fragCol = col;
}