#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include <array>

#include "Utils.h"
#include "Mesh.h"

class VulkanRenderer
{
public:
	VulkanRenderer();
	int Init(GLFWwindow* window);
	void UpdateModel(int meshId, glm::mat4 newModel);
	void Draw();
	void Cleanup();
	~VulkanRenderer();

private:
	int frameIdx = 0;

	std::vector<Mesh> meshes;

	struct UboViewProjection
	{
		glm::mat4 projection;
		glm::mat4 view;
	} uboViewProjection;

	GLFWwindow* window;
	VkInstance vkInstance;
	struct
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;

	std::vector<SwapchainImage> swapchainImages;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	VkDescriptorSetLayout descriptorSetLayout;
	VkPushConstantRange pushConstantRange;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<VkBuffer> vpUniformBuffers;
	std::vector<VkDeviceMemory> vpUniformBufferMemories;

	//std::vector<VkBuffer> modelDynUniformBuffers;
	//std::vector<VkDeviceMemory> modelDynUniformBufferMemories;

	//VkDeviceSize minUniformBufferOffset;
	//size_t modelUniformAlignment;
	//UboModel* modelTransferSpace;

	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;

	VkCommandPool graphicsCommandPool;

	VkFormat swapchainImgFormat;
	VkExtent2D swapchainImgExtent;

	std::vector<VkSemaphore> semsImgAvailable;
	std::vector<VkSemaphore> semsRenderFinished;
	std::vector<VkFence> drawFences;

	bool CheckValidationLayersAvailable(std::vector<const char*> wantedLayers);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	void CreateInstance();
	void CreateLogicalDevice();
	void CreateSurface();
	void CreateSwapchain();
	void CreateRenderPass();
	void CreateDescriptorSetLayout();
	void CreatePushConstantRange();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void UpdateUniformBuffers(uint32_t imgIdx);

	void RecordCommands(uint32_t imgIdx);

	void GetPhysicalDevice();
	QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device);
	SwapchainProperties GetSwapchainProperties(VkPhysicalDevice device);

	bool CheckInstanceExtensionSupport(std::vector<const char*>* extensions);
	bool CheckDeviceSuitable(VkPhysicalDevice device);

	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule CreateShaderModule(const std::vector<char> &shader);

	//void AllocateDynamicBufferTransferSpace();
};
