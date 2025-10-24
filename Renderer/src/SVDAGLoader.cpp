#include "SVDAGLoader.h"
#include <chrono>
#include <fstream>

void SVDAGLoader::load(std::string filePath)
{
	nodes.clear();
	auto start = std::chrono::steady_clock::now();
	std::ifstream file(filePath, std::ios::binary);
	cereal::BinaryInputArchive archive(file);
	archive(*this);
	auto end = std::chrono::steady_clock::now();
	printf("SVO loaded in %.1f seconds\n", std::chrono::duration<double>(end - start).count());
	printf("Size: %d^3\n", static_cast<int>(exp2(maxDepth)));
	printf("Max refs: %u\n", maxRefs);

}

void SVDAGLoader::uploadToGPU()
{
	if (ssbo != 0) {
		glDeleteBuffers(1, &ssbo);
		ssbo = 0;
	}
	if (nodeCounter != 0) {
		glDeleteBuffers(1, &nodeCounter);
		nodeCounter = 0;
	}

	// TODO: maybe tweak this?
	nodes.reserve(nodes.size() * 10);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, nodes.capacity() * sizeof(SVDAGGPUNode), nodes.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

	glGenBuffers(1, &nodeCounter);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, nodeCounter);
	GLuint initialCount = nodes.size();
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initialCount, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, nodeCounter);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

GLuint SVDAGLoader::getNodeCount()
{
	GLuint count;
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, nodeCounter);
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &count);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	return count;
}

SVDAGLoader::~SVDAGLoader() {
	if (ssbo != 0) {
		glDeleteBuffers(1, &ssbo);
	}
	if (nodeCounter != 0) {
		glDeleteBuffers(1, &nodeCounter);
	}
}