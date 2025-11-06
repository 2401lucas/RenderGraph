//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_DEVICE_H
#define GPU_PARTICLE_SIM_DEVICE_H

#include <memory>
#include <cstdint>

#include "Swapchain.h"
#include "CommandQueue.h"

class Shader;

struct DeviceCreateInfo {
    bool enableDebugLayer = false;
    bool enableGPUValidation = false;
    uint32_t preferredAdapterIndex = 0;
};

/// <summary>
/// Device represents the GPU and is the factory for all RHI objects.
/// This is the main entry point for the RHI.
/// </summary>
class Device {
public:
    static std::unique_ptr<Device> Create(const DeviceCreateInfo &desc);

    virtual ~Device() = default;

    // ===== Factory Methods =====

    virtual CommandQueue *CreateCommandQueue(const CommandQueueCreateInfo &createInfo) = 0;

    virtual CommandList *CreateCommandList(QueueType) = 0;

    virtual Swapchain *CreateSwapchain(void *windowHandle, CommandQueue *queue, uint32_t width, uint32_t height) = 0;

    virtual Buffer *CreateBuffer(const BufferCreateInfo &desc) = 0;

    virtual Texture *CreateTexture(const TextureCreateInfo &desc) = 0;

    virtual Pipeline *CreatePipeline(const PipelineCreateInfo &desc) = 0;

    // ===== Resource Management =====

    virtual void UploadBufferData(Buffer *buffer, const void *data, size_t size) = 0;

    virtual void UploadTextureData(Texture *texture, const void *data, size_t size) = 0;

    virtual void DestroyBuffer(Buffer *buffer) = 0;

    virtual void DestroyTexture(Texture *texture) = 0;

    virtual void DestroyPipeline(Pipeline *pipeline) = 0;

    // ===== Device Capabilities =====

    virtual bool SupportsRayTracing() const = 0;

    virtual bool SupportsMeshShaders() const = 0;

    virtual uint64_t GetVideoMemoryBudget() const = 0;

    // ===== Synchronization =====

    virtual void WaitIdle() = 0;
};


#endif //GPU_PARTICLE_SIM_DEVICE_H
