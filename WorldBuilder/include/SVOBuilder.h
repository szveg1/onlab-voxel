#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

struct CPUNode {
	uint8_t childMask;
	std::unique_ptr<CPUNode> children[8];
};

struct GPUNode {
	uint8_t childMask;
	uint32_t children[8];
	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(childMask);
		for (size_t i = 0; i < 8; i++)
		{
			ar(children[i]);
		}
	}
};


class SVOBuilder
{
public:
	SVOBuilder(uint32_t treeSize, uint16_t heightMapSize);
	~SVOBuilder();
    void build();
private:
	uint32_t treeSize;
	uint16_t heightMapSize;
	size_t maxDepth;
	std::vector<uint64_t> mortonCodes;
	std::vector<GPUNode> nodes;
	std::unique_ptr<CPUNode> root;

	void buildCPUTree();
	void insertNode(uint64_t morton);
	void insertNodeRecursive(std::unique_ptr<CPUNode>& parent, uint64_t morton, size_t currentDepth);
	void linearize();
	void linearizeRecursive(std::unique_ptr<CPUNode>& node, uint64_t nodeIndex, size_t currentDepth);
	void saveToFile();

	friend class cereal::access;
	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(maxDepth, nodes);
	}
};