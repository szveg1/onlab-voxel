#pragma once
#include <vector>
#include <random>

class HeightMapGenerator
{
public:
    HeightMapGenerator(uint32_t gridSize, int octaves, float persistence, float scale);
    ~HeightMapGenerator();
    std::vector<uint64_t> generateHeightMapChunk(uint32_t chunkX, uint32_t chunkZ);
private:
    uint32_t gridSize;
    int octaves;
	float persistence;
	float scale;
    float randomOffsetX;
    float randomOffsetY;

    std::random_device device;
    std::mt19937 generator;
    std::uniform_real_distribution<float> distribution;
    float noise(float x, float y);
    float layeredNoise(float x, float y, int octaves, float persistence, float scale);
};