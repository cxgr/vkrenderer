#pragma once
#define GLFW_INCLUDE_VULKAN

#include <fstream>
#include "glm/glm.hpp"
#include <GLFW/glfw3.h>
const int MAX_QUEUED_DRAWS = 2;
const uint64_t DRAW_TIMEOUT = std::numeric_limits<uint64_t>::max();

const std::vector<const char*> wantedDeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 col;
};

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentationFamily = -1;

	bool IsValid()
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapchainProperties
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct SwapchainImage
{
	VkImage image;
	VkImageView imageView;
};

static std::vector<char> ReadFile(const std::string &filename)
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file");
	}

	const size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

static uint32_t FindMemoryTypeIndex(VkPhysicalDevice physDevice, uint32_t allowedTypes, VkMemoryPropertyFlags wantedMemProps)
{
	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);

	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
	{
		if (allowedTypes & (1 << i) &&
			(memProps.memoryTypes[i].propertyFlags & wantedMemProps) == wantedMemProps)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to allocate memory for vb");
}

static void CreateBuffer(VkPhysicalDevice physDevice, VkDevice logicDevice, VkDeviceSize bufferSize,
	VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProps, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = bufferSize;
	createInfo.usage = bufferUsage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(logicDevice, &createInfo, nullptr, buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer");

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(logicDevice, *buffer, &memReq);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = FindMemoryTypeIndex(physDevice, memReq.memoryTypeBits, bufferProps);

	if (vkAllocateMemory(logicDevice, &allocInfo, nullptr, bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to alloc buffer");

	vkBindBufferMemory(logicDevice, *buffer, *bufferMemory, 0);
}

static void CopyBuffer(VkDevice logicDevice, VkQueue transferQ, VkCommandPool transferCmdPool,
	VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
	VkCommandBuffer transferCmdBuffer;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = transferCmdPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(logicDevice, &allocInfo, &transferCmdBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(transferCmdBuffer, &beginInfo);
	{
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = bufferSize;

		vkCmdCopyBuffer(transferCmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}
	vkEndCommandBuffer(transferCmdBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &transferCmdBuffer;

	vkQueueSubmit(transferQ, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(transferQ);
	vkFreeCommandBuffers(logicDevice, transferCmdPool, 1, &transferCmdBuffer);
}