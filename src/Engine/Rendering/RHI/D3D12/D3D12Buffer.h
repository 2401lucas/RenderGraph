//
// Created by 2401Lucas on 2025-11-04.
//

#ifndef GPU_PARTICLE_SIM_D3D12BUFFER_H
#define GPU_PARTICLE_SIM_D3D12BUFFER_H

#include "../Buffer.h"
#include "D3D12Common.h"

struct D3D12Buffer : public Buffer {
    ComPtr<ID3D12Resource> resource;
    size_t size = 0;
    uint32_t stride = 0; // For vertex/index buffers
    BufferUsage usage = BufferUsage::Vertex;
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
    void *mappedData = nullptr;

    D3D12Buffer() = default;

    void * GetMappedPtr() const override {
        return mappedData;
    }

    uint64_t GetSize() const override {
        return size;
    }

    uint64_t GetGPUAddress() const override {
        return gpuAddress;
    }

    ~D3D12Buffer() override {
        if (mappedData && resource) {
            resource->Unmap(0, nullptr);
            mappedData = nullptr;
        }
    }

    // Map buffer for CPU access (for Upload/Readback heaps)
    void *Map() override {
        if (!mappedData && resource) {
            D3D12_RANGE readRange = {0, 0}; // We don't intend to read from this buffer on CPU
            if (SUCCEEDED(resource->Map(0, &readRange, &mappedData))) {
                return mappedData;
            }
        }
        return mappedData;
    }

    // Unmap buffer
    void Unmap() override {
        if (mappedData && resource) {
            D3D12_RANGE writtenRange = {0, size}; // We wrote the entire buffer
            resource->Unmap(0, &writtenRange);
            mappedData = nullptr;
        }
    }

    // Get GPU virtual address (cached for performance)
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() {
        if (gpuAddress == 0 && resource) {
            gpuAddress = resource->GetGPUVirtualAddress();
        }
        return gpuAddress;
    }
};

#endif //GPU_PARTICLE_SIM_D3D12BUFFER_H
