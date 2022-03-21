#include "Mesh.h"
#include <iostream>

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice newPhysDevice, VkDevice newLogicDevice,
	VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* verts, std::vector<uint32_t>* indices) :
	vertexCount(static_cast<int>(verts->size())),
	indexCount(static_cast<int>(indices->size())),
	physDevice(newPhysDevice),
	logicDevice(newLogicDevice)
{
	CreateVertexBuffer(transferQueue, transferCmdPool, verts);
	CreateIndexBuffer(transferQueue, transferCmdPool, indices);
}

int Mesh::GetVertexCount()
{
	return vertexCount;
}

VkBuffer Mesh::GetVertexBuffer()
{
	return vertexBuffer;
}

int Mesh::GetIndexCount()
{
	return indexCount;
}

VkBuffer Mesh::GetIndexBuffer()
{
	return idxBuffer;
}

void Mesh::DestroyBuffers()
{
	vkDestroyBuffer(logicDevice, vertexBuffer, nullptr);
	vkFreeMemory(logicDevice, vertexBufferMemory, nullptr);

	vkDestroyBuffer(logicDevice, idxBuffer, nullptr);
	vkFreeMemory(logicDevice, idxBufferMemory, nullptr);
}

Mesh::~Mesh()
{
}

void Mesh::CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* verts)
{
	VkDeviceSize bufferSize = sizeof(Vertex) * verts->size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(physDevice, logicDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	void* data;
	vkMapMemory(logicDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, verts->data(), bufferSize);
	vkUnmapMemory(logicDevice, stagingBufferMemory);

	CreateBuffer(physDevice, logicDevice, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&vertexBuffer, &vertexBufferMemory);

	CopyBuffer(logicDevice, transferQueue, transferCmdPool, stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(logicDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicDevice, stagingBufferMemory, nullptr);
}

void Mesh::CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<uint32_t>* indices)
{
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(physDevice, logicDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	void* data;
	vkMapMemory(logicDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices->data(), bufferSize);
	vkUnmapMemory(logicDevice, stagingBufferMemory);

	CreateBuffer(physDevice, logicDevice, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&idxBuffer, &idxBufferMemory);

	CopyBuffer(logicDevice, transferQueue, transferCmdPool, stagingBuffer, idxBuffer, bufferSize);

	vkDestroyBuffer(logicDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicDevice, stagingBufferMemory, nullptr);
}
