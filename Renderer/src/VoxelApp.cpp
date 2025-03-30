#include "GLApp.h"

#include <memory>
#include <set>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include "GPUProgram.h"
#include "QuadGeometry.h"
#include "Camera.h"
#include "SVOLoader.h"
#include "Texture.h"

using namespace glm;
class VoxelApp : public GLApp {
	std::unique_ptr<QuadGeometry> quad;
	std::unique_ptr<GPUProgram> gpuProgram;
	std::unique_ptr<GPUProgram> computeProgram;
	std::unique_ptr<Camera> camera;
	std::unique_ptr<SVOLoader> svoLoader;
	std::unique_ptr<Texture> image;
	std::set<int> keysPressed;
	float deltaTime, lastFrame;
	bool mousePressed = false;
	float brushSize = 0.0001f;
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
		std::unique_ptr<Shader> fragmentShader = std::make_unique<Shader>(GL_FRAGMENT_SHADER, "shaders\\textured.frag");
		std::unique_ptr<Shader> computeShader = std::make_unique<Shader>(GL_COMPUTE_SHADER, "shaders\\raycast.comp");
		gpuProgram = std::make_unique<GPUProgram>(vertexShader.get(), fragmentShader.get());
		computeProgram = std::make_unique<GPUProgram>(computeShader.get());
		camera = std::make_unique<Camera>();
		glViewport(0, 0, width, height);
		svoLoader = std::make_unique<SVOLoader>();
		svoLoader->load();
		svoLoader->uploadToGPU();
		image = std::make_unique<Texture>(width, height);
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

    computeProgram->use();
	computeProgram->setUniform(camera->RayDirMatrix(), "camera.rayDirMatrix");
	computeProgram->setUniform(camera->Position(), "camera.position");
	computeProgram->setUniform(camera->Ahead(), "camera.ahead");
	computeProgram->setUniform(svoLoader->getDepth(), "treeDepth");
	computeProgram->setUniform(mousePressed, "mousePressed");
	computeProgram->setUniform(brushSize, "brushSize");
    image->bind(1); 
	image->bindCompute(1);
    glDispatchCompute(width / 8, height / 8, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


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

	void onMousePressed(MouseButton but, int pX, int pY) override 
	{
		if (but == MOUSE_LEFT)
		{
			mousePressed = true;
		}
	}

	void onMouseReleased(MouseButton but, int pX, int pY) override 
	{
		if (but == MOUSE_LEFT)
		{
			mousePressed = false;
		}
	}

	void onMouseScrolled(double xoffset, double yoffset) override
	{
		brushSize += static_cast<float>(yoffset) * 0.0005f;
	}
};


VoxelApp app;