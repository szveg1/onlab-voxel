#version 450
#extension GL_ARB_gpu_shader_int64: enable
precision highp float;

out vec4 fragmentColor;
in vec4 rayDir;

uniform struct {
    mat4 rayDirMatrix;
    vec3 position;
} camera;

struct VoxelNode {
    uint childMask;
    uint64_t childIndex;
};

layout(std430, binding = 0) buffer VoxelNodes {
    VoxelNode nodes[];
};

uniform uint treeDepth;

uint countBitsBefore(uint mask, uint bitPosition) {
    return bitCount(mask & ((1u << bitPosition) - 1u));
}

bool traverseOctree(vec3 p)
{
    uint64_t nodeIndex = 0;
    float nodeSize = 1.0;
    vec3 nodeCenter = vec3(0.5);

    for(int depth = 0; depth < treeDepth; depth++) {
        // TODO: This is bad, we might lose precision here
        VoxelNode node = nodes[int(nodeIndex)];

        uint octant = 0;
        if(p.x > nodeCenter.x) octant |= 1;
        if(p.y > nodeCenter.y) octant |= 2;
        if(p.z > nodeCenter.z) octant |= 4;

        uint childBit = 1u << octant;

        if(depth == treeDepth - 1) {
            if((node.childMask & childBit) != 0) {
                return true;
            }
        }

        if((node.childMask & childBit) == 0) {
            return false;
        }

        uint childOffset = countBitsBefore(node.childMask, octant);
        nodeIndex = node.childIndex + childOffset;
       
        nodeSize *= 0.5;
        nodeCenter += nodeSize * vec3(
            (octant & 1) != 0 ? 0.5 : -0.5,
            (octant & 2) != 0 ? 0.5 : -0.5,
            (octant & 4) != 0 ? 0.5 : -0.5
        );
    }
    return false;
}

vec3 getMountainColor(vec3 position) {
    float maxY = 1.0;
    
    // Normalize the y position based on maxY
    float normalizedY = position.y / maxY;
    
    // Define elevation zones for mountain coloring
    float snowLine = 0.75;
    float rockLine = 0.45;
    float grassLine = 0.25;
    
    // Get noise variation based on position
    float noise1 = sin(position.x * 50.0) * cos(position.z * 50.0) * 0.05;
    float noise2 = sin(position.x * 120.0 + position.z * 75.0) * 0.07;
    
    vec3 mountainColor;
    
    // Snow caps (highest elevation)
    if (normalizedY > snowLine) {
        float snowBlend = smoothstep(snowLine, 0.9, normalizedY);
        vec3 shadedSnow = vec3(0.8, 0.85, 0.95);
        vec3 brightSnow = vec3(1.0, 1.0, 1.0);
        mountainColor = mix(shadedSnow, brightSnow, snowBlend);
        mountainColor += vec3(noise1);
    }
    // Rocky terrain (high elevation)
    else if (normalizedY > rockLine) {
        float rockBlend = (normalizedY - rockLine) / (snowLine - rockLine);
        vec3 lowerRock = vec3(0.5, 0.4, 0.35); // Brown rock
        vec3 upperRock = vec3(0.7, 0.7, 0.75); // Gray rock near snow
        mountainColor = mix(lowerRock, upperRock, rockBlend);
        mountainColor += vec3(noise2);
    }
    // Alpine meadows / sparse vegetation
    else if (normalizedY > grassLine) {
        float meadowBlend = (normalizedY - grassLine) / (rockLine - grassLine);
        vec3 alpineMeadow = vec3(0.3, 0.5, 0.2); // Green meadow
        vec3 rockyTerrain = vec3(0.45, 0.38, 0.32); // Rocky soil
        mountainColor = mix(alpineMeadow, rockyTerrain, meadowBlend);
        mountainColor += vec3(noise1 * 2.0);
    }
    // Forest/foothills (lowest elevation)
    else {
        float forestBlend = normalizedY / grassLine;
        vec3 darkForest = vec3(0.1, 0.25, 0.1); // Dark forest
        vec3 lightForest = vec3(0.2, 0.35, 0.15); // Lighter forest
        mountainColor = mix(darkForest, lightForest, forestBlend);
        mountainColor += vec3(noise2 * 1.5);
    }
    
    return mountainColor;
}

void main(void) {
    vec3 d = normalize(rayDir.xyz);
    float t1 = -camera.position.y / d.y;
    float t2 = (1.0 - camera.position.y) / d.y;
    float tstart = max(min(t1, t2), 0.0);
    float tend = max(t1, t2);

    bool found = false;
    vec3 p;

    if (tstart < tend) {
        p = camera.position + d * tstart;
        vec3 step = d * min((tend - tstart) / 580.0, 0.001);
        for (int i = 0; i < 512; i++) {
            p += step;
            step *= 1.002;
            if(p.x > 0 && p.y > 0 && p.z > 0 && p.x < 1 && p.y < 1 && p.z < 1) {
                if (traverseOctree(p)) {
                    found = true; 
                    break;
                }
            }
        }
        if (found) {
            vec3 lastStep = step / 1.02;
            vec3 midPoint =  p - lastStep / 2.0;
            step = lastStep / 2.0;
            for (int i = 0; i < 128; i++) {
                if (traverseOctree(p)) {
                    p = midPoint;
                    midPoint -= step;
                } else {
                    midPoint += step;
                }
                step /= 2.0;
            }
        }
    }

    if (!found) {
        discard;
    } else {
        fragmentColor = vec4(getMountainColor(p), 1.0);
    }
}