#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "Utils.h"

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice newPhysDevice, VkDevice newLogicDevice,
		VkQueue transferQueue, VkCommandPool transferCmdPool,
		std::vector<Vertex>* verts, std::vector<uint32_t>* indices);

	int GetVertexCount();
	VkBuffer GetVertexBuffer();

	int GetIndexCount();
	VkBuffer GetIndexBuffer();

	void DestroyBuffers();

	~Mesh();

private:
	int vertexCount;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	int indexCount;
	VkBuffer idxBuffer;
	VkDeviceMemory idxBufferMemory;

	VkPhysicalDevice physDevice;
	VkDevice logicDevice;

	void CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* verts);
	void CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<uint32_t>* indices);
};

