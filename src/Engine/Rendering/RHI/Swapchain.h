//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_SWAPCHAIN_H
#define GPU_PARTICLE_SIM_SWAPCHAIN_H

#include <Resources/TextureLoader.h>

#include "Texture.h"

static constexpr uint32_t FrameCount = 2;

enum class SwapchainPresentResult {
    Success,
    Error
};

class Swapchain {
public:
    /// <summary>
    /// Sends the current Swapchain buffer to the GPU and gets the next swapchain buffer index
    /// </summary>
    virtual SwapchainPresentResult Present(bool vsync) = 0;

    virtual void Resize(uint32_t width, uint32_t height) = 0;

    virtual const TextureFormat GetColorFormat() const = 0;

    virtual const uint32_t GetImageCount() const = 0;

    virtual Texture *GetSwapchainBuffer(uint32_t frameIndex) const = 0;
};

#endif //GPU_PARTICLE_SIM_SWAPCHAIN_H
