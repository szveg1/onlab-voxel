#include "SVOBuilder.h"
#include "HeightMapGenerator.h"
#include <chrono>
#include <morton-nd/mortonND_BMI2.h>
#include <fstream>


SVOBuilder::SVOBuilder(uint32_t treeSize, uint16_t heightMapSize) : treeSize(treeSize), heightMapSize(heightMapSize)
{
    maxDepth = static_cast<size_t>(std::log2(treeSize));
}

SVOBuilder::~SVOBuilder()
{
}

void SVOBuilder::build() {
    auto start = std::chrono::steady_clock::now();
    printf("Max depth: %zu\n", maxDepth);

    HeightMapGenerator heightMapGenerator(heightMapSize, 8, 0.5f, 0.5f);

    size_t upscalingFactor = treeSize / heightMapSize;

    std::vector<float> heightMap = heightMapGenerator.generateHeightMap();

    auto maxHeight = 0u;
    auto minHeight = 0u;

    float x_ratio = (float)(heightMapSize - 1) / (treeSize - 1);
    float z_ratio = (float)(heightMapSize - 1) / (treeSize - 1);

    for (size_t gridX = 0; gridX < treeSize; gridX++) {
        for (size_t gridZ = 0; gridZ < treeSize; gridZ++) {
            float x = x_ratio * gridX;
            float z = z_ratio * gridZ;

            size_t x1 = static_cast<size_t>(floorf(x));
            size_t z1 = static_cast<size_t>(floorf(z));
            size_t x2 = x1 + 1;
            size_t z2 = z1 + 1;

            if (x2 >= heightMapSize) x2 = x1;
            if (z2 >= heightMapSize) z2 = z1;

            float q11 = heightMap[z1 * heightMapSize + x1];
            float q12 = heightMap[z2 * heightMapSize + x1];
            float q21 = heightMap[z1 * heightMapSize + x2];
            float q22 = heightMap[z2 * heightMapSize + x2];

            float x_diff = x - x1;
            float z_diff = z - z1;

            float interpolated = q11 * (1 - x_diff) * (1 - z_diff) +
                q21 * x_diff * (1 - z_diff) +
                q12 * (1 - x_diff) * z_diff +
                q22 * x_diff * z_diff;

            const uint32_t heightY = static_cast<uint32_t>(std::clamp(interpolated, 0.0f, 1.0f) * (treeSize - 1));
            if (heightY > maxHeight) maxHeight = heightY;
            if (heightY < minHeight) minHeight = heightY;
            for (size_t y = heightY - 10; y < heightY; y++)
            {
                uint64_t morton = mortonnd::MortonNDBmi_3D_64::Encode(gridX, y, gridZ);
                mortonCodes.push_back(morton);
            }
        }
    }

    printf("Number of leaf voxels: %zu\n", mortonCodes.size());
    printf("Max height: %u\n", maxHeight);
    printf("Min height: %u\n", minHeight);

    buildCPUTree();
    mortonCodes.clear();
    mortonCodes.shrink_to_fit();

    linearize();

    saveToFile();

    printf("Total nodes: %zu\n", nodes.size());
    nodes.clear();
    nodes.shrink_to_fit();

    // TODO: This is very slow, look into optimizing
    root.reset();

    auto end = std::chrono::steady_clock::now();
    printf("SVO generation took %.1f seconds\n", std::chrono::duration<float>(end - start).count());
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

    GPUNode rootNode;
    rootNode.childMask = root->childMask;
    for (size_t i = 0; i < 8; i++)
    {
        rootNode.children[i] = 0;
    }
    nodes.push_back(rootNode);

    linearizeRecursive(root, 0, 0);
}

void SVOBuilder::linearizeRecursive(std::unique_ptr<CPUNode>& node, uint64_t nodeIndex, size_t currentDepth)
{
    if (!node) return;

    if (currentDepth == maxDepth - 1)
    {
        return;
    }

    for (size_t i = 0; i < 8; i++)
    {
        if ((node->childMask & (1 << i)) != 0)
        {
            size_t childIndex = nodes.size();
            nodes[nodeIndex].children[i] = childIndex;

            GPUNode childNode;
            childNode.childMask = node->children[i]->childMask;
            for (size_t j = 0; j < 8; j++)
            {
                childNode.children[i] = 0;
            }
            nodes.push_back(childNode);

            linearizeRecursive(node->children[i], childIndex, currentDepth + 1);
        }
        else
        {
            nodes[nodeIndex].children[i] = 0;
        }
    }
}

void SVOBuilder::saveToFile()
{
	std::ofstream file("..\\Renderer\\world.bin", std::ios::binary);
	cereal::BinaryOutputArchive archive(file);
	archive(*this);
}