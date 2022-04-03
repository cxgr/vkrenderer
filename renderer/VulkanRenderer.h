#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include <array>

#include "stb_image.h"

#include "Utils.h"
#include "Mesh.h"
#include "MeshModel.h"

class VulkanRenderer
{
public:
	std::vector<MeshModel> models;

	VulkanRenderer();
	int Init(GLFWwindow* window);
	void InitScene();

	size_t CreateMeshModel(std::string fileName);
	glm::mat4 GetModel(size_t id);
	void UpdateModel(size_t id, glm::mat4 newModel);

	void Draw();
	void Cleanup();
	~VulkanRenderer();

private:
	int frameIdx = 0;

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

	struct BufferImage
	{
		VkImage img;
		VkDeviceMemory memory;
		VkImageView imgView;
	};
	std::vector<BufferImage> depthBuffers;
	VkFormat depthBufferFormat;
	
	std::vector<BufferImage> colorBuffers;
	VkFormat colorBufferFormat;

	VkSampler texSampler;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout samplerSetLayout;
	VkDescriptorSetLayout inputSetLayout;
	VkPushConstantRange pushConstantRange;

	VkDescriptorPool descriptorPool;
	VkDescriptorPool samplerDescriptorPool;
	VkDescriptorPool inputDescriptorPool;
	std::vector<VkDescriptorSet> descriptorSets; //1 per swapchain img
	std::vector<VkDescriptorSet> samplerDescriptorSets; //1 per texture
	std::vector<VkDescriptorSet> inputDescriptorSets;

	std::vector<VkBuffer> vpUniformBuffers;
	std::vector<VkDeviceMemory> vpUniformBufferMemories;

	//std::vector<MeshModel> models;

	std::vector<VkImage> texImages;
	std::vector<VkDeviceMemory> texImgMemories;
	std::vector<VkImageView> texImgViews;


	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
	VkPipeline blitPipeline;
	VkPipelineLayout blitLayout;
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
	void CreateDescriptorSetLayouts();
	void CreatePushConstantRange();

	void CreateGraphicsPipeline();
	void CreateBlitPipeline();

	void CreateColorBuffers();
	void CreateDepthBuffers();

	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObjects();
	void CreateTexSampler();

	void CreateUniformBuffers();
	void CreateDescriptorPools();
	void CreateDescriptorSets();
	void CreateInputDescriptorSets();

	void UpdateUniformBuffers(uint32_t imgIdx);

	void RecordCommands(uint32_t imgIdx);

	void GetPhysicalDevice();
	QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice device);
	SwapchainProperties GetSwapchainProperties(VkPhysicalDevice device);

	bool CheckInstanceExtensionSupport(std::vector<const char*>* extsToCheck);
	bool CheckDeviceSuitable(VkPhysicalDevice device);

	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
	VkFormat ChooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	VkImage CreateImage(uint32_t wid, uint32_t hei, VkFormat format, VkDeviceMemory* imgMemory,
		VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule CreateShaderModule(const std::vector<char> &shader);

	size_t CreateTextureImage(std::string fileName);
	size_t CreateTexture(std::string fileName);
	size_t CreateTextureDescriptor(VkImageView texImgView);

	stbi_uc* LoadImage(std::string fileName, int* wid, int* hei, VkDeviceSize* imgSize);
};