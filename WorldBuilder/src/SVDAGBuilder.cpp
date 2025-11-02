#include "HeightMapGenerator.h"
#include "SVDAGBuilder.h"
#include "TriangleBVH.h"

#include <atomic>
#include <chrono>
#include <execution>
#include <fstream>
#include <glm/gtc/noise.hpp>
#include <morton-nd/mortonND_BMI2.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

    #pragma omp parallel for collapse(3) reduction(+:leafVoxels) reduction(max:maxHeight) reduction(min:minHeight)
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
                printf("Chunk (%zu,%zu,%zu) took %.1f seconds\n",
                    chunkX / chunkSize, chunkY / chunkSize, chunkZ / chunkSize,
                    std::chrono::duration<float>(chunkEnd - chunkStart).count());
            }
        }
    }
    mergeSubtrees();
    linearize();
    saveToFile("world" + std::to_string(treeSize));

    auto end = std::chrono::steady_clock::now();
    printf("SVDAG generation took %.1f seconds\n", std::chrono::duration<float>(end - start).count());
    printf("Leaf voxels: %zu\n", leafVoxels);
    printf("Total nodes: %zu\n", nodes.size());
    printf("Max refs: %u\n", maxRefs);
}

uint16_t SVDAGBuilder::colorToRGB565(const glm::vec3& color) {
    glm::vec3 clamped = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
    uint16_t r = static_cast<uint16_t>(clamped.r * 31.0f);
    uint16_t g = static_cast<uint16_t>(clamped.g * 63.0f);
    uint16_t b = static_cast<uint16_t>(clamped.b * 31.0f);
    return (r << 11) | (g << 5) | b;
}

MaterialData SVDAGBuilder::loadMaterial(const aiMaterial* aiMat, const std::string& modelDir) {
    MaterialData mat;
    mat.hasTexture = false;
    mat.diffuseColor = glm::vec3(0.8f);
    mat.specularColor = glm::vec3(0.5f);
    mat.shininess = 32.0f;

    aiColor3D color;
    if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        mat.diffuseColor = glm::vec3(color.r, color.g, color.b);
    }

    if (aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
        mat.specularColor = glm::vec3(color.r, color.g, color.b);
    }

    float shininess;
    if (aiMat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
        mat.shininess = shininess;
    }

    aiString texPath;
    if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
        std::string filename = texPath.C_Str();
        std::string fullPath = modelDir + "/" + filename;

        if (textureCache.find(fullPath) == textureCache.end()) {
            textureCache[fullPath] = materials.size();
        }

        mat.texturePath = fullPath;
        mat.hasTexture = true;
        printf("  Loaded texture: %s\n", fullPath.c_str());
    }

    mat.materialID = colorToRGB565(mat.diffuseColor);
    return mat;
}

uint16_t SVDAGBuilder::sampleTextureColor(const std::string& texturePath, float u, float v) {
    if (globalTextureCache.find(texturePath) == globalTextureCache.end()) {
        int width, height, channels;
        unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 3);

        if (!data) {
            printf("Failed to load texture: %s\n", texturePath.c_str());
            return colorToRGB565(glm::vec3(1.0f, 0.0f, 1.0f));
        }

        globalTextureCache[texturePath] = {
            std::unique_ptr<unsigned char[]>(data),
            width,
            height
        };
        printf("Loaded texture into memory: %s (%dx%d)\n", texturePath.c_str(), width, height);
    }

    const auto& texData = globalTextureCache[texturePath];

    u = u - floorf(u);
    v = v - floorf(v);
    u = glm::clamp(u, 0.0f, 1.0f);
    v = glm::clamp(v, 0.0f, 1.0f);

    int x = glm::clamp(static_cast<int>(u * (texData.width - 1)), 0, texData.width - 1);
    int y = glm::clamp(static_cast<int>(v * (texData.height - 1)), 0, texData.height - 1);

    int index = (y * texData.width + x) * 3;

    glm::vec3 color(
        texData.data[index] / 255.0f,
        texData.data[index + 1] / 255.0f,
        texData.data[index + 2] / 255.0f
    );

    return colorToRGB565(color);
}

void SVDAGBuilder::buildFromModel(const std::string& modelPath, uint16_t defaultMaterial)
{
    stbi_set_flip_vertically_on_load(true);

    auto start = std::chrono::steady_clock::now();
    printf("Voxelizing model from: %s\n", modelPath.c_str());
    printf("Max depth: %zu\n", maxDepth);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("ERROR::ASSIMP:: %s\n", importer.GetErrorString());
        return;
    }

    std::string modelDir = modelPath.substr(0, modelPath.find_last_of("/\\"));

    printf("Loading %u materials...\n", scene->mNumMaterials);
    materials.clear();
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        MaterialData mat = loadMaterial(scene->mMaterials[i], modelDir);
        materials.push_back(mat);
        printf("  Material %u: RGB565=0x%04X, hasTexture=%d\n",
            i, mat.materialID, mat.hasTexture);
    }

    printf("Pre-loading textures...\n");
    for (const auto& mat : materials) {
        if (mat.hasTexture) {
            sampleTextureColor(mat.texturePath, 0.0f, 0.0f);
        }
    }

    std::vector<Triangle> triangles;
    triangles.reserve(100000);

    for (unsigned int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
        aiMesh* mesh = scene->mMeshes[meshIdx];
        uint32_t matIndex = mesh->mMaterialIndex;

        for (unsigned int faceIdx = 0; faceIdx < mesh->mNumFaces; faceIdx++) {
            aiFace face = mesh->mFaces[faceIdx];
            if (face.mNumIndices != 3) continue;

            Triangle tri;
            tri.materialIndex = matIndex;

            for (int i = 0; i < 3; i++) {
                unsigned int idx = face.mIndices[i];
                glm::vec3* verts[] = { &tri.v0, &tri.v1, &tri.v2 };
                glm::vec2* uvs[] = { &tri.uv0, &tri.uv1, &tri.uv2 };

                *verts[i] = glm::vec3(
                    mesh->mVertices[idx].x,
                    mesh->mVertices[idx].y,
                    mesh->mVertices[idx].z
                );

                if (mesh->mTextureCoords[0]) {
                    *uvs[i] = glm::vec2(
                        mesh->mTextureCoords[0][idx].x,
                        mesh->mTextureCoords[0][idx].y
                    );
                }
                else {
                    *uvs[i] = glm::vec2(0.0f);
                }
            }

            triangles.push_back(tri);
        }
    }

    printf("Loaded %zu triangles\n", triangles.size());

    glm::vec3 minAABB(std::numeric_limits<float>::max());
    glm::vec3 maxAABB(std::numeric_limits<float>::lowest());

    for (const auto& tri : triangles) {
        minAABB = glm::min(minAABB, glm::min(glm::min(tri.v0, tri.v1), tri.v2));
        maxAABB = glm::max(maxAABB, glm::max(glm::max(tri.v0, tri.v1), tri.v2));
    }

    glm::vec3 size = maxAABB - minAABB;
    float maxDim = std::max({ size.x, size.y, size.z });
    float scale = (float)(treeSize - 1) / maxDim;
    glm::vec3 translation = -minAABB;

    for (auto& tri : triangles) {
        tri.v0 = (tri.v0 + translation) * scale;
        tri.v1 = (tri.v1 + translation) * scale;
        tri.v2 = (tri.v2 + translation) * scale;
    }

    printf("Model bounds: (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f)\n",
        minAABB.x, minAABB.y, minAABB.z, maxAABB.x, maxAABB.y, maxAABB.z);

    printf("Building BVH...\n");
    auto bvhStart = std::chrono::steady_clock::now();
    TriangleBVH bvh;
    bvh.build(triangles);
    auto bvhEnd = std::chrono::steady_clock::now();
    printf("BVH built in %.2f seconds\n", std::chrono::duration<float>(bvhEnd - bvhStart).count());

    std::atomic<size_t> leafVoxels{ 0 };
    auto builtLevels = static_cast<size_t>(std::log2(chunkSize));
    auto currentDepth = maxDepth - builtLevels;

    std::vector<std::tuple<size_t, size_t, size_t>> chunkCoords;
    for (size_t chunkX = 0; chunkX < treeSize; chunkX += chunkSize) {
        for (size_t chunkY = 0; chunkY < treeSize; chunkY += chunkSize) {
            for (size_t chunkZ = 0; chunkZ < treeSize; chunkZ += chunkSize) {
                chunkCoords.emplace_back(chunkX, chunkY, chunkZ);
            }
        }
    }

    std::mutex subtreesMutex;
    std::atomic<size_t> chunksProcessed{ 0 };

    std::for_each(std::execution::par, chunkCoords.begin(), chunkCoords.end(),
        [&](const auto& coords) {
            size_t chunkX = std::get<0>(coords);
            size_t chunkY = std::get<1>(coords);
            size_t chunkZ = std::get<2>(coords);
            auto chunkStart = std::chrono::steady_clock::now();

            glm::vec3 chunkMin(chunkX, chunkY, chunkZ);
            glm::vec3 chunkMax(chunkX + chunkSize, chunkY + chunkSize, chunkZ + chunkSize);

            std::vector<size_t> relevantTriangles;
            bvh.query(chunkMin, chunkMax, relevantTriangles);

            if (relevantTriangles.empty()) {
                return;
            }

            std::shared_ptr<CPUNode> subtreeRoot = std::make_unique<CPUNode>();
            std::unordered_set<uint64_t> chunkVoxelSet;
            chunkVoxelSet.reserve(chunkSize * chunkSize * chunkSize / 10);

            size_t localLeafVoxels = 0;

            for (size_t triIdx : relevantTriangles) {
                const auto& tri = triangles[triIdx];
                const MaterialData& material = materials[tri.materialIndex];

                glm::vec3 triMin = glm::min(glm::min(tri.v0, tri.v1), tri.v2);
                glm::vec3 triMax = glm::max(glm::max(tri.v0, tri.v1), tri.v2);

                glm::ivec3 minVoxel = glm::clamp(
                    glm::ivec3(glm::floor(triMin)),
                    glm::ivec3(chunkMin),
                    glm::ivec3(chunkMax - 1.0f));
                glm::ivec3 maxVoxel = glm::clamp(
                    glm::ivec3(glm::ceil(triMax)),
                    glm::ivec3(chunkMin),
                    glm::ivec3(chunkMax - 1.0f));

                if (minVoxel.x > maxVoxel.x || minVoxel.y > maxVoxel.y || minVoxel.z > maxVoxel.z) {
                    continue;
                }

                for (int z = minVoxel.z; z <= maxVoxel.z; ++z) {
                    for (int y = minVoxel.y; y <= maxVoxel.y; ++y) {
                        for (int x = minVoxel.x; x <= maxVoxel.x; ++x) {
                            glm::vec3 voxelCenter(x + 0.5f, y + 0.5f, z + 0.5f);
                            glm::vec3 voxelHalfSize(0.5f);

                            if (triangleAABBIntersect(tri.v0, tri.v1, tri.v2,
                                voxelCenter, voxelHalfSize)) {
                                uint64_t morton = mortonnd::MortonNDBmi_3D_64::Encode(
                                    static_cast<uint32_t>(x),
                                    static_cast<uint32_t>(y),
                                    static_cast<uint32_t>(z));

                                if (chunkVoxelSet.insert(morton).second) {
                                    uint16_t voxelMaterial;
                                    if (material.hasTexture) {
                                        glm::vec3 bary = calculateBarycentric(voxelCenter,
                                            tri.v0, tri.v1, tri.v2);
                                        glm::vec2 uv = bary.x * tri.uv0 + bary.y * tri.uv1 + bary.z * tri.uv2;
                                        voxelMaterial = sampleTextureColor(material.texturePath, uv.x, uv.y);
                                    }
                                    else {
                                        voxelMaterial = material.materialID;
                                    }

                                    insertNodeRecursive(subtreeRoot, morton, currentDepth, voxelMaterial);
                                    localLeafVoxels++;
                                }
                            }
                        }
                    }
                }
            }

            leafVoxels += localLeafVoxels;

            if (!chunkVoxelSet.empty()) {
                std::unordered_map<size_t, std::shared_ptr<CPUNode>> localCache;
                reduceTreeRecursiveThreadSafe(subtreeRoot, currentDepth, localCache);
                uint64_t subtreeCode = mortonnd::MortonNDBmi_3D_64::Encode(
                    chunkX / chunkSize,
                    chunkY / chunkSize,
                    chunkZ / chunkSize);

                {
                    std::lock_guard<std::mutex> lock(subtreesMutex);
                    subtrees[subtreeCode] = subtreeRoot;
                }
            }

            size_t processed = ++chunksProcessed;
            if (processed % 10 == 0) {
                printf("Processed %zu/%zu chunks (%.1f%%), %zu voxels so far\n",
                    processed, chunkCoords.size(),
                    100.0f * processed / chunkCoords.size(),
                    leafVoxels.load());
            }

            auto chunkEnd = std::chrono::steady_clock::now();
        });

    printf("\nMerging subtrees...\n");
    mergeSubtrees();

    printf("Linearizing...\n");
    linearize();

    printf("Saving to file...\n");
    saveToFile(modelDir + std::to_string(treeSize));

    auto end = std::chrono::steady_clock::now();
    printf("\n=== SVDAG generation complete ===\n");
    printf("Total time: %.1f seconds\n", std::chrono::duration<float>(end - start).count());
    printf("Leaf voxels: %zu\n", leafVoxels.load());
    printf("Total nodes: %zu\n", nodes.size());
    printf("Compression ratio: %.2fx\n", static_cast<float>(leafVoxels) / nodes.size());
}

glm::vec3 SVDAGBuilder::calculateBarycentric(const glm::vec3& p, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
    glm::vec3 v0v1 = v1 - v0;
    glm::vec3 v0v2 = v2 - v0;
    glm::vec3 v0p = p - v0;

    float d00 = glm::dot(v0v1, v0v1);
    float d01 = glm::dot(v0v1, v0v2);
    float d11 = glm::dot(v0v2, v0v2);
    float d20 = glm::dot(v0p, v0v1);
    float d21 = glm::dot(v0p, v0v2);

    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return glm::vec3(u, v, w);
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

    return colorToRGB565(mountainColor);
}

void SVDAGBuilder::insertNodeRecursive(std::shared_ptr<CPUNode>& parent, uint64_t morton, size_t currentDepth, uint16_t material)
{
    uint8_t childIndex = (morton >> (3 * (maxDepth - currentDepth - 1))) & 0b111;
    uint8_t childMask = 1 << childIndex;

    parent->childMask |= childMask;

    if (currentDepth == maxDepth - 1) {
        return;
    }

    if (parent->children[childIndex] == nullptr) {
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

void SVDAGBuilder::reduceTreeRecursiveThreadSafe(std::shared_ptr<CPUNode>& node, size_t currentDepth, std::unordered_map<size_t, std::shared_ptr<CPUNode>>& localCache) {
    if (!node || currentDepth == maxDepth) return;

    for (int i = 0; i < 8; ++i) {
        if (node->children[i]) {
            reduceTreeRecursiveThreadSafe(node->children[i], currentDepth + 1, localCache);

            size_t childHash = calculateNodeHash(node->children[i]);
            auto nodeIter = localCache.find(childHash);
            if (nodeIter != localCache.end()) {
                node->children[i] = nodeIter->second;
            }
            else {
                localCache[childHash] = node->children[i];
            }
        }
    }

    size_t nodeHash = calculateNodeHash(node);
    auto it = localCache.find(nodeHash);
    if (it != localCache.end()) {
        it->second->refs++;
        node = it->second;
    }
    else {
        localCache[nodeHash] = node;
        localCache[nodeHash]->refs = 1;
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
    uint64_t nodeIndex, size_t currentDepth) {
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

void SVDAGBuilder::saveToFile(std::string fileName)
{
    size_t pos = fileName.find_last_of("\\");
	std::string path = "..\\Renderer\\" + fileName.substr(pos + 1) + ".dag";
    std::ofstream file(path, std::ios::binary);
    cereal::BinaryOutputArchive archive(file);
    archive(*this);
}
