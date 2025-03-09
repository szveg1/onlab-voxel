#pragma once
#include <vector>
#include <string>

class HeightMapGenerator
{
public:
    HeightMapGenerator(unsigned int gridSize);
    ~HeightMapGenerator();

    std::vector<float> generateHeightMap(int octaves, float persistence, float scale);

private:
    float noise(float x, float y);
    float layeredNoise(float x, float y, int octaves, float persistence, float scale);

    unsigned int gridSize;
};