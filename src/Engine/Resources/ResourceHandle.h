//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_RESOURCEHANDLE_H
#define GPU_PARTICLE_SIM_RESOURCEHANDLE_H

#include <cstdint>

// Type-safe handle
template<typename T>
struct ResourceHandle {
    uint64_t id = 0;
    uint32_t generation = 0; // Detect use-after-free

    bool IsValid() const { return id != 0; }

    bool operator==(const ResourceHandle &other) const {
        return id == other.id && generation == other.generation;
    }
};

// Specific resource types
struct MeshTag {
};

struct TextureTag {
};

struct MaterialTag {
};

struct PipelineTag {
};

using MeshHandle = ResourceHandle<MeshTag>;
using TextureHandle = ResourceHandle<TextureTag>;
using MaterialHandle = ResourceHandle<MaterialTag>;
using PipelineHandle = ResourceHandle<PipelineTag>;

#endif //GPU_PARTICLE_SIM_RESOURCEHANDLE_H
