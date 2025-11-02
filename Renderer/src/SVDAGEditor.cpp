#include "SVDAGEditor.h"

SVDAGEditor::SVDAGEditor(std::vector<SVDAGGPUNode>& nodes, uint32_t treeDepth)
	: nodes(nodes), maxDepth(treeDepth)
{
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

bool SVDAGEditor::paintVoxel(const glm::vec3& worldPos, uint16_t material) {
    if (glm::any(glm::lessThan(worldPos, glm::vec3(0))) || glm::any(glm::greaterThanEqual(worldPos, glm::vec3(1)))) {
        return false;
    }
    rootNodeIndex = recursivePaint(rootNodeIndex, worldPos, 0, material);
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

uint32_t SVDAGEditor::recursivePaint(uint32_t nodeIndex, const glm::vec3& targetPos, uint32_t currentDepth, uint16_t material) {
    if (nodeIndex == 0) {
        return nodeIndex;
    }

    if (currentDepth == maxDepth - 1) {
        glm::vec3 relPos = glm::fract(targetPos * exp2f(currentDepth));
        uint32_t octant = (relPos.x > 0.5f ? 1 : 0) | (relPos.y > 0.5f ? 2 : 0) | (relPos.z > 0.5f ? 4 : 0);
        uint32_t childBit = 1u << octant;

        if ((nodes[nodeIndex].childMask & childBit) != 0) {
            uint32_t mutableNodeIndex = ensureNodeIsMutable(nodeIndex);
            nodes[mutableNodeIndex].material = material;
            return mutableNodeIndex;
        }
        else {
            return nodeIndex;
        }
    }

    if (nodes[nodeIndex].childMask == 0xFFu && nodes[nodeIndex].children[0] == 0u) {
        uint32_t mutableNodeIndex = ensureNodeIsMutable(nodeIndex);
        nodes[mutableNodeIndex].material = material;
        return mutableNodeIndex;
    }

    glm::vec3 relPos = glm::fract(targetPos * exp2f(currentDepth));
    uint32_t octant = (relPos.x > 0.5f ? 1 : 0) | (relPos.y > 0.5f ? 2 : 0) | (relPos.z > 0.5f ? 4 : 0);
    uint32_t childBit = 1u << octant;

    if ((nodes[nodeIndex].childMask & childBit) == 0) {
        return nodeIndex;
    }

    uint32_t mutableNodeIndex = ensureNodeIsMutable(nodeIndex);
    SVDAGGPUNode& node = nodes[mutableNodeIndex];

    uint32_t oldChildIndex = node.children[octant];
    uint32_t newChildIndex = recursivePaint(oldChildIndex, targetPos, currentDepth + 1, material);

    if (newChildIndex != oldChildIndex) {
        node.children[octant] = newChildIndex;
        nodes[oldChildIndex].refs--;
        modifiedIndices.push_back(oldChildIndex);
    }

    return mutableNodeIndex;
}

void SVDAGEditor::modifyRegion(const Box& targetBox, bool addVoxels, uint16_t material) {
    uint32_t resolution = 1 << maxDepth;
    Box rootBox = { glm::uvec3(0), glm::uvec3(resolution - 1) };

    rootNodeIndex = recursiveModifyRegion(rootNodeIndex, targetBox, rootBox, 0, addVoxels, material);
}

uint32_t SVDAGEditor::recursiveModifyRegion(uint32_t nodeIndex, const Box& targetBox, const Box& nodeBox, uint32_t currentDepth, bool addVoxels, uint16_t material) {

    if (!boxesIntersect(targetBox, nodeBox)) {
        return nodeIndex;
    }

    if (boxContains(targetBox, nodeBox)) {
        if (addVoxels) {
            uint32_t newLeafIndex = createSolidLeafNode(material);
            return newLeafIndex;
        }
        else {
            return 0;
        }
    }

    if (currentDepth == maxDepth - 1) {
        uint32_t mutableNodeIndex = ensureNodeIsMutable(nodeIndex);
        SVDAGGPUNode& node = nodes[mutableNodeIndex];
        for (uint32_t octant = 0; octant < 8; ++octant) {
            Box voxelBox = getChildBox(nodeBox, octant);
            if (boxesIntersect(targetBox, voxelBox)) {
                uint32_t childBit = 1u << octant;
                if (addVoxels) {
                    node.childMask |= childBit;
                    node.material = material;
                }
                else {
                    node.childMask &= ~childBit;
                }
            }
        }
        return mutableNodeIndex;
    }

    if (nodes[nodeIndex].childMask == 0xFFu && nodes[nodeIndex].children[0] == 0u) {

        uint32_t mutableNodeIndex = ensureNodeIsMutable(nodeIndex);
        SVDAGGPUNode& node = nodes[mutableNodeIndex];
        uint16_t originalMaterial = node.material;

        for (int i = 0; i < 8; i++) {
            node.children[i] = createSolidLeafNode(originalMaterial);
        }

        node.material = 0;
        nodeIndex = mutableNodeIndex;
    }

    uint32_t mutableNodeIndex = ensureNodeIsMutable(nodeIndex);
    SVDAGGPUNode& node = nodes[mutableNodeIndex];

    for (uint32_t octant = 0; octant < 8; ++octant) {
        Box childNodeBox = getChildBox(nodeBox, octant);

        if (!boxesIntersect(targetBox, childNodeBox)) {
            continue;
        }

        uint32_t childBit = 1u << octant;
        uint32_t oldChildIndex = 0;

        if ((node.childMask & childBit) == 0) {
            if (!addVoxels) {
                continue;
            }

            SVDAGGPUNode newNode = {};
            newNode.refs = 1;
            uint32_t newChildIndex = nodes.size();
            nodes.push_back(newNode);

            node.children[octant] = newChildIndex;
            node.childMask |= childBit;
            oldChildIndex = newChildIndex;
        }
        else {
            oldChildIndex = node.children[octant];
        }

        uint32_t newChildIndex = recursiveModifyRegion(
            oldChildIndex,
            targetBox,
            childNodeBox,
            currentDepth + 1,
            addVoxels,
            material
        );

        if (newChildIndex != oldChildIndex) {
            node.children[octant] = newChildIndex;

            if (oldChildIndex != 0) {
                nodes[oldChildIndex].refs--;
                modifiedIndices.push_back(oldChildIndex);
            }

            if (newChildIndex == 0) {
                node.childMask &= ~childBit;
            }
        }
    }

    return mutableNodeIndex;
}

uint32_t SVDAGEditor::createSolidLeafNode(uint16_t material) {
    SVDAGGPUNode solidNode = {};
    solidNode.refs = 1;
    solidNode.childMask = 0xFF;
    solidNode.material = material;

    uint32_t newIndex = nodes.size();
    nodes.push_back(solidNode);
    return newIndex;
}

bool SVDAGEditor::boxesIntersect(const Box& a, const Box& b) const {
    bool overlapX = a.min.x <= b.max.x && a.max.x >= b.min.x;
    bool overlapY = a.min.y <= b.max.y && a.max.y >= b.min.y;
    bool overlapZ = a.min.z <= b.max.z && a.max.z >= b.min.z;

    return overlapX && overlapY && overlapZ;
}

bool SVDAGEditor::boxContains(const Box& container, const Box& content) const {
    bool minCheck = glm::all(glm::lessThanEqual(container.min, content.min));
    bool maxCheck = glm::all(glm::greaterThanEqual(container.max, content.max));

    return minCheck && maxCheck;
}

Box SVDAGEditor::getChildBox(const Box& parentBox, uint32_t octant) const {
    glm::uvec3 parentSize = parentBox.max - parentBox.min + glm::uvec3(1);
    glm::uvec3 childSize = parentSize / 2u;

    glm::uvec3 childMin = parentBox.min;

    if (octant & 1) {
        childMin.x += childSize.x;
    }
    if (octant & 2) {
        childMin.y += childSize.y;
    }
    if (octant & 4) {
        childMin.z += childSize.z;
    }

    glm::uvec3 parentMid = parentBox.min + childSize;

    Box childBox;
    childBox.min.x = (octant & 1) ? parentMid.x : parentBox.min.x;
    childBox.min.y = (octant & 2) ? parentMid.y : parentBox.min.y;
    childBox.min.z = (octant & 4) ? parentMid.z : parentBox.min.z;

    childBox.max.x = (octant & 1) ? parentBox.max.x : parentMid.x - 1;
    childBox.max.y = (octant & 2) ? parentBox.max.y : parentMid.y - 1;
    childBox.max.z = (octant & 4) ? parentBox.max.z : parentMid.z - 1;

    return childBox;
}