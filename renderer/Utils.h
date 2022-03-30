#pragma once
#define GLFW_INCLUDE_VULKAN

#include <fstream>
#include "glm/glm.hpp"
#include <GLFW/glfw3.h>
const int MAX_QUEUED_DRAWS = 2;
const int MAX_MESHES = 200;
const int MAX_TEXTURES = 200;
const uint64_t DRAW_TIMEOUT = std::numeric_limits<uint64_t>::max();

const std::vector<const char*> wantedDeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 col;
	glm::vec2 uv;
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

static VkCommandBuffer BeginCmdBuffer(VkDevice logicDevice, VkCommandPool cmdPool)
{
	VkCommandBuffer cmdBuffer;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(logicDevice, &allocInfo, &cmdBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmdBuffer, &beginInfo);

	return cmdBuffer;
}

static void EndAndSubmitCmdBuffer(VkDevice logicDevice, VkCommandPool cmdPool, VkQueue queue, VkCommandBuffer cmdBuffer)
{
	vkEndCommandBuffer(cmdBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(queue);
	vkFreeCommandBuffers(logicDevice, cmdPool, 1, &cmdBuffer);
}

static void CopyBuffer(VkDevice logicDevice, VkQueue transferQ, VkCommandPool transferCmdPool,
	VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
	auto transferCmdBuffer = BeginCmdBuffer(logicDevice, transferCmdPool);
	{
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = bufferSize;

		vkCmdCopyBuffer(transferCmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}
	EndAndSubmitCmdBuffer(logicDevice, transferCmdPool, transferQ, transferCmdBuffer);
}

static void CopyImgBuffer(VkDevice logicDevice, VkQueue transferQ, VkCommandPool transferCmdPool,
	VkBuffer srcBuffer, VkImage dstImg, uint32_t wid, uint32_t hei)
{
	auto transferCmdBuffer = BeginCmdBuffer(logicDevice, transferCmdPool);
	{
		VkBufferImageCopy imgRegion = {};
		imgRegion.bufferOffset = 0;
		imgRegion.bufferRowLength = 0;
		imgRegion.bufferImageHeight = 0;
		imgRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgRegion.imageSubresource.mipLevel = 0;
		imgRegion.imageSubresource.baseArrayLayer = 0;
		imgRegion.imageSubresource.layerCount = 1;
		imgRegion.imageOffset = { 0, 0, 0 };
		imgRegion.imageExtent = { wid, hei, 1 };

		vkCmdCopyBufferToImage(transferCmdBuffer, srcBuffer, dstImg, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgRegion);
	}
	EndAndSubmitCmdBuffer(logicDevice, transferCmdPool, transferQ, transferCmdBuffer);
}

static void TransitionImageLayout(VkDevice logicDevice, VkQueue queue, VkCommandPool cmdPool, VkImage img,
	VkImageLayout srcLayout, VkImageLayout dstLayout)
{
	auto cmdBuffer = BeginCmdBuffer(logicDevice, cmdPool);
	{
		VkImageMemoryBarrier imgMemBarrier = {};
		imgMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imgMemBarrier.oldLayout = srcLayout;
		imgMemBarrier.newLayout = dstLayout;
		imgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgMemBarrier.image = img;
		imgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgMemBarrier.subresourceRange.baseMipLevel = 0;
		imgMemBarrier.subresourceRange.levelCount = 1;
		imgMemBarrier.subresourceRange.baseArrayLayer = 0;
		imgMemBarrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags srcStage, dstStage;

		if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			imgMemBarrier.srcAccessMask = 0;
			imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (srcLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		vkCmdPipelineBarrier
		(
			cmdBuffer,
			srcStage, dstStage, //pipeline
			0, //dependency
			0, nullptr, //global memory
			0, nullptr, //buffer memory
			1, &imgMemBarrier //img
		);
	}
	EndAndSubmitCmdBuffer(logicDevice, cmdPool, queue, cmdBuffer);
}