#include "GLApp.h"

#include <memory>
#include <set>
#include "GPUProgram.h"
#include "QuadGeometry.h"
#include "Camera.h"
#include <imgui_impl_opengl3.h>
#include "SVOBuilder.h"


class VoxelApp : public GLApp {
	std::unique_ptr<QuadGeometry> quad;
	std::unique_ptr<GPUProgram> gpuProgram;
	std::unique_ptr<Camera> camera;
	std::unique_ptr<SVOBuilder> svoBuilder;
	std::set<int> keysPressed;
	float deltaTime, lastFrame;
	glm::vec3 lightPosition = glm::vec3(0.0f, 0.0f, 0.0f);
public:
	VoxelApp() : GLApp("Voxel app") 
	{
		deltaTime = 0.0f;
		lastFrame = 0.0f;
	}
	
	void onInit() 
	{
		quad = std::make_unique<QuadGeometry>();
		std::unique_ptr<Shader> vertexShader = std::make_unique<Shader>(GL_VERTEX_SHADER, "shaders\\quad.vert");
		std::unique_ptr<Shader> fragmentShader = std::make_unique<Shader>(GL_FRAGMENT_SHADER, "shaders\\raycast.frag");
		gpuProgram = std::make_unique<GPUProgram>(vertexShader.get(), fragmentShader.get());
		camera = std::make_unique<Camera>();
		glViewport(0, 0, width, height);
		svoBuilder  = std::make_unique<SVOBuilder>(1024u, 256u);
		svoBuilder->build();
	}

	void onDisplay() 
	{
		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		float fps = 1 / deltaTime;

		// Calculate the size needed for the text
		ImVec2 textSize = ImGui::CalcTextSize("delta: 0.000000\n0.000000 FPS");

		// Set the next window size to fit the text
		ImGui::SetNextWindowSize(ImVec2(textSize.x + 20, textSize.y + 20)); // Add some padding

		// Create a window in the upper left corner
		ImGui::Begin("Info", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		ImGui::SetWindowPos(ImVec2(10, 10)); // Position the window at the top-left corner
		ImGui::Text("delta: %f\n%.1f FPS", deltaTime, fps);
		ImGui::End();


		glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		camera->move(keysPressed, deltaTime);

		gpuProgram->use();

		gpuProgram->setUniform(camera->RayDirMatrix(), "camera.rayDirMatrix");
		gpuProgram->setUniform(camera->Position(), "camera.position");
		gpuProgram->setUniform(svoBuilder->getDepth(), "treeDepth");
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