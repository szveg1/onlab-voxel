#include "HeightMapGenerator.h"
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <morton-nd/mortonND_BMI2.h>

HeightMapGenerator::HeightMapGenerator(uint32_t gridSize, int octaves, float persistence, float scale) 
    : gridSize(gridSize), octaves(octaves), persistence(persistence), scale(scale)
{
    generator = std::mt19937(device());
    distribution = std::uniform_real_distribution<float>(-1000.0f, 1000.0f);
    randomOffsetX = distribution(generator);
    randomOffsetY = distribution(generator);
}

HeightMapGenerator::~HeightMapGenerator() {}

std::vector<float> HeightMapGenerator::generateHeightMap()
{
    auto start = std::chrono::steady_clock::now();
    std::vector<float> heightMap;
    heightMap.resize(gridSize * gridSize);
    for (uint32_t x = 0; x < gridSize; x++)
    {
        for (uint32_t z = 0; z < gridSize; z++)
        {
            float nx = static_cast<float>(x) / gridSize + randomOffsetX;
            float nz = static_cast<float>(z) / gridSize + randomOffsetY;
            float nheight = layeredNoise(nx, nz, octaves, persistence, scale);
            heightMap[z * gridSize + x] = nheight;
        }
    }
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(end - start);
    printf("heightmap generation took %.1f seconds\n", duration.count());
    return heightMap;
}

float HeightMapGenerator::noise(float x, float z)
{
    return (glm::simplex(glm::vec2(x, z)) + 1.0f) / 2.0f;
}

float HeightMapGenerator::layeredNoise(float x, float z, int octaves, float persistence, float scale)
{
    float total = 0.0f;
    float frequency = scale;
    float amplitude = 1.0f;
    float maxValue = 0.0f; 

    for (int i = 0; i < octaves; i++)
    {
        total += noise(x * frequency, z * frequency) * amplitude;

        maxValue += amplitude;

        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}