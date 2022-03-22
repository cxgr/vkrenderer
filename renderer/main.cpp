#define GLFW_INCLUDE_VULKAN
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

	if (renderer.Init(window) == EXIT_FAILURE)
		return EXIT_FAILURE;

	auto angle = 0.0f;
	auto deltaTime = 0.0f;
	auto lastTime = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		float now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		angle += 10 * deltaTime;
		if (angle > 360.0f)
			angle -= 360.0f;

		renderer.UpdateModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)));

		if (0 == glfwGetWindowAttrib(window, GLFW_ICONIFIED))
			renderer.Draw();
	}

	renderer.Cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}