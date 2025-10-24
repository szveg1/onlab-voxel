#include "GLApp.h"

#include <algorithm>
#include <memory>
#include <nfd.h>
#include <set>
#include <print>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>

#include "Brush.h"
#include "Camera.h"
#include "GPUProgram.h"
#include "QuadGeometry.h"
#include "SVDAGEditor.h"
#include "SVDAGLoader.h"
#include "Texture.h"

using namespace glm;

class VoxelApp : public GLApp {
    std::unique_ptr<QuadGeometry> quad;
    std::unique_ptr<GPUProgram> drawProgram;
    std::unique_ptr<Shader> renderShader;
    std::unique_ptr<GPUProgram> renderProgram;
    std::unique_ptr<Texture> scene;

    std::shared_ptr<Camera> camera;
    std::shared_ptr<SVDAGLoader> svdagLoader;
    std::shared_ptr<SVDAGEditor> svdagEditor;
    std::unique_ptr<Brush> brush;
    std::set<int> keysPressed;
    float deltaTime, lastFrame;
    bool needsUpload = false;
    bool cameraLock = false;

    bool visualizeSteps = false;
    int mousePressed = -1;
    float brushSize = 0.005f;
    vec3 brushColor = vec3(1);

public:
    VoxelApp() : GLApp("Voxel app")
    {
        deltaTime = 0.0f;
        lastFrame = 0.0f;
    }

    void onInit()
    {
        quad = std::make_unique<QuadGeometry>();
        auto vertexShader = std::make_unique<Shader>(GL_VERTEX_SHADER, "shaders\\quad.vert");
        auto fragmentShader = std::make_unique<Shader>(GL_FRAGMENT_SHADER, "shaders\\textured.frag");
        drawProgram = std::make_unique<GPUProgram>(vertexShader.get(), fragmentShader.get());

        camera = std::make_shared<Camera>();

        svdagLoader = std::make_shared<SVDAGLoader>();

        svdagEditor = std::make_shared<SVDAGEditor>(svdagLoader->getNodes(), svdagLoader->getDepth());

        brush = std::make_unique<Brush>(camera, svdagLoader, svdagEditor);

        renderShader = std::make_unique<Shader>(GL_COMPUTE_SHADER, "shaders\\render.comp");
        renderProgram = std::make_unique<GPUProgram>(renderShader.get());
        renderProgram->use();
        renderProgram->setUniform(visualizeSteps, "visualizeSteps");
        renderProgram->setUniform(svdagLoader->getDepth(), "treeDepth");

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        scene = std::make_unique<Texture>(width, height);
        glViewport(0, 0, width, height);
    }

    void onDisplay()
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float fps = 1 / deltaTime;

        ImGui::Begin("Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("delta: %f\n%.1f FPS", deltaTime, fps);
        ImGui::Text("Camera position: (%.2f, %.2f, %.2f)", camera->Position().x, camera->Position().y, camera->Position().z);
        ImGui::Text("mouse button: %s", mousePressed == MOUSE_RIGHT ? "right" : mousePressed == MOUSE_LEFT ? "left" : mousePressed == MOUSE_MIDDLE ? "middle" : "none");
        ImGui::Text("brush size: %.5f", brushSize);
        ImGui::Text("r to reload shader");
        ImGui::Text("t to visualize raycasting steps (blue = less, yellow = more)");
        ImGui::Text("u to unlock cursor, l to lock");
        ImGui::End();

        
        ImGui::Begin("Color picker", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::ColorPicker3("Brush color", &brushColor.x);
        ImGui::End();

		ImGui::Begin("Load", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::Button("Load SVDAG")) {
			nfdu8char_t* outPath = nullptr;
			nfdu8filteritem_t filters[1] = { { "svdag files", "bin" } };
            nfdopendialogu8args_t args = { 0 };
			args.filterList = filters;
			args.filterCount = 1;
			nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
            if (result == NFD_OKAY) {
                svdagLoader->load(std::string(outPath));
                svdagLoader->uploadToGPU();

                svdagEditor = std::make_shared<SVDAGEditor>(svdagLoader->getNodes(), svdagLoader->getDepth());

                brush = std::make_unique<Brush>(camera, svdagLoader, svdagEditor);

                renderProgram->use();
                renderProgram->setUniform(svdagLoader->getDepth(), "treeDepth");
            } else if (result == NFD_CANCEL) {
				std::print("User pressed cancel. \n");
            } else {
                std::print("Error: {} \n", NFD_GetError());
            }
        }
        ImGui::End();

        uint16_t r = static_cast<uint16_t>(brushColor.r * 31.0f);
        uint16_t g = static_cast<uint16_t>(brushColor.g * 63.0f);
        uint16_t b = static_cast<uint16_t>(brushColor.b * 31.0f);

        uint16_t material = (r << 11) | (g << 5) | b;


        glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!cameraLock) 
        camera->move(keysPressed, deltaTime);


        if (keysPressed.count('r')) {
            renderShader = std::make_unique<Shader>(GL_COMPUTE_SHADER, "shaders\\render.comp");
            renderProgram = std::make_unique<GPUProgram>(renderShader.get());
            renderProgram->use();
            renderProgram->setUniform(visualizeSteps, "visualizeSteps");
            renderProgram->setUniform(svdagLoader->getDepth(), "treeDepth");
        }
        if (keysPressed.count('t')) {
            visualizeSteps = !visualizeSteps;
            renderProgram->use();
            renderProgram->setUniform(visualizeSteps, "visualizeSteps");
			keysPressed.erase('t');
        }
        if (keysPressed.count('u')) {
            glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cameraLock = true;
        }
        if (keysPressed.count('l')) {
            glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            cameraLock = false;
        }

		BrushData brushData = brush->getBrushData();
        vec3 brushCenter = brushData.position;
        if ((mousePressed == MOUSE_LEFT || mousePressed == MOUSE_RIGHT) && !cameraLock) {
            if (brushCenter.x != -999.0f) {
                bool isAdding = (mousePressed == MOUSE_LEFT);
                brush->apply(brushCenter, brushSize, isAdding, material);
                brush->uploadChangesToGPU();
                needsUpload = true;
            }
        }


        if (needsUpload) {
            brush->uploadChangesToGPU();
            needsUpload = false;
        }


        renderProgram->use();
        renderProgram->setUniform(camera->RayDirMatrix(), "camera.rayDirMatrix");
        renderProgram->setUniform(camera->Position(), "camera.position");
        renderProgram->setUniform(visualizeSteps, "visualizeSteps");
        renderProgram->setUniform(brushSize, "brushSize");
        renderProgram->setUniform(brushColor, "brushColor");
        renderProgram->setUniform(brushCenter, "brushCenter");

        svdagLoader->bindNodes(0);
        scene->bindCompute(1);

        glDispatchCompute(width / 8, height / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        drawProgram->use();
        scene->bind(0);
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
        if (!cameraLock) camera->mouseMove(x, y);
    }

    void onMousePressed(MouseButton but, int pX, int pY) override
    {
        mousePressed = but;
    }

    void onMouseReleased(MouseButton but, int pX, int pY) override
    {
        mousePressed = -1;
    }

    void onMouseScrolled(double xoffset, double yoffset) override
    {
        brushSize += static_cast<float>(yoffset) * 0.0005f;
        brushSize = std::clamp(brushSize, 0.0f, 0.5f);
    }
};


VoxelApp app;