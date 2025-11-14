//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_BUFFER_H
#define GPU_PARTICLE_SIM_BUFFER_H
#include <cstdint>

enum class BufferUsage {
    Vertex = 1 << 0,
    Index = 1 << 1,
    Uniform = 1 << 2,
    Storage = 1 << 3,
    UnorderedAccess = 1 << 4,
    CopySource = 1 << 5,
    CopyDest = 1 << 6,
};

enum MemoryType {
    GPU, // Device-local, fast for GPU, no CPU access
    Upload, // CPU → GPU transfers, mappable
    Readback // GPU → CPU transfers, mappable
};

struct BufferCreateInfo {
    uint64_t size;
    uint32_t stride;
    BufferUsage usage;
    MemoryType memoryType;
    const char *debugName = nullptr;
};

class Buffer {
public:
    virtual ~Buffer() = default;

    // Map/Unmap for CPU access (only valid for Upload/Readback buffers)
    virtual void *Map() = 0;

    virtual void *GetMappedPtr() const = 0;

    virtual void Unmap() = 0;

    // Getters
    virtual uint64_t GetSize() const = 0;

    virtual uint64_t GetGPUAddress() const = 0;

    virtual uint32_t GetBindlessIndex() const = 0;
};


#endif //GPU_PARTICLE_SIM_BUFFER_H
