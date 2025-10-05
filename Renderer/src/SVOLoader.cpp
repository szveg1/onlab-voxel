#include "SVOLoader.h"
#include <chrono>
#include <fstream>

void SVOLoader::load()
{
	auto start = std::chrono::steady_clock::now();
	std::ifstream file("world.bin", std::ios::binary);
	cereal::BinaryInputArchive archive(file);
	archive(*this);
	auto end = std::chrono::steady_clock::now();
	printf("SVO loaded in %.1f seconds\n", std::chrono::duration<double>(end - start).count());
	printf("Size: %d^3\n", static_cast<int>(exp2(maxDepth)));

}

void SVOLoader::uploadToGPU()
{
	// TODO: maybe tweak this?
	nodes.reserve(nodes.size() * 3);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, nodes.capacity() * sizeof(GPUNode), nodes.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    glGenBuffers(1, &nodeCounter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, nodeCounter);
    GLuint initialCount = nodes.size();
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initialCount, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, nodeCounter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}
