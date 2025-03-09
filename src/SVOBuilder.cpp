#include "SVOBuilder.h"
#include "HeightMapGenerator.h"
#include "Color.h"
#include <chrono>

SVOBuilder::SVOBuilder(unsigned int size) : size(size)
{
}

SVOBuilder::~SVOBuilder()
{
}


void SVOBuilder::build()
{
	auto start = std::chrono::steady_clock::now();
	nodes = std::vector<VoxelNode>(size * size * size);
	HeightMapGenerator heightMapGenerator(size);
	heightMap = heightMapGenerator.generateHeightMap(8, 0.5f, 0.5f);

	for (int x = 0; x < size; x++)
	{
		for (int z = 0; z < size; z++)
		{
			int height = heightMap[x * size + z] * size;
			for (int y = 0; y < height; y++)
			{
				Color color = color.heightToColor(y / (float)size);
				nodes[x * size * size + y * size + z] = { x, y, z, 255, color.r, color.g, color.b };
			}
		}
	}

	glGenTextures(1, &voxelTexture);
	glBindTexture(GL_TEXTURE_3D, voxelTexture);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	std::vector<uint8_t> textureData(size * size * size * 4);
	for (size_t i = 0; i < nodes.size(); ++i) {
		textureData[i * 4 + 0] = nodes[i].r;
		textureData[i * 4 + 1] = nodes[i].g;
		textureData[i * 4 + 2] = nodes[i].b;
		textureData[i * 4 + 3] = nodes[i].a;
	}

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, size, size, size, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, textureData.data());

	textureData.clear();
	textureData.shrink_to_fit();

	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(end - start);

	printf("SVO generation took %.1f seconds\n", duration.count());

	
}

GLuint SVOBuilder::getTexture()
{
	return voxelTexture;
}

