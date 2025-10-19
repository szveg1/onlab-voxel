#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "Camera.h"
#include "GPUProgram.h"
#include "SVDAGLoader.h"
#include "SVDAGEditor.h"

struct BrushData {
	bool didHit;
	glm::vec3 position;
	glm::vec3 normal;
};

struct GpuBrushData {
	glm::vec4 positionAndHit;
	glm::vec4 normal;
};

class Brush
{
public:
	Brush(std::shared_ptr<Camera> camera, std::shared_ptr<SVDAGLoader> svdagLoader, std::shared_ptr<SVDAGEditor> svdagEditor);
	~Brush();

	glm::vec3 getBrushCenter();

	void apply(glm::vec3 center, float radius, bool isAdding, uint16_t material);

	void uploadChangesToGPU();

private:
	std::shared_ptr<Camera> camera;
	std::shared_ptr<SVDAGLoader> svdagLoader;
	std::shared_ptr<SVDAGEditor> svdagEditor;

	GLuint brushBuffer;
	std::unique_ptr<Shader> brushShader;
	std::unique_ptr<GPUProgram> brushProgram;
};