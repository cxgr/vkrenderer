#define GLFW_INCLUDE_VULKAN
#define	GLM_FORCE_DEPTH_ZERO_TO_ONE
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

		glm::mat4 model0(1.0f);
		glm::mat4 model1(1.0f);

		model0 = glm::translate(model0, glm::vec3(-.2f, 0.0f, -5.0f));
		model0 = glm::rotate(model0, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));

		model1 = glm::translate(model1, glm::vec3(.2f, 0.0f, -2.0f));
		model1 = glm::rotate(model1, glm::radians(-angle * 10), glm::vec3(0.0f, 0.0f, 1.0f));

		renderer.UpdateModel(0, model0);
		renderer.UpdateModel(1, model1);

		if (0 == glfwGetWindowAttrib(window, GLFW_ICONIFIED))
			renderer.Draw();
	}

	renderer.Cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}