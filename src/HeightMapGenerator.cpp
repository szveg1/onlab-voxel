#include "HeightMapGenerator.h"
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

HeightMapGenerator::HeightMapGenerator(unsigned int gridSize) : gridSize(gridSize) {}

HeightMapGenerator::~HeightMapGenerator() {}

std::vector<float> HeightMapGenerator::generateHeightMap(int octaves, float persistence, float scale)
{
 
    auto start = std::chrono::steady_clock::now();

    std::vector<float> heightMap(gridSize * gridSize);

    for (unsigned int y = 0; y < gridSize; ++y)
    {
        for (unsigned int x = 0; x < gridSize; ++x)
        {
            float nx = static_cast<float>(x) / gridSize;
            float ny = static_cast<float>(y) / gridSize;
            heightMap[y * gridSize + x] = layeredNoise(nx, ny, octaves, persistence, scale);
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(end - start);

    printf("heightmap generation took %.1f seconds\n", duration.count());

    return heightMap;
}

float HeightMapGenerator::noise(float x, float y)
{
    return (glm::simplex(glm::vec2(x, y)) + 1.0f) / 2.0f;
}

float HeightMapGenerator::layeredNoise(float x, float y, int octaves, float persistence, float scale)
{
    float total = 0.0f;
    float frequency = scale;
    float amplitude = 1.0f;
    float maxValue = 0.0f; 

    for (int i = 0; i < octaves; ++i)
    {
        total += noise(x * frequency, y * frequency) * amplitude;

        maxValue += amplitude;

        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}