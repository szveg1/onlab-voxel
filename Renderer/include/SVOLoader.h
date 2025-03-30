#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

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

class SVOLoader {
public:
    std::vector<GPUNode> nodes;
    size_t maxDepth;
    void load();
    void uploadToGPU();
	GLuint getDepth() { return static_cast<GLuint>(maxDepth); }
private:
    GLuint ssbo, nodeCounter;
    friend class cereal::access;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(maxDepth, nodes);
    }
};