#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "Utils.h"

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice physDevice, VkDevice logicDevice, std::vector<Vertex> * verts);

	int GetVertexCount();
	VkBuffer GetVertexBuffer();

	void DestroyVertexBuffer();

	~Mesh();

private:
	int vertexCount;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkPhysicalDevice physDevice;
	VkDevice logicDevice;

	VkBuffer CreateVertexBuffer(std::vector<Vertex>* verts);
	uint32_t FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags wantedMemProps);
};

