//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_MESH_H
#define GPU_PARTICLE_SIM_MESH_H


#include <cstdint>

#include "MeshLoader.h"
#include "Rendering/RHI/Buffer.h"
#include "Rendering/RHI/Device.h"

class Mesh {
public:
    Mesh(Device *renderer, const MeshData &data);

    ~Mesh();

    // GPU resources
    Buffer *GetVertexBuffer() const { return m_vertexBuffer; }
    Buffer *GetIndexBuffer() const { return m_indexBuffer; }
    uint32_t GetVertexCount() const { return m_vertexCount; }
    uint32_t GetIndexCount() const { return m_indexCount; }

    // CPU data (optional, for physics/collision)
    const MeshData &GetCPUData() const { return m_cpuData; }

    // Bounding box
    void GetBounds(float min[3], float max[3]) const {
        for (int i = 0; i < 3; i++) {
            min[i] = m_cpuData.boundsMin[i];
            max[i] = m_cpuData.boundsMax[i];
        }
    }

    // Memory usage
    uint64_t GetGPUMemorySize() const { return m_gpuMemorySize; }

private:
    Device *m_device;
    Buffer *m_vertexBuffer = nullptr;
    Buffer *m_indexBuffer = nullptr;
    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;
    MeshData m_cpuData; // Keep CPU copy for physics
    uint64_t m_gpuMemorySize = 0;
};


#endif //GPU_PARTICLE_SIM_MESH_H
