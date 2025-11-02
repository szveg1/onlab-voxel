#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "Camera.h"
#include "Common.h"
#include "GPUProgram.h"
#include "SVDAGEditor.h"
#include "SVDAGLoader.h"

struct BrushData {
	bool didHit;
	glm::vec3 position;
	glm::vec3 normal;
};

struct GpuBrushData {
	glm::vec4 positionAndHit;
	glm::vec4 normal;
};

enum BrushMode {
	PAINT,
	SPHERE,
	BOX
};

class Brush
{
public:
	Brush(std::shared_ptr<Camera> camera, std::shared_ptr<SVDAGLoader> svdagLoader, std::shared_ptr<SVDAGEditor> svdagEditor);
	~Brush();

	BrushData getBrushData();

	void sphere(glm::vec3 center, float radius, bool isAdding, uint16_t material);
	void paint(glm::vec3 center, float radius, uint16_t material);
	void box(glm::vec3 p1, glm::vec3 p2, bool isAdding, uint16_t material);
	void uploadChangesToGPU();

private:
	std::shared_ptr<Camera> camera;
	std::shared_ptr<SVDAGLoader> svdagLoader;
	std::shared_ptr<SVDAGEditor> svdagEditor;

	GLuint brushBuffer;
	std::unique_ptr<Shader> brushShader;
	std::unique_ptr<GPUProgram> brushProgram;
};