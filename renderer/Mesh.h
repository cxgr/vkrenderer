#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "Utils.h"

struct Model
{
	glm::mat4 model;
};

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice newPhysDevice, VkDevice newLogicDevice,
		VkQueue transferQueue, VkCommandPool transferCmdPool,
		std::vector<Vertex>* verts, std::vector<uint32_t>* indices, size_t textureId);

	void SetModel(glm::mat4 newModel);
	Model GetModel();
	size_t GetTexId();

	int GetVertexCount();
	VkBuffer GetVertexBuffer();

	int GetIndexCount();
	VkBuffer GetIndexBuffer();

	void DestroyBuffers();

	~Mesh();

private:
	Model model;

	size_t texId;

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

