#pragma once
#include <cstdint>
#include <vector>
#include <glad/glad.h>

struct VoxelNode {
    int32_t x;
    int32_t y;
    int32_t z;
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class SVOBuilder
{
public:
	SVOBuilder(unsigned int size);
	~SVOBuilder();
    void build();
    GLuint getTexture();
private:
	unsigned int size;
    std::vector<float> heightMap;
	std::vector<VoxelNode> nodes;
    GLuint voxelTexture;
};