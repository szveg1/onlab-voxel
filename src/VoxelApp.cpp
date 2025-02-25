#include "GLApp.h"

#include <memory>
#include <set>
#include "GPUProgram.h"
#include "QuadGeometry.h"
#include "Camera.h"
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

class VoxelApp : public GLApp {
	std::unique_ptr<QuadGeometry> quad;
	std::unique_ptr<GPUProgram> gpuProgram;
	std::unique_ptr<Camera> camera;
	std::set<int> keysPressed;
	float deltaTime, lastFrame;
public:
	VoxelApp() : GLApp("Voxel app") 
	{
		deltaTime = 0.0f;
		lastFrame = 0.0f;
	}
	
	void onInit() 
	{
		quad = std::make_unique<QuadGeometry>();
		Shader vertexShader = Shader(GL_VERTEX_SHADER, "shaders\\quad.vert");
		Shader fragmentShader = Shader(GL_FRAGMENT_SHADER, "shaders\\raycast.frag");
		gpuProgram = std::make_unique<GPUProgram>(vertexShader, fragmentShader);
		camera = std::make_unique<Camera>();
	}

	void onDisplay() 
	{
		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glClearColor(1.0f, 0.0f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera->move(keysPressed, deltaTime);
		
		gpuProgram->setUniform(camera->RayDirMatrix(), "camera.rayDirMatrix");
		gpuProgram->setUniform(camera->Position(), "camera.position");
		
			
		gpuProgram->use();
		quad->draw();
	}
	
	void onKeyboard(int key) override
	{
		keysPressed.insert(key);
	}

	void onKeyboardUp(int key) override
	{
		keysPressed.erase(key);
	}

	void window_size_callback(GLFWwindow* window, int width, int height) override
	{
		GLApp::window_size_callback(window, width, height);
		camera->setAspectRatio((float)width / (float)height);
	}

	void onMouseMotion(double x, double y) override 
	{
		camera->mouseMove(x, y);
	}
};


VoxelApp app;