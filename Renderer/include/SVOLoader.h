#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <vector>
#include <glad/glad.h>

struct GPUNode {
    uint8_t childMask;
    uint64_t childIndex;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(childMask, childIndex);
    }
};

class SVOLoader {
public:
    std::vector<GPUNode> nodes;
    size_t maxDepth;
    void load();
    void uploadToGPU();
	GLuint getDepth() { return static_cast<GLuint>(maxDepth); }
private:
    friend class cereal::access;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(maxDepth, nodes);
    }
};