//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_D3D12DEVICE_H
#define GPU_PARTICLE_SIM_D3D12DEVICE_H

#include "../Device.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12Device : public Device {
public:
    D3D12Device(const DeviceCreateInfo &);

    ~D3D12Device() override;

    CommandList *CreateCommandList() override;

    Swapchain *CreateSwapchain(void *windowHandle, uint32_t width, uint32_t height) override;

    void DestroyBuffer(Buffer *buffer) override;

    void DestroyTexture(Texture *texture) override;

    void DestroyShader(Shader *shader) override;

    void DestroyPipeline(Pipeline *pipeline) override;

    bool SupportsRayTracing() const override;

    bool SupportsMeshShaders() const override;

    uint64_t GetVideoMemoryBudget() const override;

    void WaitIdle() override;

    CommandQueue *CreateCommandQueue() override;

    Buffer *CreateBuffer(const BufferCreateInfo &desc) override;

    // ... implement all virtual methods
    Texture *CreateTexture(const TextureCreateInfo &desc) override;

    Shader *CreateShader(const std::string &path) override;

    Pipeline *CreatePipeline(const PipelineDesc &desc) override;

    void UploadBufferData(Buffer *buffer, const void *data, size_t size) override;

    void UploadTextureData(Texture *texture, const void *data, size_t size) override;

    // D3D12-specific access
    ID3D12Device *GetD3D12Device() const { return m_device.Get(); }

private:
    DeviceCreateInfo m_desc;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGIAdapter4> m_adapter;
};


#endif //GPU_PARTICLE_SIM_D3D12DEVICE_H
