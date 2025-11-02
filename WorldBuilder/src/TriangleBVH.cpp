#include "TriangleBVH.h"

void TriangleBVH::build(const std::vector<Triangle>& triangles) {
    if (triangles.empty()) return;

    std::vector<size_t> indices(triangles.size());
    for (size_t i = 0; i < triangles.size(); i++) indices[i] = i;

    root = buildRecursive(triangles, indices, 0);
}

void TriangleBVH::query(const glm::vec3& boxMin, const glm::vec3& boxMax, std::vector<size_t>& outIndices) {
    if (!root) return;
    queryRecursive(root.get(), boxMin, boxMax, outIndices);
}

std::unique_ptr<BVHNode> TriangleBVH::buildRecursive(const std::vector<Triangle>& triangles, std::vector<size_t>& indices, int depth) {
    auto node = std::make_unique<BVHNode>();

    node->aabbMin = glm::vec3(std::numeric_limits<float>::max());
    node->aabbMax = glm::vec3(std::numeric_limits<float>::lowest());

    for (size_t idx : indices) {
        const auto& tri = triangles[idx];
        node->aabbMin = glm::min(node->aabbMin, glm::min(glm::min(tri.v0, tri.v1), tri.v2));
        node->aabbMax = glm::max(node->aabbMax, glm::max(glm::max(tri.v0, tri.v1), tri.v2));
    }

    if (indices.size() <= 8 || depth > 20) {
        node->isLeaf = true;
        node->triangleIndices = indices;
        return node;
    }

    glm::vec3 extent = node->aabbMax - node->aabbMin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;

    float splitPos = (node->aabbMin[axis] + node->aabbMax[axis]) * 0.5f;

    std::vector<size_t> leftIndices, rightIndices;
    for (size_t idx : indices) {
        const auto& tri = triangles[idx];
        float center = (tri.v0[axis] + tri.v1[axis] + tri.v2[axis]) / 3.0f;

        if (center < splitPos) {
            leftIndices.push_back(idx);
        }
        else {
            rightIndices.push_back(idx);
        }
    }

    if (leftIndices.empty() || rightIndices.empty()) {
        node->isLeaf = true;
        node->triangleIndices = indices;
        return node;
    }

    node->isLeaf = false;
    node->children[0] = buildRecursive(triangles, leftIndices, depth + 1);
    node->children[1] = buildRecursive(triangles, rightIndices, depth + 1);

    return node;
}

void TriangleBVH::queryRecursive(const BVHNode* node, const glm::vec3& boxMin, const glm::vec3& boxMax, std::vector<size_t>& outIndices) { 
    if (!node->intersects(boxMin, boxMax)) return;

    if (node->isLeaf) {
        outIndices.insert(outIndices.end(),
            node->triangleIndices.begin(),
            node->triangleIndices.end());
    }
    else {
        if (node->children[0]) queryRecursive(node->children[0].get(), boxMin, boxMax, outIndices);
        if (node->children[1]) queryRecursive(node->children[1].get(), boxMin, boxMax, outIndices);
    }
}