#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout (location = 0) out vec4 color;

void main()
{
	int xHalf = 1600 / 2;
	if (gl_FragCoord.x > xHalf)
	{
		float low = 0.98f;
		float high = 1;
		float depth = subpassLoad(inputDepth).r;
		float remap = 1.0f - ((depth - low) / (high - low));
		vec3 remapCol = subpassLoad(inputColor).rgb * remap;
		color = vec4(remapCol, 1.0f);
	}
	else
	{
		color = subpassLoad(inputColor).rgba;
	}
}