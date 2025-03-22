#include "SVOLoader.h"
#include <chrono>
#include <fstream>

void SVOLoader::load()
{
	auto start = std::chrono::steady_clock::now();
	std::ifstream file("svo.bin", std::ios::binary);
	cereal::BinaryInputArchive archive(file);
	archive(*this);
	auto end = std::chrono::steady_clock::now();
	printf("SVO loaded in %.1f seconds\n", std::chrono::duration<double>(end - start).count());
}

void SVOLoader::uploadToGPU()
{
	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, nodes.size() * sizeof(GPUNode), nodes.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
}
