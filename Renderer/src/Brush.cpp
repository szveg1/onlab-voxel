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

BrushData Brush::getBrushData() {
    brushProgram->use();
    brushProgram->setUniform(camera->RayDirMatrix(), "camera.rayDirMatrix");
    brushProgram->setUniform(camera->Position(), "camera.position");

    svdagLoader->bindNodes(0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, brushBuffer);

    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, brushBuffer);
    glm::vec4* data = (glm::vec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

	BrushData brushData;
    if (data && data[0].w > 0.0) {
        brushData.didHit = true;
        brushData.position = glm::vec3(data[0]);
        brushData.normal = glm::vec3(data[1]);
    }
    else {
        brushData.didHit = false;
        brushData.position = glm::vec3(-999.0);
        brushData.normal = glm::vec3(0.0);
	}
    
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    return brushData;
}
 

void Brush::sphere(glm::vec3 center, float radius, bool isAdding, uint16_t material) {
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

void Brush::paint(glm::vec3 center, float radius, uint16_t material) {
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

                    svdagEditor->paintVoxel(voxelPos, material);

                }
            }
        }
    }
}

void Brush::box(glm::vec3 p1, glm::vec3 p2, bool isAdding, uint16_t material) {
    size_t resolution = static_cast<size_t>(exp2f(svdagLoader->getDepth()));
    const float voxelSize = 1.0f / resolution;

    glm::ivec3 boxMin = glm::floor(p1 / voxelSize);
    glm::ivec3 boxMax = glm::ceil(p2 / voxelSize);

    glm::ivec3 maxCoord = glm::ivec3(resolution - 1);
    boxMin = glm::clamp(boxMin, glm::ivec3(0), maxCoord);
    boxMax = glm::clamp(boxMax, glm::ivec3(0), maxCoord);

    Box targetBox = { boxMin, boxMax };

    svdagEditor->modifyRegion(targetBox, isAdding, material);
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