//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_BUFFER_H
#define GPU_PARTICLE_SIM_BUFFER_H
#include <cstdint>

enum class BufferUsage {
    Vertex      = 1 << 0,
    Index       = 1 << 1,
    Constant    = 1 << 2,
    Storage     = 1 << 3,
    Indirect    = 1 << 4,
    Transfer    = 1 << 5,
};

enum MemoryType {
    GPU,      // Device-local, fast for GPU, no CPU access
    Upload,   // CPU → GPU transfers, mappable
    Readback  // GPU → CPU transfers, mappable
};

struct BufferCreateInfo {
    uint64_t size;
    BufferUsage usage;
    MemoryType memoryType;
    const char* debugName = nullptr;
};

class Buffer {
public:
    virtual ~Buffer() = default;

    // Map/Unmap for CPU access (only valid for Upload/Readback buffers)
    virtual void* Map() = 0;
    virtual void Unmap() = 0;

    // Getters
    virtual uint64_t GetSize() const = 0;
    virtual uint64_t GetGPUAddress() const = 0;

    // Internal - get native handle (D3D12Resource, VkBuffer, etc.)
    virtual void* GetNativeHandle() const = 0;
};


#endif //GPU_PARTICLE_SIM_BUFFER_H
