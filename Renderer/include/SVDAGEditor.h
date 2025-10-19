#pragma once
#include "SVDAGLoader.h"

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

class SVDAGEditor
{
public:
	SVDAGEditor(std::vector<SVDAGGPUNode>& nodes, uint32_t treeDepth);
	~SVDAGEditor();

	bool setVoxel(const glm::vec3& worldPos, uint16_t material);
	bool clearVoxel(const glm::vec3& worldPos);

	const std::vector<uint32_t>& getModifiedIndices() const { return modifiedIndices; }
	size_t getNewNodesStartIndex() const { return originalNodeCount; }
	void clearModifiedLists() {
		modifiedIndices.clear();
		originalNodeCount = nodes.size();
	}
	void uploadChangesToGPU();
private:
	std::vector<SVDAGGPUNode>& nodes;
	uint32_t rootNodeIndex = 0;
	uint32_t maxDepth;

	size_t originalNodeCount;
	std::vector<uint32_t> modifiedIndices;

	uint32_t recursiveModify(uint32_t nodeIndex, const glm::vec3& targetPos, uint32_t currentDepth, bool addVoxel, uint16_t material);


	uint32_t ensureNodeIsMutable(uint32_t nodeIndex);
};