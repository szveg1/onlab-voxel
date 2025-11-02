#pragma once
#include "SVDAGLoader.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

#include "Common.h"

class SVDAGEditor
{
public:
	SVDAGEditor(std::vector<SVDAGGPUNode>& nodes, uint32_t treeDepth);
	~SVDAGEditor();

	bool setVoxel(const glm::vec3& worldPos, uint16_t material);
	bool clearVoxel(const glm::vec3& worldPos);
	bool paintVoxel(const glm::vec3& worldPos, uint16_t material);

	const std::vector<uint32_t>& getModifiedIndices() const { return modifiedIndices; }
	size_t getNewNodesStartIndex() const { return originalNodeCount; }
	void clearModifiedLists() {
		modifiedIndices.clear();
		originalNodeCount = nodes.size();
	}
	
	void uploadChangesToGPU();
	void modifyRegion(const Box& targetBox, bool addVoxels, uint16_t material);
private:
	std::vector<SVDAGGPUNode>& nodes;
	uint32_t rootNodeIndex = 0;
	uint32_t maxDepth;

	size_t originalNodeCount;
	std::vector<uint32_t> modifiedIndices;

	uint32_t recursiveModify(uint32_t nodeIndex, const glm::vec3& targetPos, uint32_t currentDepth, bool addVoxel, uint16_t material);
	uint32_t recursivePaint(uint32_t nodeIndex, const glm::vec3& targetPos, uint32_t currentDepth, uint16_t material);
	uint32_t recursiveModifyRegion(uint32_t nodeIndex, const Box& targetBox, const Box& nodeBox, uint32_t currentDepth, bool addVoxels, uint16_t material);
	uint32_t createSolidLeafNode(uint16_t material);
	uint32_t ensureNodeIsMutable(uint32_t nodeIndex);
	bool boxesIntersect(const Box& a, const Box& b) const;
	bool boxContains(const Box& container, const Box& content) const;
	Box getChildBox(const Box& parentBox, uint32_t octant) const;
};