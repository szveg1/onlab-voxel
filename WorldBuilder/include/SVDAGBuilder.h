#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>


struct CPUNode
{
	uint8_t childMask;
	std::shared_ptr<CPUNode> children[8];
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


class SVDAGBuilder
{
public:
	SVDAGBuilder(uint32_t treeSize, uint16_t heightMapSize, uint16_t chunkSize);
	~SVDAGBuilder();
	void build();
private:
	uint32_t treeSize;
	uint16_t heightMapSize;
	uint16_t chunkSize;
	size_t maxDepth;

	std::unordered_map<uint64_t, std::shared_ptr<CPUNode>> subtrees;


	std::vector<GPUNode> nodes;
	std::shared_ptr<CPUNode> root;
	std::unordered_map<size_t, std::shared_ptr<CPUNode>> nodeCache;
	std::unordered_map<std::shared_ptr<CPUNode>, size_t> nodeToIndexMap;


	void insertNodeRecursive(std::shared_ptr<CPUNode>& parent, uint64_t morton, size_t currentDepth);
	size_t calculateNodeHash(std::shared_ptr<CPUNode>& node);
	void reduceTreeRecursive(std::shared_ptr<CPUNode>& node, size_t currentDepth);
	void mergeSubtrees();
	void linearize();
	void linearizeRecursive(std::shared_ptr<CPUNode>& node, uint64_t nodeIndex, size_t currentDepth);
	void saveToFile();

	friend class cereal::access;
	template <class Archive>
	void serialize(Archive& archive)
	{
		archive(maxDepth, nodes);
	}
};