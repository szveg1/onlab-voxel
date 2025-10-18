#include "SVDAGBuilder.h"
#include "HeightMapGenerator.h"
#include <chrono>
#include <morton-nd/mortonND_BMI2.h>
#include <fstream>
#include <glm/gtc/noise.hpp>

SVDAGBuilder::SVDAGBuilder(uint32_t treeSize, uint16_t heightMapSize, uint16_t chunkSize) : treeSize(treeSize), heightMapSize(heightMapSize), chunkSize(chunkSize)
{
	maxDepth = static_cast<size_t>(std::log2(treeSize));

	materialLUT.resize(treeSize);
	for (uint32_t y = 0; y < treeSize; y++) {
		float normalizedY = static_cast<float>(y) / (treeSize - 1);
		materialLUT[y] = computeMountainColor(normalizedY);
	}
}

SVDAGBuilder::~SVDAGBuilder()
{
}

void SVDAGBuilder::build()
{
	auto start = std::chrono::steady_clock::now();
	printf("Max depth: %zu\n", maxDepth);


	uint32_t maxHeight = 0;
	uint32_t minHeight = 0;
	size_t leafVoxels = 0;

	float xRatio = (float)(heightMapSize - 1) / (treeSize - 1);
	float zRatio = (float)(heightMapSize - 1) / (treeSize - 1);

	HeightMapGenerator generator = HeightMapGenerator(heightMapSize, 8, 0.5f, 0.5f);
	std::vector<float> heightmap = generator.generateHeightMap();

	for (size_t chunkX = 0; chunkX < treeSize; chunkX += chunkSize)
	{
	    for (size_t chunkZ = 0; chunkZ < treeSize; chunkZ += chunkSize)
	    {
	        for (size_t chunkY = 0; chunkY < treeSize; chunkY += chunkSize)
	        {
				auto chunkStart = std::chrono::steady_clock::now();
	            std::vector<uint64_t> chunkMortonCodes;
				std::shared_ptr<CPUNode> subtreeRoot = std::make_unique<CPUNode>();
				auto builtLevels = static_cast<size_t>(std::log2(chunkSize));
				auto currentDepth = maxDepth - builtLevels;
				uint64_t subtreeCode = mortonnd::MortonNDBmi_3D_64::Encode(chunkX / chunkSize, chunkY / chunkSize, chunkZ / chunkSize);

	            for (size_t voxelX = chunkX; voxelX < chunkX + chunkSize; voxelX++)
	            {
	                for (size_t voxelZ = chunkZ; voxelZ < chunkZ + chunkSize; voxelZ++)
	                {
	                    float x = xRatio * voxelX;
	                    float z = zRatio * voxelZ;

	                    size_t x1 = static_cast<size_t>(floorf(x));
	                    size_t z1 = static_cast<size_t>(floorf(z));
	                    size_t x2 = x1 + 1;
	                    size_t z2 = z1 + 1;

	                    if (x2 >= heightMapSize) x2 = x1;
	                    if (z2 >= heightMapSize) z2 = z1;

	                    float q11 = heightmap[z1 * heightMapSize + x1];
	                    float q12 = heightmap[z2 * heightMapSize + x1];
	                    float q21 = heightmap[z1 * heightMapSize + x2];
	                    float q22 = heightmap[z2 * heightMapSize + x2];

	                    float x_diff = x - x1;
	                    float z_diff = z - z1;

	                    float interpolated = q11 * (1 - x_diff) * (1 - z_diff) +
	                        q21 * x_diff * (1 - z_diff) +
	                        q12 * (1 - x_diff) * z_diff +
	                        q22 * x_diff * z_diff;

	                    const uint32_t y = static_cast<uint32_t>(std::clamp(interpolated, 0.0f, 1.0f) * (treeSize - 1));
	                    maxHeight = std::max(maxHeight, y);
	                    minHeight = std::min(minHeight, y);

	                    for (size_t voxelY = chunkY; voxelY < chunkY + chunkSize; voxelY++)
	                    {
	                        if (voxelY > y) break;
	                        uint64_t morton = mortonnd::MortonNDBmi_3D_64::Encode(voxelX, voxelY, voxelZ);
							leafVoxels++;
							uint16_t material = getMountainColor(voxelY);
							insertNodeRecursive(subtreeRoot, morton, currentDepth, material);
	                    }
	                }
	            }
				reduceTreeRecursive(subtreeRoot, currentDepth);
				subtrees[subtreeCode] = subtreeRoot;
				
				auto chunkEnd = std::chrono::steady_clock::now();
				printf("Chunk generation took %.1f seconds\n", std::chrono::duration<float>(chunkEnd - chunkStart).count());
	        }

	    }
	}
	
	mergeSubtrees();
	linearize();
	saveToFile();

	auto end = std::chrono::steady_clock::now();
	printf("SVDAG generation took %.1f seconds\n", std::chrono::duration<float>(end - start).count());
	printf("Leaf voxels: %zu\n", leafVoxels);
	printf("Total nodes: %zu\n", nodes.size());
	printf("Max height: %u\n", maxHeight);
	printf("Min height: %u\n", minHeight);
	printf("Max refs: %u\n", maxRefs);
}

uint16_t SVDAGBuilder::computeMountainColor(float y) {

	float snowLine = 0.75f;
	float rockLine = 0.45f;
	float grassLine = 0.25f;

	glm::vec3 mountainColor;

	float noise1 = glm::simplex(glm::vec2(y * 10.0f, 0.5f)) * 0.1f;
	float noise2 = glm::simplex(glm::vec2(y * 20.0f, 0.7f)) * 0.05f;

	if (y > snowLine) {
		float snowBlend = glm::smoothstep(snowLine, 0.9f, y);
		glm::vec3 shadedSnow = glm::vec3(0.8, 0.85, 0.95);
		glm::vec3 brightSnow = glm::vec3(1.0, 1.0, 1.0);
		mountainColor = glm::mix(shadedSnow, brightSnow, snowBlend);
		mountainColor += glm::vec3(noise1);
	}
	else if (y > rockLine) {
		float rockBlend = (y - rockLine) / (snowLine - rockLine);
		glm::vec3 lowerRock = glm::vec3(0.5, 0.4, 0.35);
		glm::vec3 upperRock = glm::vec3(0.7, 0.7, 0.75);
		mountainColor = glm::mix(lowerRock, upperRock, rockBlend);
		mountainColor += glm::vec3(noise2);
	}
	else if (y > grassLine) {
		float meadowBlend = (y - grassLine) / (rockLine - grassLine);
		glm::vec3 alpineMeadow = glm::vec3(0.3, 0.5, 0.2);
		glm::vec3 rockyTerrain = glm::vec3(0.45, 0.38, 0.32);
		mountainColor = glm::mix(alpineMeadow, rockyTerrain, meadowBlend);
		mountainColor += glm::vec3(noise1 * 2.0);
	}
	else {
		float forestBlend = y / grassLine;
		glm::vec3 darkForest = glm::vec3(0.1, 0.25, 0.1);
		glm::vec3 lightForest = glm::vec3(0.2, 0.35, 0.15);
		mountainColor = glm::mix(darkForest, lightForest, forestBlend);
		mountainColor += glm::vec3(noise2 * 1.5);
	}

	mountainColor = glm::clamp(mountainColor, glm::vec3(0.0f), glm::vec3(1.0f));

	uint16_t r = static_cast<uint16_t>(mountainColor.r * 31.0f);
	uint16_t g = static_cast<uint16_t>(mountainColor.g * 63.0f);
	uint16_t b = static_cast<uint16_t>(mountainColor.b * 31.0f);

	uint16_t rgb565 = (r << 11) | (g << 5) | b;

	return rgb565;
}

void SVDAGBuilder::insertNodeRecursive(std::shared_ptr<CPUNode>& parent, uint64_t morton, size_t currentDepth, uint16_t material)
{
	uint8_t childIndex = (morton >> (3 * (maxDepth - currentDepth - 1))) & 0b111;
	uint8_t childMask = 1 << childIndex;

	parent->childMask |= childMask;

	if (currentDepth == maxDepth - 1)
	{
		return;
	}

	if (parent->children[childIndex] == nullptr)
	{
		parent->children[childIndex] = std::make_unique<CPUNode>();
		parent->children[childIndex]->material = material;
	}

	insertNodeRecursive(parent->children[childIndex], morton, currentDepth + 1, material);
}

size_t hash_combine(size_t lhs, size_t rhs) {
	lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
	return lhs;
}

size_t SVDAGBuilder::calculateNodeHash(std::shared_ptr<CPUNode>& node) {
	if (!node) return 0;

	size_t hash = std::hash<uint8_t>{}(node->childMask);

	hash = hash_combine(hash, std::hash<uint16_t>{}(node->material));

	for (int i = 0; i < 8; ++i) {
		if (node->children[i]) {
			hash = hash_combine(hash, calculateNodeHash(node->children[i]));
		}
	}
	return hash;
}

void SVDAGBuilder::reduceTreeRecursive(std::shared_ptr<CPUNode>& node, size_t currentDepth) {
	if (!node || currentDepth == maxDepth) return;

	for (int i = 0; i < 8; ++i) {
		if (node->children[i]) {
			reduceTreeRecursive(node->children[i], currentDepth + 1);

			size_t childHash = calculateNodeHash(node->children[i]);
			auto nodeIter = nodeCache.find(childHash);
			if (nodeIter != nodeCache.end()) {
				node->children[i] = nodeIter->second;
			}
			else {
				nodeCache[childHash] = node->children[i];
			}
		}
	}

	size_t nodeHash = calculateNodeHash(node);

	auto it = nodeCache.find(nodeHash);
	if (it != nodeCache.end()) {
		it->second->refs++;
		maxRefs = std::max(maxRefs, it->second->refs);
		node = it->second;
	}
	else {
		nodeCache[nodeHash] = node;
		nodeCache[nodeHash]->refs = 1;
	}
}

void SVDAGBuilder::mergeSubtrees()
{
	root = std::make_shared<CPUNode>();

	size_t subtreeDepth = static_cast<size_t>(std::log2(chunkSize));
	size_t levels = static_cast<size_t>(std::log2(treeSize / chunkSize));

	for (const auto& [subtreeCode, subtreeRoot] : subtrees) {
		std::shared_ptr<CPUNode> currentNode = root;

		for (size_t level = 0; level < levels; level++) {
			uint8_t childIndex = (subtreeCode >> (3 * (levels - level - 1))) & 0b111;

			currentNode->childMask |= (1 << childIndex);

			if (level == levels - 1) {
				currentNode->children[childIndex] = subtreeRoot;
			}
			else {
				if (currentNode->children[childIndex] == nullptr) {
					currentNode->children[childIndex] = std::make_shared<CPUNode>();
				}

				currentNode = currentNode->children[childIndex];
			}
		}
	}

	nodeCache.clear();
	reduceTreeRecursive(root, 0);
}

void SVDAGBuilder::linearize()
{
	GPUNode rootNode;
	rootNode.childMask = root->childMask;
	nodes.push_back(rootNode);
	nodeToIndexMap[root] = 0;

	linearizeRecursive(root, 0, 0);
}

void SVDAGBuilder::linearizeRecursive(std::shared_ptr<CPUNode>& node,
	uint64_t nodeIndex,
	size_t currentDepth) {
	if (!node || currentDepth == maxDepth - 1) return;

	for (size_t i = 0; i < 8; i++) {
		if (!(node->childMask & (1 << i))) {
			nodes[nodeIndex].children[i] = 0;
			continue;
		}

		auto& child = node->children[i];

		if (nodeToIndexMap.contains(child)) {
			nodes[nodeIndex].children[i] = nodeToIndexMap[child];
		}

		else {
			size_t childIndex = nodes.size();
			nodes[nodeIndex].children[i] = childIndex;

			GPUNode gpuNode;
			gpuNode.childMask = child->childMask;
			gpuNode.refs = child->refs;
			gpuNode.material = child->material;

			nodes.push_back(gpuNode);

			nodeToIndexMap[child] = childIndex;
			linearizeRecursive(child, childIndex, currentDepth + 1);
		}
	}
}

void SVDAGBuilder::saveToFile()
{
	std::ofstream file("..\\Renderer\\world.bin", std::ios::binary);
	cereal::BinaryOutputArchive archive(file);
	archive(*this);
}

void SVDAGBuilder::dumpRefsToFile(const std::string& filename) {
	std::ofstream out(filename);
	if (!out.is_open()) {
		printf("Failed to open %s for writing\n", filename.c_str());
		return;
	}
	for (size_t i = 0; i < nodes.size(); ++i) {
		out << "Node " << i << ": refs = " << nodes[i].refs << "\n";
	}
	out.close();
	printf("Dumped refs to %s\n", filename.c_str());
}