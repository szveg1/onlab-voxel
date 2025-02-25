#include "GLApp.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <glad/glad.h>
#include <functional>


GLApp* GLApp::instance = nullptr;

GLApp::GLApp(const char* title)
{
	this->versionMajor = 3;
	this->versionMinor = 3;
	this->width = 800;
	this->height = 600;
	this->title = title;
	instance = this;
}

GLApp::~GLApp()
{

	glfwDestroyWindow(window);
	window = nullptr;
	instance = nullptr;
}

void GLApp::run()
{
	glfwSetErrorCallback(errorCallback);

	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, instance->versionMajor);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, instance->versionMinor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	instance->window = glfwCreateWindow(instance->width, instance->height, instance->title, NULL, NULL);
	if (instance->window == nullptr)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

    glfwSetWindowSizeCallback(instance->window, 
		[](GLFWwindow* window, int width, int height) 
		{
			instance->window_size_callback(window, width, height);
		}
	);

	glfwSetKeyCallback(instance->window, &keyCallback);
	glfwSetInputMode(instance->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(instance->window, cursorPosCallback);
	glfwSetMouseButtonCallback(instance->window, mouseButtonCallback);

	glfwMakeContextCurrent(instance->window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		exit(EXIT_FAILURE);
	}

	int framebufferWidth, framebufferHeight;
	glfwGetFramebufferSize(instance->window, &framebufferWidth, &framebufferHeight);

	glViewport(0, 0, framebufferWidth, framebufferHeight);

	instance->onInit();
	

	while (!glfwWindowShouldClose(instance->window))
	{
		instance->onDisplay();

		glfwSwapBuffers(instance->window);
		glfwPollEvents();
	}



}

void GLApp::errorCallback(int error, const char* description)
{
	std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

void GLApp::window_size_callback(GLFWwindow* window, int width, int height)
{
	int framebufferWidth, framebufferHeight;
	glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

	glViewport(0, 0, framebufferWidth, framebufferHeight);
}


void GLApp::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
		return;
	}
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if ((mods & GLFW_MOD_SHIFT) == 0 && key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
			key += 'a' - 'A';
		}
		instance->onKeyboard(key);
	}
	if (action == GLFW_RELEASE) {
		if ((mods & GLFW_MOD_SHIFT) == 0 && key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
			key += 'a' - 'A';
		}
		instance->onKeyboardUp(key);
	}
}

void GLApp::cursorPosCallback(GLFWwindow* window, double x, double y)
{
	instance->onMouseMotion(x, y);
}

void GLApp::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	double pX, pY;
	glfwGetCursorPos(instance->window, &pX, &pY);
	if (action == GLFW_PRESS) instance->onMousePressed((button == GLFW_MOUSE_BUTTON_LEFT) ? MOUSE_LEFT : MOUSE_RIGHT, (int)pX, (int)pY);
	else				      instance->onMouseReleased((button == GLFW_MOUSE_BUTTON_LEFT) ? MOUSE_LEFT : MOUSE_RIGHT, (int)pX, (int)pY);
}
