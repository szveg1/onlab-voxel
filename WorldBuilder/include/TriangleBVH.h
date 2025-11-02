#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct Triangle {
    glm::vec3 v0, v1, v2;
    glm::vec2 uv0, uv1, uv2;
    uint32_t materialIndex;
};

struct BVHNode {
    glm::vec3 aabbMin, aabbMax;
    std::vector<size_t> triangleIndices;
    std::unique_ptr<BVHNode> children[2];
    bool isLeaf = true;

    bool intersects(const glm::vec3& boxMin, const glm::vec3& boxMax) const {
        return !(aabbMax.x < boxMin.x || aabbMin.x > boxMax.x ||
            aabbMax.y < boxMin.y || aabbMin.y > boxMax.y ||
            aabbMax.z < boxMin.z || aabbMin.z > boxMax.z);
    }
};

class TriangleBVH {
public:
    void build(const std::vector<Triangle>& triangles);
    void query(const glm::vec3& boxMin, const glm::vec3& boxMax, std::vector<size_t>& outIndices);
private:
    std::unique_ptr<BVHNode> root;
    std::unique_ptr<BVHNode> buildRecursive(const std::vector<Triangle>& triangles, std::vector<size_t>& indices, int depth);
    void queryRecursive(const BVHNode* node, const glm::vec3& boxMin, const glm::vec3& boxMax, std::vector<size_t>& outIndices);
};

inline bool planeBoxOverlap(const glm::vec3& normal, const glm::vec3& vert, const glm::vec3& maxbox) {
    glm::vec3 vmin, vmax;
    for (int q = 0; q < 3; q++) {
        float v = vert[q];
        if (normal[q] > 0.0f) {
            vmin[q] = -maxbox[q] - v;
            vmax[q] = maxbox[q] - v;
        }
        else {
            vmin[q] = maxbox[q] - v;
            vmax[q] = -maxbox[q] - v;
        }
    }
    if (glm::dot(normal, vmin) > 0.0f) return false;
    if (glm::dot(normal, vmax) >= 0.0f) return true;
    return false;
}

static inline bool axisTestX01(float a, float b, float fa, float fb, const glm::vec3& v0, const glm::vec3& v2, const glm::vec3& boxhalfsize) {
    float p0 = a * v0.y - b * v0.z;
    float p2 = a * v2.y - b * v2.z;
    float min = std::min(p0, p2);
    float max = std::max(p0, p2);
    float rad = fa * boxhalfsize.y + fb * boxhalfsize.z;
    return !(min > rad || max < -rad);
}

static inline bool axisTestX2(float a, float b, float fa, float fb, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& boxhalfsize) {
    float p0 = a * v0.y - b * v0.z;
    float p1 = a * v1.y - b * v1.z;
    float min = std::min(p0, p1);
    float max = std::max(p0, p1);
    float rad = fa * boxhalfsize.y + fb * boxhalfsize.z;
    return !(min > rad || max < -rad);
}

static inline bool axisTestY02(float a, float b, float fa, float fb, const glm::vec3& v0, const glm::vec3& v2, const glm::vec3& boxhalfsize) {
    float p0 = -a * v0.x + b * v0.z;
    float p2 = -a * v2.x + b * v2.z;
    float min = std::min(p0, p2);
    float max = std::max(p0, p2);
    float rad = fa * boxhalfsize.x + fb * boxhalfsize.z;
    return !(min > rad || max < -rad);
}

static inline bool axisTestY1(float a, float b, float fa, float fb, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& boxhalfsize) {
    float p0 = -a * v0.x + b * v0.z;
    float p1 = -a * v1.x + b * v1.z;
    float min = std::min(p0, p1);
    float max = std::max(p0, p1);
    float rad = fa * boxhalfsize.x + fb * boxhalfsize.z;
    return !(min > rad || max < -rad);
}

static inline bool axisTestZ12(float a, float b, float fa, float fb, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& boxhalfsize) {
    float p1 = a * v1.x - b * v1.y;
    float p2 = a * v2.x - b * v2.y;
    float min = std::min(p1, p2);
    float max = std::max(p1, p2);
    float rad = fa * boxhalfsize.x + fb * boxhalfsize.y;
    return !(min > rad || max < -rad);
}

static inline bool axisTestZ0(float a, float b, float fa, float fb, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& boxhalfsize) {
    float p0 = a * v0.x - b * v0.y;
    float p1 = a * v1.x - b * v1.y;
    float min = std::min(p0, p1);
    float max = std::max(p0, p1);
    float rad = fa * boxhalfsize.x + fb * boxhalfsize.y;
    return !(min > rad || max < -rad);
}

static bool triangleAABBIntersect(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& boxcenter, const glm::vec3& boxhalfsize) {
    glm::vec3 tv0 = v0 - boxcenter;
    glm::vec3 tv1 = v1 - boxcenter;
    glm::vec3 tv2 = v2 - boxcenter;

    glm::vec3 triMin = glm::min(glm::min(tv0, tv1), tv2);
    glm::vec3 triMax = glm::max(glm::max(tv0, tv1), tv2);

    if (triMin.x > boxhalfsize.x || triMax.x < -boxhalfsize.x) return false;
    if (triMin.y > boxhalfsize.y || triMax.y < -boxhalfsize.y) return false;
    if (triMin.z > boxhalfsize.z || triMax.z < -boxhalfsize.z) return false;

    glm::vec3 e0 = tv1 - tv0;
    glm::vec3 e1 = tv2 - tv1;
    glm::vec3 e2 = tv0 - tv2;

    float fex = glm::abs(e0.x);
    float fey = glm::abs(e0.y);
    float fez = glm::abs(e0.z);

    if (!axisTestX01(e0.z, e0.y, fez, fey, tv0, tv2, boxhalfsize)) return false;
    if (!axisTestY02(e0.z, e0.x, fez, fex, tv0, tv2, boxhalfsize)) return false;
    if (!axisTestZ12(e0.y, e0.x, fey, fex, tv1, tv2, boxhalfsize)) return false;

    fex = glm::abs(e1.x);
    fey = glm::abs(e1.y);
    fez = glm::abs(e1.z);

    if (!axisTestX01(e1.z, e1.y, fez, fey, tv0, tv2, boxhalfsize)) return false;
    if (!axisTestY02(e1.z, e1.x, fez, fex, tv0, tv2, boxhalfsize)) return false;
    if (!axisTestZ0(e1.y, e1.x, fey, fex, tv0, tv1, boxhalfsize)) return false;

    fex = glm::abs(e2.x);
    fey = glm::abs(e2.y);
    fez = glm::abs(e2.z);

    if (!axisTestX2(e2.z, e2.y, fez, fey, tv0, tv1, boxhalfsize)) return false;
    if (!axisTestY1(e2.z, e2.x, fez, fex, tv0, tv1, boxhalfsize)) return false;
    if (!axisTestZ12(e2.y, e2.x, fey, fex, tv1, tv2, boxhalfsize)) return false;

    glm::vec3 normal = glm::cross(e0, e1);
    return planeBoxOverlap(normal, tv0, boxhalfsize);
}