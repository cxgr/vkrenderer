#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

#include "VulkanRenderer.h"

GLFWwindow* window;
VulkanRenderer renderer;

void InitWindow(std::string name = "test", const int wid = 1600, const int hei = 900)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, 0);
	glfwWindowHint(GLFW_RESIZABLE, false);

	window = glfwCreateWindow(wid, hei, name.c_str(), nullptr, nullptr);
}

int main()
{
	InitWindow();

	if (renderer.Init(window) == 1)
		return 1;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		renderer.Draw();
	}

	renderer.Cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}