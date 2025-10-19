#include "Brush.h"

Brush::Brush(std::shared_ptr<Camera> camera, std::shared_ptr<SVDAGLoader> svdagLoader, std::shared_ptr<SVDAGEditor> svdagEditor)
	: camera(camera), svdagLoader(svdagLoader), svdagEditor(svdagEditor)
{
	brushShader = std::make_unique<Shader>(GL_COMPUTE_SHADER, "shaders/brushData.comp");
	brushProgram = std::make_unique<GPUProgram>(brushShader.get());
	brushProgram->use();
	brushProgram->setUniform(svdagLoader->getDepth(), "treeDepth");

	glGenBuffers(1, &brushBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, brushBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * 2, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, brushBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

Brush::~Brush()
{
	glDeleteBuffers(1, &brushBuffer);
}

glm::vec3 Brush::getBrushCenter() {
    brushProgram->use();
    brushProgram->setUniform(camera->RayDirMatrix(), "camera.rayDirMatrix");
    brushProgram->setUniform(camera->Position(), "camera.position");

    svdagLoader->bindNodes(0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, brushBuffer);

    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, brushBuffer);
    glm::vec4* data = (glm::vec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    glm::vec3 position = glm::vec3(-999.0);
    if (data && data->w > 0.0) {
        position = glm::vec3(*data);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    return position;
}

void Brush::apply(glm::vec3 center, float radius, bool isAdding, uint16_t material) {
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
                        svdagEditor->setVoxel(voxelPos, material);
                    }
                    else {
                        svdagEditor->clearVoxel(voxelPos);
                    }
                }
            }
        }
    }
}

void Brush::uploadChangesToGPU() {
    const auto& modified = svdagEditor->getModifiedIndices();
    const auto& nodes = svdagLoader->getNodes();
    size_t newNodesStart = svdagEditor->getNewNodesStartIndex();

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

    svdagEditor->clearModifiedLists();
}