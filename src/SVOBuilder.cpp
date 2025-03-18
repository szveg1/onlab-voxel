#include "SVOBuilder.h"
#include "HeightMapGenerator.h"
#include "Color.h"
#include <chrono>

#include <morton-nd/mortonND_BMI2.h>
#include <execution>

SVOBuilder::SVOBuilder(uint32_t treeSize, uint16_t chunkSize) : treeSize(treeSize), chunkSize(chunkSize)
{
	maxDepth = static_cast<size_t>(std::log2(treeSize));
}

SVOBuilder::~SVOBuilder()
{
}


void SVOBuilder::build() {
    auto start = std::chrono::steady_clock::now();
    printf("Max depth: %zu\n", maxDepth);

    HeightMapGenerator heightMapGenerator(chunkSize, 8, 0.5f, 0.5f);
    uint32_t numChunks = (treeSize * treeSize) / (chunkSize * chunkSize);
    std::vector<std::vector<uint64_t>> chunkMortonCodes(numChunks);

    for (size_t i = 0; i < numChunks; ++i) {
        uint32_t chunkX, chunkZ;
        std::tie(chunkX, chunkZ) = mortonnd::MortonNDBmi_2D_32::Decode(i);
        chunkMortonCodes[i] = heightMapGenerator.generateHeightMapChunk(chunkX, chunkZ);
    }

    for (auto& chunk : chunkMortonCodes) {
        mortonCodes.insert(mortonCodes.end(), chunk.begin(), chunk.end());
    }

    std::sort(std::execution::par, mortonCodes.begin(), mortonCodes.end());
    printf("Number of leaf nodes: %zu\n", mortonCodes.size());

	buildCPUTree();
    mortonCodes.clear();
    mortonCodes.shrink_to_fit();

    linearize();

    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nodes.size() * sizeof(GPUNode),
        nodes.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    nodes.clear();
    nodes.shrink_to_fit();

	// TODO: This is very slow, look into optimizing
    root.reset();

    auto end = std::chrono::steady_clock::now();
    printf("SVO generation took %.1f seconds\n", std::chrono::duration<float>(end - start).count());
    printf("Total nodes: %zu\n", nodes.size());
}

void SVOBuilder::buildCPUTree() {
    root = std::make_unique<CPUNode>();

	for (auto morton : mortonCodes) {
		insertNode(morton);
	}
}

void SVOBuilder::insertNode(uint64_t morton)
{
	insertNodeRecursive(root, morton, 0);
}

void SVOBuilder::insertNodeRecursive(std::unique_ptr<CPUNode>& parent, uint64_t morton, size_t currentDepth)
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
    }

    insertNodeRecursive(parent->children[childIndex], morton, currentDepth + 1);
}

void SVOBuilder::linearize()
{
    nodes.reserve(mortonCodes.size());
    nodes.push_back({ root->childMask, 0 });

    linearizeRecursive(root, 0, 0);
}

void SVOBuilder::linearizeRecursive(std::unique_ptr<CPUNode>& node, uint64_t nodeIndex, size_t currentDepth)
{
    if (!node) return;

    if (currentDepth == maxDepth - 1)
	{
		nodes[nodeIndex].childIndex = 0;
        return;
	}

    size_t childCount = std::popcount(node->childMask);

    if (childCount == 0) return;

    nodes[nodeIndex].childIndex = nodes.size();

    size_t startIndex = nodes.size();
    for (size_t i = 0; i < childCount; i++)
    {
        nodes.push_back({ 0,0 });
    }

    size_t resultIndex = 0;
    for (size_t i = 0; i < 8; i++)
    {
        if ((node->childMask & (1 << i)) != 0)
        {
            nodes[startIndex + resultIndex].childMask = node->children[i]->childMask;
            linearizeRecursive(node->children[i], startIndex + resultIndex, currentDepth + 1);
            resultIndex++;
        }
    }
}