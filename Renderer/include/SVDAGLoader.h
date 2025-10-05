#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct SVDAGGPUNode {
    uint8_t childMask;
    uint32_t refs;
    uint16_t material;
    uint32_t children[8];

	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(childMask);
        ar(refs);
        ar(material);
		for (size_t i = 0; i < 8; i++)
		{
			ar(children[i]);
		}
	}
};

class SVDAGLoader {
public:
    std::vector<SVDAGGPUNode> nodes;
    size_t maxDepth;
    uint32_t maxRefs;
    void load();
    void uploadToGPU();
    GLuint getDepth() { return static_cast<GLuint>(maxDepth); }


private:
    GLuint ssbo, nodeCounter;
    friend class cereal::access;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(maxDepth, maxRefs, nodes);
    }
};