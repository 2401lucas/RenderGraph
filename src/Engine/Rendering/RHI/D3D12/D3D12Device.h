//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_D3D12DEVICE_H
#define GPU_PARTICLE_SIM_D3D12DEVICE_H

#include "../Device.h"

#include "D3D12Common.h"

#include <queue>
#include <mutex>
#include <unordered_map>

#include "D3D12Pipeline.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12CommandList.h"
#include "D3D12Swapchain.h"

// Descriptor heap allocator for managing descriptor handles
class DescriptorHeapAllocator {
public:
    DescriptorHeapAllocator(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

    D3D12_CPU_DESCRIPTOR_HANDLE AllocateCPU();

    D3D12_GPU_DESCRIPTOR_HANDLE AllocateGPU();

    void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    ID3D12DescriptorHeap *GetHeap() const { return m_heap.Get(); }

private:
    ComPtr<ID3D12DescriptorHeap> m_heap;
    uint32_t m_descriptorSize;
    uint32_t m_numDescriptors;
    uint32_t m_currentOffset;
    std::queue<uint32_t> m_freeList;
    std::mutex m_mutex;
};

// Command allocator pool for reusing allocators
class CommandAllocatorPool {
public:
    CommandAllocatorPool(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type);

    ComPtr<ID3D12CommandAllocator> RequestAllocator(uint64_t completedFenceValue);

    void DiscardAllocator(uint64_t fenceValue, ComPtr<ID3D12CommandAllocator> allocator);

private:
    struct AllocatorEntry {
        uint64_t fenceValue;
        ComPtr<ID3D12CommandAllocator> allocator;
    };

    ID3D12Device *m_device;
    D3D12_COMMAND_LIST_TYPE m_type;
    std::queue<AllocatorEntry> m_allocatorQueue;
    std::mutex m_mutex;
};

// Resource state tracker for managing barrier transitions
class ResourceStateTracker {
public:
    void TrackResource(ID3D12Resource *resource, D3D12_RESOURCE_STATES initialState);

    void TransitionResource(ID3D12GraphicsCommandList *cmdList, ID3D12Resource *resource,
                            D3D12_RESOURCE_STATES newState,
                            uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    void FlushBarriers(ID3D12GraphicsCommandList *cmdList);

    void Reset();

private:
    struct ResourceState {
        D3D12_RESOURCE_STATES state;
        uint32_t subresource;
    };

    std::unordered_map<ID3D12Resource *, ResourceState> m_resourceStates;
    std::vector<D3D12_RESOURCE_BARRIER> m_pendingBarriers;
};

// Upload buffer manager for staging data
class UploadBufferAllocator {
public:
    UploadBufferAllocator(ID3D12Device *device, size_t pageSize = 2 * 1024 * 1024); // 2MB pages

    struct Allocation {
        void *cpuAddress;
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
        ID3D12Resource *resource;
        size_t offset;
    };

    Allocation Allocate(size_t size, size_t alignment = 256);

    void Reset();

private:
    struct UploadPage {
        ComPtr<ID3D12Resource> resource;
        void *cpuAddress;
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
        size_t offset;
        size_t pageSize;
    };

    ID3D12Device *m_device;
    size_t m_pageSize;
    std::vector<UploadPage> m_pages;
    size_t m_currentPage;
};

class D3D12Device : public Device {
public:
    explicit D3D12Device(const DeviceCreateInfo &info);

    ~D3D12Device() override;

    // Prevent copying
    D3D12Device(const D3D12Device &) = delete;

    D3D12Device &operator=(const D3D12Device &) = delete;

    CommandList *CreateCommandList(QueueType queueType) override;

    Swapchain *CreateSwapchain(void *windowHandle, CommandQueue *queue, uint32_t width, uint32_t height) override;

    CommandQueue *CreateCommandQueue(const CommandQueueCreateInfo &createInfo) override;

    Buffer *CreateBuffer(const BufferCreateInfo &desc) override;

    Texture *CreateTexture(const TextureCreateInfo &desc) override;

    Pipeline *CreatePipeline(const PipelineCreateInfo &pipelineCreateInfo) override;

    void DestroyBuffer(Buffer *buffer) override;

    void DestroyTexture(Texture *texture) override;

    void DestroyPipeline(Pipeline *pipeline) override;

    void UploadBufferData(Buffer *buffer, const void *data, size_t size) override;

    void UploadTextureData(Texture *texture, const void *data, size_t size) override;

    bool SupportsRayTracing() const override;

    bool SupportsMeshShaders() const override;

    uint64_t GetVideoMemoryBudget() const override;

    void FlushUploads() override;

    void WaitIdle() override;

    // D3D12-specific access
    ID3D12Device *GetD3D12Device() const { return m_device.Get(); }

    // Helper methods for internal use
    DescriptorHeapAllocator *GetRTVHeap() { return m_rtvHeap.get(); }
    DescriptorHeapAllocator *GetDSVHeap() { return m_dsvHeap.get(); }
    DescriptorHeapAllocator *GetCBVSRVUAVHeap() { return m_cbvSrvUavHeap.get(); }
    DescriptorHeapAllocator *GetSamplerHeap() { return m_samplerHeap.get(); }

    UploadBufferAllocator *GetUploadAllocator() { return m_uploadAllocator.get(); }

    BindlessDescriptorManager *GetBindlessManager() const override { return m_bindlessManager.get(); }

private:
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGIAdapter4> m_adapter;
    ComPtr<ID3D12RootSignature> m_graphicsRootSignature;
    ComPtr<ID3D12RootSignature> m_computeRootSignature;
    std::unique_ptr<D3D12CommandQueue> m_uploadCommandQueue;
    std::unique_ptr<D3D12CommandList> m_uploadCommandList;

    struct PendingUpload {
        uint64_t fenceValue;
        ComPtr<ID3D12CommandAllocator> allocator;
    };

    std::queue<PendingUpload> m_pendingUploads;

    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue = 0;
    HANDLE m_fenceEvent = nullptr;

    std::unique_ptr<D3D12BindlessDescriptorManager> m_bindlessManager;
    ComPtr<ID3D12RootSignature> m_bindlessRootSignature;

    std::unique_ptr<DescriptorHeapAllocator> m_rtvHeap;
    std::unique_ptr<DescriptorHeapAllocator> m_dsvHeap;
    std::unique_ptr<DescriptorHeapAllocator> m_cbvSrvUavHeap;
    std::unique_ptr<DescriptorHeapAllocator> m_samplerHeap;

    std::unique_ptr<CommandAllocatorPool> m_directAllocatorPool;
    std::unique_ptr<CommandAllocatorPool> m_computeAllocatorPool;
    std::unique_ptr<CommandAllocatorPool> m_copyAllocatorPool;

    std::unique_ptr<UploadBufferAllocator> m_uploadAllocator;

    ResourceStateTracker m_stateTracker;

    // Helper methods
    static ComPtr<ID3DBlob> GetShaderBlob(const Shader &shader);

    void InitializeSynchronization();

    void InitializeDescriptorHeaps();

    void InitializeCommandAllocatorPools();

    void WaitForFenceValue(UINT64 value);

    ComPtr<ID3D12RootSignature> CreateBindlessRootSignature();

    ComPtr<ID3D12RootSignature> CreateDefaultGraphicsRootSignature();

    ComPtr<ID3D12RootSignature> CreateDefaultComputeRootSignature();

    ComPtr<ID3D12RootSignature> CreateFallbackRootSignature();

    void CheckBindlessSupport();

    static std::string ShaderTargetToString(ShaderStage stage);

    static DXGI_FORMAT TextureFormatToDxgiFormat(TextureFormat format);

    static D3D12_RESOURCE_STATES BufferUsageToResourceState(BufferUsage usage);

    static D3D12_RESOURCE_STATES TextureUsageToResourceState(TextureUsage usage);

    static D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType(QueueType type);

    static const wchar_t *GetQueueTypeName(QueueType type);
};

#endif //GPU_PARTICLE_SIM_D3D12DEVICE_H
