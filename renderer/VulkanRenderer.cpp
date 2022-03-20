#include "VulkanRenderer.h"

#include <iostream>
#include <string>

VulkanRenderer::VulkanRenderer()
{
}

int VulkanRenderer::Init(GLFWwindow* window)
{
	this->window = window;

	try
	{
		CreateInstance();
		CreateSurface();
		GetPhysicalDevice();
		CreateLogicalDevice();

		std::vector<Vertex> meshVerts = {
						{{0.4, -0.4, 0.0}},
			{{0.4, 0.4, 0.0}},
			{{-0.4, 0.4, 0.0}}
		};
		firstMesh = Mesh(mainDevice.physicalDevice, mainDevice.logicalDevice, &meshVerts);

		CreateSwapchain();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandPool();
		CreateCommandBuffers();
		RecordCommands();
		CreateSyncObjects();
	}
	catch (const std::runtime_error& e)
	{
		printf(e.what());
		return 1;
	}

	return 0;
}

void VulkanRenderer::Draw()
{
	vkWaitForFences(mainDevice.logicalDevice, 1, &drawFences[frameIdx], VK_TRUE, DRAW_TIMEOUT);
	vkResetFences(mainDevice.logicalDevice, 1, &drawFences[frameIdx]);

	uint32_t imgIdx;
	if (VK_SUCCESS != vkAcquireNextImageKHR(mainDevice.logicalDevice, swapchain, DRAW_TIMEOUT,
		semsImgAvailable[frameIdx], VK_NULL_HANDLE, &imgIdx))
	{
		throw std::runtime_error("failed to acquire img");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &semsImgAvailable[frameIdx];
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imgIdx];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &semsRenderFinished[frameIdx];

	if (VK_SUCCESS != vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[frameIdx]))
		throw std::runtime_error("failed to submit cmd buffer to graphics queue");

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &semsRenderFinished[frameIdx];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &imgIdx;

	if (VK_SUCCESS != vkQueuePresentKHR(presentationQueue, &presentInfo))
	{
		throw std::runtime_error("failed to present img");
	}

	frameIdx = ++frameIdx % MAX_QUEUED_DRAWS;
}

void VulkanRenderer::Cleanup()
{
	vkDeviceWaitIdle(mainDevice.logicalDevice);

	firstMesh.DestroyVertexBuffer();

	for (size_t i = 0; i < MAX_QUEUED_DRAWS; ++i)
	{
		vkDestroySemaphore(mainDevice.logicalDevice, semsRenderFinished[i], nullptr);
		vkDestroySemaphore(mainDevice.logicalDevice, semsImgAvailable[i], nullptr);
		vkDestroyFence(mainDevice.logicalDevice, drawFences[i], nullptr);
	}
	vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);
	for (auto fb : swapchainFramebuffers)
		vkDestroyFramebuffer(mainDevice.logicalDevice, fb, nullptr);
	vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);
	for (auto image : swapchainImages)
		vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
	vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
	vkDestroySurfaceKHR(vkInstance, surface, nullptr);
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(vkInstance, nullptr);
}

VulkanRenderer::~VulkanRenderer()
{
}

bool VulkanRenderer::CheckValidationLayersAvailable(std::vector<const char*> wantedLayers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const auto& wl : wantedLayers)
	{
		bool layerFound = false;
		for (const auto &al : availableLayers)
		{
			if (0 == strcmp(wl, al.layerName))
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
			return false;
	}

	return true;
}

bool VulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
	if (extCount <= 0)
		return false;
	std::vector<VkExtensionProperties> deviceExtList(extCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, deviceExtList.data());

	for (const auto &wantedExtension : wantedDeviceExtensions)
	{
		bool hasExtension = false;
		for (const auto &devExt : deviceExtList)
		{
			if (strcmp(wantedExtension, devExt.extensionName))
			{
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension)
			return false;
	}

	return true;
}

void VulkanRenderer::CreateInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "testname";
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;


	uint32_t extCount = 0;
	auto glfwExts = glfwGetRequiredInstanceExtensions(&extCount);
	std::vector<const char*> instanceExtensions = std::vector<const char*>();
	for (size_t i = 0; i < extCount; ++i)
		instanceExtensions.push_back(glfwExts[i]);

	if (!CheckInstanceExtensionSupport(&instanceExtensions))
		throw std::runtime_error("missing vkInstance ext support");

	createInfo.enabledExtensionCount = extCount;
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	const std::vector<const char*> wantedValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	if (CheckValidationLayersAvailable(wantedValidationLayers))
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(wantedValidationLayers.size());
		createInfo.ppEnabledLayerNames = wantedValidationLayers.data();
	}

	auto result = vkCreateInstance(&createInfo, nullptr, &vkInstance);
	if (result != VK_SUCCESS)
		throw std::runtime_error("could not create vk vkInstance");
}

void VulkanRenderer::CreateLogicalDevice()
{
	QueueFamilyIndices indices = GetQueueFamilyIndices(mainDevice.physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	for (int familyIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = familyIndex;
		queueCreateInfo.queueCount = 1;
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(wantedDeviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = wantedDeviceExtensions.data();

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to create a logical device");

	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}

void VulkanRenderer::CreateSurface()
{
	auto result = glfwCreateWindowSurface(vkInstance, window, nullptr, &surface);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create surface");
	}
}

void VulkanRenderer::CreateSwapchain()
{
	SwapchainProperties scProperties = GetSwapchainProperties(mainDevice.physicalDevice);

	const VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(scProperties.formats);
	const VkPresentModeKHR presentMode = ChoosePresentMode(scProperties.presentModes);
	const VkExtent2D extent = ChooseExtent(scProperties.surfaceCapabilities);

	auto imageCount = scProperties.surfaceCapabilities.minImageCount + 1;
	if (scProperties.surfaceCapabilities.maxImageCount > 0 && scProperties.surfaceCapabilities.maxImageCount < imageCount)
		imageCount = scProperties.surfaceCapabilities.maxImageCount;

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.preTransform = scProperties.surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	QueueFamilyIndices indices = GetQueueFamilyIndices(mainDevice.physicalDevice);
	if (indices.graphicsFamily == indices.presentationFamily)
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 1;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}
	else
	{
		uint32_t idxArray[] = { static_cast<uint32_t>(indices.graphicsFamily), static_cast<uint32_t>(indices.presentationFamily) };
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = idxArray;
	}

	auto result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapchainCreateInfo, nullptr, &swapchain);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swapchain");
	}

	swapchainImgFormat = surfaceFormat.format;
	swapchainImgExtent = extent;

	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapchainImageCount, nullptr);
	std::vector<VkImage> imageList(swapchainImageCount);
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapchainImageCount, imageList.data());

	for (const auto& image : imageList)
	{
		SwapchainImage newImage = {};
		newImage.image = image;
		newImage.imageView = CreateImageView(image, swapchainImgFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		swapchainImages.push_back(newImage);
	}
}

void VulkanRenderer::CreateGraphicsPipeline()
{
	auto vertShader = ReadFile("Shaders/vert.spv");
	auto fragShader = ReadFile("Shaders/frag.spv");

	auto vertModule = CreateShaderModule(vertShader);
	auto fragModule = CreateShaderModule(fragShader);

	VkPipelineShaderStageCreateInfo vertCreateInfo = {};
	vertCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertCreateInfo.module = vertModule;
	vertCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragCreateInfo = {};
	fragCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragCreateInfo.module = fragModule;
	fragCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertCreateInfo, fragCreateInfo };

	VkVertexInputBindingDescription bindingDesc = {};
	bindingDesc.binding = 0; //stream idx
	bindingDesc.stride = sizeof(Vertex);
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 1> attributeDescs;
	attributeDescs[0].binding = 0;
	attributeDescs[0].location = 0;
	attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescs[0].offset = offsetof(Vertex, pos);

	//vert input
	VkPipelineVertexInputStateCreateInfo viCreateInfo = {};
	viCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	viCreateInfo.vertexBindingDescriptionCount = 1;
	viCreateInfo.pVertexBindingDescriptions = &bindingDesc;
	viCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
	viCreateInfo.pVertexAttributeDescriptions = attributeDescs.data();

	VkPipelineInputAssemblyStateCreateInfo iaCreateInfo = {};
	iaCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	iaCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	iaCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainImgExtent.width);
	viewport.height = static_cast<float>(swapchainImgExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = swapchainImgExtent;

	VkPipelineViewportStateCreateInfo vsCreateInfo = {};
	vsCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vsCreateInfo.viewportCount = 1;
	vsCreateInfo.pViewports = &viewport;
	vsCreateInfo.scissorCount = 1;
	vsCreateInfo.pScissors = &scissor;

	/*
	std::vector<VkDynamicState> dynStates;
	dynStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);		//vkCmdSetViewport(buffer, 0, 1, &viewport)
	dynStates.push_back(VK_DYNAMIC_STATE_SCISSOR);		//vkCmdSetScissor(buffer, 0, 1, &viewport)
	VkPipelineDynamicStateCreateInfo dsCreateInfo = {};
	dsCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dsCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
	dsCreateInfo.pDynamicStates = dynStates.data();
	*/

	VkPipelineRasterizationStateCreateInfo rastCreateInfo = {};
	rastCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rastCreateInfo.depthClampEnable = VK_FALSE;
	rastCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rastCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rastCreateInfo.lineWidth = 1.0f;
	rastCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rastCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rastCreateInfo.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo msCreateInfo = {};
	msCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	msCreateInfo.sampleShadingEnable = VK_FALSE;
	msCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState blendState = {};
	blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendState.blendEnable = VK_TRUE;
	blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendState.colorBlendOp = VK_BLEND_OP_ADD;
	blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo blendCreateInfo = {};
	blendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendCreateInfo.logicOpEnable = false;
	blendCreateInfo.attachmentCount = 1;
	blendCreateInfo.pAttachments = &blendState;

	//TODO descriptor sets

	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 0;
	layoutCreateInfo.pSetLayouts = nullptr;
	layoutCreateInfo.pushConstantRangeCount = 0;
	layoutCreateInfo.pPushConstantRanges = nullptr;

	auto result = vkCreatePipelineLayout(mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline");

	//TODO depth stencil

	VkGraphicsPipelineCreateInfo pipeCreateInfo = {};
	pipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeCreateInfo.stageCount = 2;
	pipeCreateInfo.pStages = shaderStages;
	pipeCreateInfo.pVertexInputState = &viCreateInfo;
	pipeCreateInfo.pInputAssemblyState = &iaCreateInfo;
	pipeCreateInfo.pViewportState = &vsCreateInfo;
	pipeCreateInfo.pDynamicState = nullptr;
	pipeCreateInfo.pRasterizationState = &rastCreateInfo;
	pipeCreateInfo.pMultisampleState = &msCreateInfo;
	pipeCreateInfo.pColorBlendState = &blendCreateInfo;
	pipeCreateInfo.pDepthStencilState = nullptr;
	pipeCreateInfo.layout = pipelineLayout;
	pipeCreateInfo.renderPass = renderPass;
	pipeCreateInfo.subpass = 0;
	pipeCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipeCreateInfo.basePipelineIndex = -1;

	result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipeCreateInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline");

	vkDestroyShaderModule(mainDevice.logicalDevice, fragModule, nullptr);
	vkDestroyShaderModule(mainDevice.logicalDevice, vertModule, nullptr);
}

void VulkanRenderer::CreateFramebuffers()
{
	swapchainFramebuffers.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainFramebuffers.size(); ++i)
	{
		std::array<VkImageView, 1> attachments = { swapchainImages[i].imageView };

		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.width = swapchainImgExtent.width;
		createInfo.height = swapchainImgExtent.height;
		createInfo.layers = 1;

		auto result = vkCreateFramebuffer(mainDevice.logicalDevice, &createInfo, nullptr, &swapchainFramebuffers[i]);
		if (result != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer");
	}
}

void VulkanRenderer::CreateCommandPool()
{
	auto indices = GetQueueFamilyIndices(mainDevice.physicalDevice);

	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = indices.graphicsFamily;

	auto result = vkCreateCommandPool(mainDevice.logicalDevice, &createInfo, nullptr, &graphicsCommandPool);
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics command pool");
}

void VulkanRenderer::CreateCommandBuffers()
{
	commandBuffers.resize(swapchainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = graphicsCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	auto result = vkAllocateCommandBuffers(mainDevice.logicalDevice, &allocInfo, commandBuffers.data());
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to allocate primary cmd buffers");
}

void VulkanRenderer::CreateSyncObjects()
{
	semsImgAvailable.resize(MAX_QUEUED_DRAWS);
	semsRenderFinished.resize(MAX_QUEUED_DRAWS);
	drawFences.resize(MAX_QUEUED_DRAWS);

	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_QUEUED_DRAWS; ++i)
	{
		if (vkCreateSemaphore(mainDevice.logicalDevice, &createInfo, nullptr, &semsImgAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(mainDevice.logicalDevice, &createInfo, nullptr, &semsRenderFinished[i]) != VK_SUCCESS ||
			vkCreateFence(mainDevice.logicalDevice, &fenceInfo, nullptr, &drawFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create a sync object");
		}
	}
}

void VulkanRenderer::RecordCommands()
{
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkRenderPassBeginInfo rpBeginInfo = {};
	rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBeginInfo.renderPass = renderPass;
	rpBeginInfo.renderArea.offset = { 0, 0 };
	rpBeginInfo.renderArea.extent = swapchainImgExtent;
	VkClearValue clearValues[] =
	{
		{0.6f, 0.65f, 0.4f, 1.0f}
	};
	rpBeginInfo.pClearValues = clearValues;
	rpBeginInfo.clearValueCount = 1;

	for (size_t i = 0; i < commandBuffers.size(); ++i)
	{
		auto result = vkBeginCommandBuffer(commandBuffers[i], &bufferBeginInfo);
		if (result != VK_SUCCESS)
			throw std::runtime_error("failed to start recording cmd buffer");

		rpBeginInfo.framebuffer = swapchainFramebuffers[i];
		vkCmdBeginRenderPass(commandBuffers[i], &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkBuffer vertBuffers[] = { firstMesh.GetVertexBuffer() };
			std::cout << firstMesh.GetVertexBuffer() << "  " << firstMesh.GetVertexCount() << std::endl;
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertBuffers, offsets);

			vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(firstMesh.GetVertexCount()), 1, 0, 0);
		}
		vkCmdEndRenderPass(commandBuffers[i]);

		result = vkEndCommandBuffer(commandBuffers[i]);
		if (result != VK_SUCCESS)
			throw std::runtime_error("failed to end recording cmd buffer");
	}
}

void VulkanRenderer::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainImgFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference attRef = {};
	attRef.attachment = 0; //idx in attachments list
	attRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attRef;

	std::array<VkSubpassDependency, 2> spDependencies;

	spDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	spDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	spDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	spDependencies[0].dstSubpass = 0;
	spDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	spDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	spDependencies[0].dependencyFlags = 0;

	spDependencies[1].srcSubpass = 0;
	spDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	spDependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	spDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	spDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	spDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	spDependencies[1].dependencyFlags = 0;

	VkRenderPassCreateInfo rpCreateInfo = {};
	rpCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rpCreateInfo.attachmentCount = 1;
	rpCreateInfo.pAttachments = &colorAttachment;
	rpCreateInfo.subpassCount = 1;
	rpCreateInfo.pSubpasses = &subpass;
	rpCreateInfo.dependencyCount = static_cast<uint32_t>(spDependencies.size());
	rpCreateInfo.pDependencies = spDependencies.data();

	auto result = vkCreateRenderPass(mainDevice.logicalDevice, &rpCreateInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass");
}

void VulkanRenderer::GetPhysicalDevice()
{
	uint32_t devCount = 0;
	auto countAttemptResult = vkEnumeratePhysicalDevices(vkInstance, &devCount, nullptr);

	if (devCount <= 0)
		throw std::runtime_error("no supported gpus");

	std::vector<VkPhysicalDevice> devList(devCount);
	vkEnumeratePhysicalDevices(vkInstance, &devCount, devList.data());

	for (const auto& d : devList)
	{
		if (CheckDeviceSuitable(d))
		{
			mainDevice.physicalDevice = d;
			break;
		}
	}
}

QueueFamilyIndices VulkanRenderer::GetQueueFamilyIndices(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t qFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &qFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyList(qFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &qFamilyCount, queueFamilyList.data());

	int i = 0;
	for (const auto& qf : queueFamilyList)
	{
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);

		if (qf.queueCount > 0)
		{
			if (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphicsFamily = i;

			if (presentationSupport)
				indices.presentationFamily = i;
		}
		
		if (indices.IsValid())
			break;

		++i;
	}

	return indices;
}

SwapchainProperties VulkanRenderer::GetSwapchainProperties(VkPhysicalDevice device)
{
	SwapchainProperties result;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &result.surfaceCapabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount > 0)
	{
		result.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, result.formats.data());
	}

	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);
	if (presentationCount > 0)
	{
		result.presentModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, result.presentModes.data());
	}

	return result;
}

bool VulkanRenderer::CheckInstanceExtensionSupport(std::vector<const char*>* extsToCheck)
{
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> supportedExts(extCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, supportedExts.data());

	for (const auto& extCheck : *extsToCheck)
	{
		bool found = false;
		for (const auto& extSupported : supportedExts)
		{
			if (strcmp(extCheck, extSupported.extensionName))
			{
				found = true;
				break;
			}
		}

		if (!found)
			return false;
	}
	return true;
}

bool VulkanRenderer::CheckDeviceSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties devProperties;
	vkGetPhysicalDeviceProperties(device, &devProperties);

	VkPhysicalDeviceFeatures devFeatures;
	vkGetPhysicalDeviceFeatures(device, &devFeatures);

	auto qIndices = GetQueueFamilyIndices(device);
	auto hasExtSupport = CheckDeviceExtensionSupport(device);
	auto isSwapchainValid = false;

	if (hasExtSupport)
	{
		const auto details = GetSwapchainProperties(device);
		isSwapchainValid = !details.presentModes.empty() && !details.formats.empty();
	}

	return qIndices.IsValid() && hasExtSupport && isSwapchainValid;
}

VkSurfaceFormatKHR VulkanRenderer::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	/*
	for (const auto& f : formats)
	{
		std::cout << std::endl;
		std::cout << f.format;
	}
	std::cout << std::endl;
	*/

	for (const auto& f : formats)
	{
		if ((f.format == VK_FORMAT_R8G8B8A8_UNORM || f.format == VK_FORMAT_B8G8R8A8_UNORM)
			&& f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return f;
		}
	}

	std::cout << "fallback format";
	return formats[0];
}

VkPresentModeKHR VulkanRenderer::ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	for (const auto& mode : presentationModes)
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::ChooseExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;
	}
	else
	{
		int wid, hei;
		glfwGetFramebufferSize(window, &wid, &hei);

		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(wid);
		newExtent.height = static_cast<uint32_t>(hei);

		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width,
			std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));

		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
			std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

		return newExtent;
	}
}

VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.format = format;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	auto result = vkCreateImageView(mainDevice.logicalDevice, &createInfo, nullptr, &imageView);
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to create image view");
	return imageView;
}

VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& shader)
{
	VkShaderModuleCreateInfo createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shader.size();
	createInfo.pCode = reinterpret_cast<const uint32_t *>(shader.data());

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(mainDevice.logicalDevice, &createInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module");

	return shaderModule;
}