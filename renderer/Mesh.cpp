#include "Mesh.h"
#include <iostream>

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice physDevice, VkDevice logicDevice, std::vector<Vertex>* verts)
{
	this->vertexCount = static_cast<int>(verts->size());
	this->physDevice = physDevice;
	this->logicDevice = logicDevice;
	CreateVertexBuffer(verts);
}

int Mesh::GetVertexCount()
{
	return vertexCount;
}

VkBuffer Mesh::GetVertexBuffer()
{
	return vertexBuffer;
}

void Mesh::DestroyVertexBuffer()
{
	vkDestroyBuffer(logicDevice, vertexBuffer, nullptr);
	vkFreeMemory(logicDevice, vertexBufferMemory, nullptr);
}

Mesh::~Mesh()
{
}

VkBuffer Mesh::CreateVertexBuffer(std::vector<Vertex>* verts)
{
	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = sizeof(Vertex) * verts->size();
	createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(logicDevice, &createInfo, nullptr, &vertexBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create vert buffer");

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(logicDevice, vertexBuffer, &memReq);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = FindMemoryTypeIndex(memReq.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(logicDevice, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to alloc vertex buffer");

	vkBindBufferMemory(logicDevice, vertexBuffer, vertexBufferMemory, 0);

	void* data;
	vkMapMemory(logicDevice, vertexBufferMemory, 0, createInfo.size, 0, &data);
	memcpy(data, verts->data(), (size_t)verts->size());
	vkUnmapMemory(logicDevice, vertexBufferMemory);
}

uint32_t Mesh::FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags wantedMemProps)
{
	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);

	std::cout << "typecount " << memProps.memoryTypeCount << std::endl;

	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
	{
		if (allowedTypes & 1 << i &&
			(memProps.memoryTypes[i].propertyFlags & wantedMemProps) == wantedMemProps)
		{
			std::cout << i << std::endl;
			return i;
		}
	}
}
