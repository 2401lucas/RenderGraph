//
// Created by 2401Lucas on 2025-11-04.
//

#ifndef GPU_PARTICLE_SIM_D3D12SWAPCHAIN_H
#define GPU_PARTICLE_SIM_D3D12SWAPCHAIN_H
#include "Rendering/RHI/Swapchain.h"

#include "D3D12Common.h"

#include "D3D12Texture.h"


class D3D12Swapchain : public Swapchain {
public:
    SwapchainPresentResult Present(bool vsync) override;

    void Resize(uint32_t width, uint32_t height) override;

    const TextureFormat GetColorFormat() const override;

    const uint32_t GetImageCount() const override;

    Texture *GetSwapchainBuffer(uint32_t frameIndex) const override;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;

    uint32_t m_frameIndex = 0;
    uint32_t m_rtvDescriptorSize;
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGISwapChain4> m_swapchain;
    ComPtr<ID3D12Resource> m_backBuffers[2];
    std::unique_ptr<D3D12Texture> m_backBufferTextureWrappers[2];
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
};


#endif //GPU_PARTICLE_SIM_D3D12SWAPCHAIN_H
