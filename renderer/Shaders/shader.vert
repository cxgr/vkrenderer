#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec2 uv;

layout(set = 0, binding = 0) uniform UboVP
{
	mat4 projection;
	mat4 view;
} uboVP;

layout(push_constant) uniform PushModel
{
	mat4 model;
} pushModel;

layout(location = 0) out vec3 fragCol;
layout(location = 1) out vec2 fragUV;

void main()
{
	gl_Position = uboVP.projection * uboVP.view * pushModel.model * vec4(pos, 1.0);
	fragCol = col;
	fragUV = uv;
}