#include "SVDAGEditor.h"

SVDAGEditor::SVDAGEditor(std::vector<SVDAGGPUNode>& nodes, uint32_t treeDepth)
	: nodes(nodes), maxDepth(treeDepth)
w{
    clearModifiedLists();
}

SVDAGEditor::~SVDAGEditor()
{
}

uint32_t SVDAGEditor::ensureNodeIsMutable(uint32_t nodeIndex)
{
	if (nodes[nodeIndex].refs > 1) {
		nodes[nodeIndex].refs--;
		modifiedIndices.push_back(nodeIndex);
		
		SVDAGGPUNode newNode = nodes[nodeIndex];
		newNode.refs = 1;

		uint32_t newIndex = nodes.size();
		nodes.push_back(newNode);
		return newIndex;
	}

	modifiedIndices.push_back(nodeIndex);
	return nodeIndex;
}

bool SVDAGEditor::setVoxel(const glm::vec3& worldPos, uint16_t material) {
	if (glm::any(glm::lessThan(worldPos, glm::vec3(0))) || glm::any(glm::greaterThanEqual(worldPos, glm::vec3(1)))) {
		return false;
	}
	rootNodeIndex = recursiveModify(rootNodeIndex, worldPos, 0, true, material);
	return true;
}

bool SVDAGEditor::clearVoxel(const glm::vec3& worldPos) {
	if (glm::any(glm::lessThan(worldPos, glm::vec3(0))) || glm::any(glm::greaterThanEqual(worldPos, glm::vec3(1)))) {
		return false;
	}
	rootNodeIndex = recursiveModify(rootNodeIndex, worldPos, 0, false, 0);
	return true;
}

uint32_t SVDAGEditor::recursiveModify(uint32_t nodeIndex, const glm::vec3& targetPos, uint32_t currentDepth, bool addVoxel, uint16_t material) {
    uint32_t mutableNodeIndex = ensureNodeIsMutable(nodeIndex);
    SVDAGGPUNode& node = nodes[mutableNodeIndex];

    if (currentDepth == maxDepth - 1) {
        glm::vec3 relPos = glm::fract(targetPos * exp2f(currentDepth));
        uint32_t octant = (relPos.x > 0.5f ? 1 : 0) | (relPos.y > 0.5f ? 2 : 0) | (relPos.z > 0.5f ? 4 : 0);
        uint32_t childBit = 1u << octant;

        if (addVoxel) {
            node.childMask |= childBit;
            node.material = material;
        }
        else {
            node.childMask &= ~childBit;
        }
        return mutableNodeIndex;
    }

    glm::vec3 relPos = glm::fract(targetPos * exp2f(currentDepth));
    uint32_t octant = (relPos.x > 0.5f ? 1 : 0) | (relPos.y > 0.5f ? 2 : 0) | (relPos.z > 0.5f ? 4 : 0);
    uint32_t childBit = 1u << octant;

    if ((node.childMask & childBit) == 0 && addVoxel) {
        SVDAGGPUNode newNode = {};
        newNode.refs = 1;
        uint32_t newChildIndex = nodes.size();
        nodes.push_back(newNode);

        node.children[octant] = newChildIndex;
        node.childMask |= childBit;
    }
    else if ((node.childMask & childBit) == 0) {
        return mutableNodeIndex;
    }

    uint32_t oldChildIndex = node.children[octant];
    uint32_t newChildIndex = recursiveModify(oldChildIndex, targetPos, currentDepth + 1, addVoxel, material);

    if (newChildIndex != oldChildIndex) {
        node.children[octant] = newChildIndex;
        nodes[oldChildIndex].refs--;
        modifiedIndices.push_back(oldChildIndex);
    }

    return mutableNodeIndex;
}
