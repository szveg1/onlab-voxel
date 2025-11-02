#include "GLApp.h"

#include <algorithm>
#include <future>
#include <glm/glm.hpp>
#include <imgui_impl_opengl3.h>
#include <memory>
#include <nfd.h>
#include <print>
#include <set>

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
    float brushSize = 0.005f;
    vec3 brushColor = vec3(1);
	BrushMode brushMode = SPHERE;
    vec3 firstCorner = vec3(-999.0f);
	bool firstCornerSet = false;

    std::set<int> keysPressed;
    float deltaTime, lastFrame;
    bool cameraLock = false;
    MouseButton mousePressed = NONE;
    MouseButton previousMouseButton = NONE;
    size_t worldResolution = 0;

    bool showInfo = false;
	bool showHelp = false;
	bool showColorPicker = false;
    bool showBrushMode = false;
    bool isWorldLoading = false;
    
    bool needsUpload = false;
    bool visualizeSteps = false;

	std::future<void> loadingTask;
	std::string pendingFilePath;
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

        if (isWorldLoading && loadingTask.valid()) {
            if (loadingTask.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                loadingTask.get();
                finishLoading();
            }
		}

        if (showInfo) {
            showInfoWindow();
        }
        if (showHelp) {
            showHelpWindow();
		}
        if (showColorPicker) {
            showColorPickerWindow();
        }
        if (isWorldLoading) {
            showLoadingWindow();
        }

        showMainMenuBar();

        glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		BrushData brushData = brush->getBrushData();

        if (!cameraLock) {
            camera->move(keysPressed, deltaTime);
			applyBrush(brushData);
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
        renderProgram->setUniform(brushData.position, "brushCenter");
        renderProgram->setUniform(firstCornerSet, "selectingBox");
        renderProgram->setUniform(firstCorner, "firstCorner");

        svdagLoader->bindNodes(0);
        scene->bindCompute(1);

        glDispatchCompute(width / 8, height / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        drawProgram->use();
        scene->bind(0);
        quad->draw();

        previousMouseButton = mousePressed;
    }

    void onKeyboard(int key) override
    {
        switch (key) {
        case 't' :
            visualizeSteps = !visualizeSteps;
            break;
        case 'r' :
			reloadShader();
			break;
        case GLFW_KEY_ESCAPE:
            if (cameraLock) {
                lockMouse();
            }
            else {
                unlockMouse();
            }
			break;
        }
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
        mousePressed = NONE;
    }

    void onMouseScrolled(double xoffset, double yoffset) override
    {
        brushSize += static_cast<float>(yoffset) * 0.0005f;
        brushSize = std::clamp(brushSize, 0.0f, 0.5f);
    }

    void applyBrush(BrushData brushData) {
        vec3 brushCenter = brushData.position;

        if ((mousePressed == MOUSE_LEFT || mousePressed == MOUSE_RIGHT) && !isWorldLoading) {
            if (brushCenter.x != -999.0f) {
                bool isAdding = (mousePressed == MOUSE_LEFT);

                uint16_t r = static_cast<uint16_t>(brushColor.r * 31.0f);
                uint16_t g = static_cast<uint16_t>(brushColor.g * 63.0f);
                uint16_t b = static_cast<uint16_t>(brushColor.b * 31.0f);
                 
                uint16_t material = (r << 11) | (g << 5) | b;

                switch (brushMode) {
                    case PAINT:
						brush->paint(brushCenter, brushSize, material);
                        break;
                    case SPHERE:
                        brush->sphere(brushCenter, brushSize, isAdding, material);
                        break;
                    case BOX:
                        if (previousMouseButton == NONE) {
                            static vec3 boxMin, boxMax;
                            if (!firstCornerSet) {
                                firstCorner = brushCenter;
                                firstCornerSet = true;
                            }
                            else {
                                firstCornerSet = false;
                                boxMin = glm::min(firstCorner, brushCenter);
                                boxMax = glm::max(firstCorner, brushCenter);
                                brush->box(boxMin, boxMax, isAdding, material);
                                firstCorner = vec3(-999.0f);
                            }
                        }
                        break;
                }
                
                brush->uploadChangesToGPU();
                needsUpload = true;
            }
        }
    }

    void showMainMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Load world")) {
                    showFileDialog();
                }
                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(this->window, GLFW_TRUE);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Brush")) {
                if (ImGui::MenuItem("Open color picker")) {
                    showColorPicker = true;
                }
                if (ImGui::BeginMenu("Mode")) {
                    if (ImGui::MenuItem("Sphere", nullptr, brushMode == SPHERE)) {
                        brushMode = SPHERE;
                    }
                    if (ImGui::MenuItem("Paint", nullptr, brushMode == PAINT)) {
                        brushMode = PAINT;
                    }
                    if (ImGui::MenuItem("Box", nullptr, brushMode == BOX)) {
                        brushMode = BOX;
                    }
					ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Stats & Info")) {
                if (ImGui::MenuItem("Show")) {
                    showInfo = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Show")) {
                    showHelp = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void showInfoWindow()
    {
        float fps = 1 / deltaTime;

        ImGui::Begin("Stats & Info", &showInfo, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Delta: %f\n%.1f FPS", deltaTime, fps);
        ImGui::Text("Camera position: (%.2f, %.2f, %.2f)", camera->Position().x, camera->Position().y, camera->Position().z);
        ImGui::Text("Mouse button: %s", mousePressed == MOUSE_RIGHT ? "right" : mousePressed == MOUSE_LEFT ? "left" : mousePressed == MOUSE_MIDDLE ? "middle" : "none");
        ImGui::Text("Brush size: %.5f", brushSize);
        ImGui::Text("Resolution: %zu", worldResolution);
        ImGui::End();
	}

    void showHelpWindow() {
        ImGui::Begin("Help", &showHelp, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("r to reload shader");
        ImGui::Text("t to visualize raycasting steps (blue = less, yellow = more)");
        ImGui::End();
    }

    void showColorPickerWindow() {
        ImGui::Begin("Color picker", &showColorPicker, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::ColorPicker3("Brush color", &brushColor.x);
        ImGui::End();
    }

    void showLoadingWindow() {
        ImGui::OpenPopup("Loading world");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Loading world", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
			ImGui::ProgressBar(ImGui::GetTime() * -1.5f, ImVec2(0.0f, 0.0f));
            ImGui::EndPopup();
        }
    }

    void showFileDialog() {
        nfdu8char_t* outPath = nullptr;
        nfdu8filteritem_t filters[1] = { { "SVDAG files", "dag" } };
        nfdopendialogu8args_t args = { 0 };
        args.filterList = filters;
        args.filterCount = 1;
        nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
        if (result == NFD_OKAY) {
            pendingFilePath = std::string(outPath);
            NFD_FreePathU8(outPath);
            startAsyncLoading();
        }
    }

    void startAsyncLoading() {
        isWorldLoading = true;
        loadingTask = std::async(std::launch::async, [this]() {
            svdagLoader->load(pendingFilePath);
            });
    }

    void finishLoading() {
        svdagLoader->uploadToGPU();

        worldResolution = static_cast<size_t>(powf(2.0f, svdagLoader->getDepth()));

        svdagEditor = std::make_shared<SVDAGEditor>(svdagLoader->getNodes(), svdagLoader->getDepth());
        brush = std::make_unique<Brush>(camera, svdagLoader, svdagEditor);

        renderProgram->use();
        renderProgram->setUniform(svdagLoader->getDepth(), "treeDepth");

        isWorldLoading = false;
    }

    void reloadShader() {
        renderShader = std::make_unique<Shader>(GL_COMPUTE_SHADER, "shaders\\render.comp");
        renderProgram = std::make_unique<GPUProgram>(renderShader.get());
        renderProgram->use();
        renderProgram->setUniform(visualizeSteps, "visualizeSteps");
        renderProgram->setUniform(svdagLoader->getDepth(), "treeDepth");
    }

    void lockMouse() {
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        cameraLock = false;
	}

    void unlockMouse() {
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        cameraLock = true;
	}
};


VoxelApp app;