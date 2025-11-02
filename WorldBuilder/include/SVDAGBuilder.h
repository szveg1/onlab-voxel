#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct CPUNode
{
	uint8_t childMask;
	uint32_t refs;
	uint16_t material;
	std::shared_ptr<CPUNode> children[8];
};

struct GPUNode {
	uint8_t childMask;
	uint32_t refs;
	uint16_t material;
	uint32_t children[8];
	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(childMask);
		ar(refs);
		ar(material);
		for (size_t i = 0; i < 8; i++)
		{
			ar(children[i]);
		}
	}
};

struct MaterialData {
	glm::vec3 diffuseColor;
	glm::vec3 specularColor;
	float shininess;
	std::string texturePath;
	bool hasTexture;
	uint16_t materialID;
};

struct TextureData {
    std::unique_ptr<unsigned char[]> data;
    int width;
    int height;
};

static std::unordered_map<std::string, TextureData> globalTextureCache;

class SVDAGBuilder
{
public:
	SVDAGBuilder(uint32_t treeSize, uint16_t heightMapSize, uint16_t chunkSize);
	~SVDAGBuilder();
	void build();
	void buildFromModel(const std::string& modelPath, uint16_t defaultMaterial = 0xFFFF);
private:
	uint32_t treeSize;
	uint16_t heightMapSize;
	uint16_t chunkSize;
	size_t maxDepth;
	uint32_t maxRefs;

	std::unordered_map<uint64_t, std::shared_ptr<CPUNode>> subtrees;


	std::vector<GPUNode> nodes;
	std::shared_ptr<CPUNode> root;
	std::unordered_map<size_t, std::shared_ptr<CPUNode>> nodeCache;
	std::unordered_map<std::shared_ptr<CPUNode>, size_t> nodeToIndexMap;
	std::vector<uint16_t> materialLUT;
	std::vector<MaterialData> materials;
	std::unordered_map<std::string, size_t> textureCache;


	uint16_t computeMountainColor(float y);
	uint16_t getMountainColor(uint32_t y) { return materialLUT[y]; }
	MaterialData loadMaterial(const aiMaterial* aiMat, const std::string& modelDir);
	uint16_t sampleTextureColor(const std::string& texturePath, float u, float v);
	uint16_t colorToRGB565(const glm::vec3& color);
	glm::vec3 calculateBarycentric(const glm::vec3& p, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
	void insertNodeRecursive(std::shared_ptr<CPUNode>& parent, uint64_t morton, size_t currentDepth, uint16_t material);
	size_t calculateNodeHash(std::shared_ptr<CPUNode>& node);
	void reduceTreeRecursive(std::shared_ptr<CPUNode>& node, size_t currentDepth);
    void reduceTreeRecursiveThreadSafe(std::shared_ptr<CPUNode>& node, size_t currentDepth, std::unordered_map<size_t, std::shared_ptr<CPUNode>>& localCache);
	void mergeSubtrees();
	void linearize();
	void linearizeRecursive(std::shared_ptr<CPUNode>& node, uint64_t nodeIndex, size_t currentDepth);
	void saveToFile(std::string fileName);

	friend class cereal::access;
	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(maxDepth, maxRefs, nodes);
	}
};