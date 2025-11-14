//
// Created by 2401Lucas on 2025-11-06.
//

#ifndef GPU_PARTICLE_SIM_D3D12BINDLESSDESCRIPTORMANAGER_H
#define GPU_PARTICLE_SIM_D3D12BINDLESSDESCRIPTORMANAGER_H

#include "D3D12Common.h"
#include <vector>
#include <queue>
#include <mutex>
#include <unordered_map>

#include "Rendering/RHI/BindlessDescriptorManager.h"

// Descriptor counts in single CBV_SRV_UAV heap
constexpr uint32_t MAX_BINDLESS_SRVS = 90000; // SRVs at indices 0-89,999
constexpr uint32_t MAX_BINDLESS_UAVS = 10000; // UAVs at indices 90,000-99,999
constexpr uint32_t SRV_HEAP_START = 0;
constexpr uint32_t UAV_HEAP_START = MAX_BINDLESS_SRVS; // 90,000
constexpr uint32_t TOTAL_CBV_SRV_UAV_DESCRIPTORS = MAX_BINDLESS_SRVS + MAX_BINDLESS_UAVS;

constexpr uint32_t MAX_BINDLESS_SAMPLERS = 2048;

// Invalid descriptor index
constexpr uint32_t INVALID_DESCRIPTOR_INDEX = 0xFFFFFFFF;

// Bindless descriptor types
enum class BindlessDescriptorType {
    SRV, // Shader Resource View (textures, structured buffers for reading)
    UAV, // Unordered Access View (RWTextures, RWStructuredBuffers)
    CBV, // Constant Buffer View
    Sampler // Samplers
};

// Handle that represents a bindless resource
struct BindlessHandle {
    uint32_t index = INVALID_DESCRIPTOR_INDEX;
    BindlessDescriptorType type;

    bool IsValid() const { return index != INVALID_DESCRIPTOR_INDEX; }
};

class D3D12BindlessDescriptorManager : public BindlessDescriptorManager {
public:
    D3D12BindlessDescriptorManager(ID3D12Device *device);

    ~D3D12BindlessDescriptorManager() override;

    // Initialize the bindless system
    void Initialize();

    // Allocate a descriptor slot and create the view
    BindlessHandle AllocateSRV(ID3D12Resource *resource, const D3D12_SHADER_RESOURCE_VIEW_DESC *desc);

    BindlessHandle AllocateUAV(ID3D12Resource *resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC *desc);

    BindlessHandle AllocateCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC *desc);

    BindlessHandle AllocateSampler(const D3D12_SAMPLER_DESC *desc);

    // Free a descriptor slot
    void Free(BindlessHandle handle);

    D3D12_GPU_DESCRIPTOR_HANDLE GetResourceGPUHandle(uint32_t index) const;

    D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerGPUHandle(uint32_t index) const;

    uint32_t GetDefaultLinearSampler() const;

    uint32_t GetDefaultPointSampler() const;

    uint32_t GetDefaultAnisotropicSampler() const;

    ID3D12RootSignature *GetRootSignature() const { return m_rootSignature; }

    ID3D12DescriptorHeap *GetResourceHeap() const { return m_resourceHeap.Get(); }
    ID3D12DescriptorHeap *GetSamplerHeap() const { return m_samplerHeap.Get(); }

    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHeapStart() const;

    D3D12_GPU_DESCRIPTOR_HANDLE GetUAVHeapStart() const;

    D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerHeapStart() const;

private:
    void CreateDefaultSamplers();

    friend class D3D12Device;

    ID3D12Device *m_device;
    ID3D12RootSignature *m_rootSignature;

    // Main descriptor heaps (shader-visible)
    ComPtr<ID3D12DescriptorHeap> m_resourceHeap; // CBV/SRV/UAV
    ComPtr<ID3D12DescriptorHeap> m_samplerHeap;

    // Descriptor sizes
    uint32_t m_resourceDescriptorSize;
    uint32_t m_samplerDescriptorSize;

    // Free list management
    std::queue<uint32_t> m_freeSRVIndices;
    std::queue<uint32_t> m_freeUAVIndices;
    std::queue<uint32_t> m_freeSamplerIndices;

    uint32_t m_nextSRVIndex = SRV_HEAP_START;
    uint32_t m_nextUAVIndex = UAV_HEAP_START;
    uint32_t m_nextSamplerIndex = 0;

    std::mutex m_resourceMutex;
    std::mutex m_samplerMutex;

    std::unordered_map<uint32_t, ID3D12Resource *> m_allocatedResources;

    BindlessHandle m_defaultLinearSampler;
    BindlessHandle m_defaultPointSampler;
    BindlessHandle m_defaultAnisotropicSampler;
};

#endif //GPU_PARTICLE_SIM_D3D12BINDLESSDESCRIPTORMANAGER_H
