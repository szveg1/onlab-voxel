#include "GLApp.h"

#include <algorithm>
#include <memory>
#include <set>
#include <print>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>

#include "GPUProgram.h"
#include "QuadGeometry.h"
#include "Camera.h"
#include "SVDAGLoader.h"
#include "Texture.h"
#include "SVDAGEditor.h"

using namespace glm;

class VoxelApp : public GLApp {
    std::unique_ptr<QuadGeometry> quad;
    std::unique_ptr<GPUProgram> gpuProgram;
    std::unique_ptr<Shader> renderShader;
    std::unique_ptr<GPUProgram> renderProgram;
    std::unique_ptr<Shader> editShader;
    std::unique_ptr<GPUProgram> editProgram;
    std::unique_ptr<Shader> brushShader;
    std::unique_ptr<GPUProgram> brushProgram;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<SVDAGLoader> svdagLoader;
    std::unique_ptr<SVDAGEditor> voxelEditor;
    std::unique_ptr<Texture> image;
    std::set<int> keysPressed;
    float deltaTime, lastFrame;

    bool visualizeSteps = false;
    int mousePressed = -1;
    float brushSize = 0.0001f;


    GLuint brushPosBufferID;

public:
    VoxelApp() : GLApp("Voxel app")
    {
        deltaTime = 0.0f;
        lastFrame = 0.0f;
    }

    ~VoxelApp() {
        glDeleteBuffers(1, &brushPosBufferID);
    }

    void onInit()
    {
        quad = std::make_unique<QuadGeometry>();
        camera = std::make_unique<Camera>();

        svdagLoader = std::make_unique<SVDAGLoader>();
        svdagLoader->load();
        svdagLoader->uploadToGPU();

        voxelEditor = std::make_unique<SVDAGEditor>(svdagLoader->getNodes(), svdagLoader->getDepth());

        auto vertexShader = std::make_unique<Shader>(GL_VERTEX_SHADER, "shaders\\quad.vert");
        auto fragmentShader = std::make_unique<Shader>(GL_FRAGMENT_SHADER, "shaders\\textured.frag");
        gpuProgram = std::make_unique<GPUProgram>(vertexShader.get(), fragmentShader.get());

        renderShader = std::make_unique<Shader>(GL_COMPUTE_SHADER, "shaders\\render.comp");
        renderProgram = std::make_unique<GPUProgram>(renderShader.get());
        renderProgram->use();
        renderProgram->setUniform(visualizeSteps, "visualizeSteps");
        renderProgram->setUniform(svdagLoader->getDepth(), "treeDepth");

        editShader = std::make_unique<Shader>(GL_COMPUTE_SHADER, "shaders\\edit.comp");
        editProgram = std::make_unique<GPUProgram>(editShader.get());
        editProgram->use();
        editProgram->setUniform(svdagLoader->getDepth(), "treeDepth");

        brushShader = std::make_unique<Shader>(GL_COMPUTE_SHADER, "shaders\\brushData.comp");
        brushProgram = std::make_unique<GPUProgram>(brushShader.get());
        brushProgram->use();
        brushProgram->setUniform(svdagLoader->getDepth(), "treeDepth");

        glGenBuffers(1, &brushPosBufferID);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, brushPosBufferID);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4), NULL, GL_STREAM_READ);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        image = std::make_unique<Texture>(width, height);
        glViewport(0, 0, width, height);
    }

    void onDisplay()
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float fps = 1 / deltaTime;

        std::string placeholder = R"(
        delta: 0.000000
        0.00 FPS
        Camera position: (0.00, 0.00, 0.00)
        mouse button: right
        brush size: 0.00000
        r to reload shader
        t to visualize raycasting steps (blue = less, yellow = more))";

        // Calculate the size needed for the text
        ImVec2 textSize = ImGui::CalcTextSize(placeholder.c_str());

        // Set the next window size to fit the text
        ImGui::SetNextWindowSize(ImVec2(textSize.x + 20, textSize.y + 20)); // Add some padding

        // Create a window in the upper left corner
        ImGui::Begin("Info", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::SetWindowPos(ImVec2(10, 10)); // Position the window at the top-left corner
        ImGui::Text("delta: %f\n%.1f FPS", deltaTime, fps);
        ImGui::Text("Camera position: (%.2f, %.2f, %.2f)", camera->Position().x, camera->Position().y, camera->Position().z);
        ImGui::Text("mouse button: %s", mousePressed == MOUSE_RIGHT ? "right" : mousePressed == MOUSE_LEFT ? "left" : mousePressed == MOUSE_MIDDLE ? "middle" : "none");
        ImGui::Text("brush size: %.5f", brushSize);
        ImGui::Text("r to reload shader");
        ImGui::Text("t to visualize raycasting steps (blue = less, yellow = more)");
        ImGui::End();

        glClearColor(0.529f, 0.808f, 0.922f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera->move(keysPressed, deltaTime);


        if (keysPressed.count('r')) {
            renderShader = std::make_unique<Shader>(GL_COMPUTE_SHADER, "shaders\\raycast.comp");
            renderProgram = std::make_unique<GPUProgram>(renderShader.get());
        }
        if (keysPressed.count('t')) {
            visualizeSteps = !visualizeSteps;
            renderProgram->use();
            renderProgram->setUniform(visualizeSteps, "visualizeSteps");
            keysPressed.erase('t');
        }


        bool needsUpload = false;
        if (mousePressed == MOUSE_LEFT || mousePressed == MOUSE_RIGHT) {

            glm::vec3 center = getBrushCenter();

            if (center.x != -999.0f) {
                bool isAdding = (mousePressed == MOUSE_LEFT);
                applyBrushStroke(center, brushSize, isAdding);
                needsUpload = true;
            }
        }

        if (needsUpload) {
            uploadChangesToGPU();
        }


        renderProgram->use();
        renderProgram->setUniform(camera->RayDirMatrix(), "camera.rayDirMatrix");
        renderProgram->setUniform(camera->Position(), "camera.position");
        renderProgram->setUniform(visualizeSteps, "visualizeSteps");
        renderProgram->setUniform(brushSize, "brushSize");

        svdagLoader->bindNodes(0);
        image->bindCompute(1);

        glDispatchCompute(width / 8, height / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

       


        gpuProgram->use();
		image->bind(0);
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

    glm::vec3 getBrushCenter() {
        brushProgram->use();
        brushProgram->setUniform(camera->RayDirMatrix(), "camera.rayDirMatrix");
        brushProgram->setUniform(camera->Position(), "camera.position");

        svdagLoader->bindNodes(0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, brushPosBufferID);

        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, brushPosBufferID);
        glm::vec4* data = (glm::vec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        glm::vec3 position = glm::vec3(-999.0);
        if (data && data->w > 0.0) {
            position = glm::vec3(*data);
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        return position;
    }

    void performVoxelEdit(glm::vec3 center, float size, int action, uint material = 0x8410) {
        editProgram->use();

        const float voxelSize = 1.0f / pow(2.0f, svdagLoader->getDepth() - 1);
        const float brushDiameter = size * 2.0f;
        const GLuint numVoxelsToCover = static_cast<GLuint>(ceil(brushDiameter / voxelSize));

        glm::ivec3 brushGridBase = glm::ivec3(glm::floor(center / voxelSize)) - glm::ivec3(numVoxelsToCover / 2);

        editProgram->setUniform(action, "editAction");
        editProgram->setUniform(material, "editMaterial");
        editProgram->setUniform(center, "brushCenter");
        editProgram->setUniform(size, "brushSize");
        editProgram->setUniform(brushGridBase, "brushGridBase");

        svdagLoader->bindNodes(0);
        svdagLoader->bindCounter(1);

        const GLuint localSize = 4;
        const GLuint numWorkGroups = (numVoxelsToCover + localSize - 1) / localSize;

        glDispatchCompute(numWorkGroups, numWorkGroups, numWorkGroups);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void uploadChangesToGPU() {
        const auto& modified = voxelEditor->getModifiedIndices();
        const auto& nodes = svdagLoader->getNodes();
        size_t newNodesStart = voxelEditor->getNewNodesStartIndex();

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, svdagLoader->getNodeBufferID());

        for (uint32_t index : modified) {
            if (index < newNodesStart) {
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                    index * sizeof(SVDAGGPUNode),
                    sizeof(SVDAGGPUNode),
                    &nodes[index]);
            }
        }

        if (nodes.size() > newNodesStart) {
            glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                newNodesStart * sizeof(SVDAGGPUNode),
                (nodes.size() - newNodesStart) * sizeof(SVDAGGPUNode),
                &nodes[newNodesStart]);
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        voxelEditor->clearModifiedLists();
    }

    void applyBrushStroke(glm::vec3 center, float radius, bool isAdding) {
        const float voxelSize = 1.0f / exp2f(svdagLoader->getDepth());

        glm::vec3 minWorld = center - glm::vec3(radius);
        glm::vec3 maxWorld = center + glm::vec3(radius);

        glm::ivec3 minGrid = glm::floor(minWorld / voxelSize);
        glm::ivec3 maxGrid = glm::ceil(maxWorld / voxelSize);

        minGrid = glm::clamp(minGrid, glm::ivec3(0), glm::ivec3(1.0f / voxelSize - 1.0f));
        maxGrid = glm::clamp(maxGrid, glm::ivec3(0), glm::ivec3(1.0f / voxelSize - 1.0f));

        for (int z = minGrid.z; z <= maxGrid.z; ++z) {
            for (int y = minGrid.y; y <= maxGrid.y; ++y) {
                for (int x = minGrid.x; x <= maxGrid.x; ++x) {

                    glm::vec3 voxelPos = (glm::vec3(x, y, z) + 0.5f) * voxelSize;

                    if (glm::distance(voxelPos, center) <= radius) {

                        if (isAdding) {
                            uint16_t material = 0xF800;
                            voxelEditor->setVoxel(voxelPos, material);
                        }
                        else {
                            voxelEditor->clearVoxel(voxelPos);
                        }
                    }
                }
            }
        }
    }
};


VoxelApp app;