//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_MESHLOADER_H
#define GPU_PARTICLE_SIM_MESHLOADER_H

#include <cstdint>
#include <vector>
#include <string>

struct Vertex {
    float position[3];
    float normal[3];
    float texCoord[2];
    float tangent[3];
};

struct SubMesh {
    uint32_t startIndex;
    uint32_t indexCount;
    std::string materialName;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh> subMeshes;
    std::string name;

    // Bounding box
    float boundsMin[3] = {0, 0, 0};
    float boundsMax[3] = {0, 0, 0};
};

class MeshLoader {
public:
    // Load mesh from file (auto-detects format)
    static MeshData LoadFromFile(const std::string& path);

    // Format-specific loaders
    static MeshData LoadOBJ(const std::string& path);
    static MeshData LoadFBX(const std::string& path);
    static MeshData LoadGLTF(const std::string& path);

private:
    static void CalculateBounds(MeshData& mesh);
    static void CalculateTangents(MeshData& mesh);
};



#endif //GPU_PARTICLE_SIM_MESHLOADER_H