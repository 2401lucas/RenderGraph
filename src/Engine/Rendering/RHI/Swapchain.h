//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_SWAPCHAIN_H
#define GPU_PARTICLE_SIM_SWAPCHAIN_H

#include "Texture.h"

static constexpr uint32_t FrameCount = 2;

enum class SwapchainPresentResult {
    Success,
    Error,
    // TODO: Feature more descriptive errors (IE wrong size)
};

class Swapchain {
public:
    virtual ~Swapchain() = default;

    /// <summary>
    /// Sends the current Swapchain buffer to the GPU and gets the next swapchain buffer index
    /// </summary>
    virtual SwapchainPresentResult Present(bool vsync) = 0;

    /// <summary>
    /// Resizes the swapchain buffers
    /// </summary>
    virtual void Resize(uint32_t width, uint32_t height) = 0;

    virtual const TextureFormat GetColorFormat() const = 0;

    virtual const uint32_t GetImageCount() const = 0;

    /// <summary>
    /// Gets the Texture file wrapping the swapchain buffer
    /// </summary>
    virtual Texture *GetSwapchainBuffer(uint32_t frameIndex) const = 0;
};

#endif //GPU_PARTICLE_SIM_SWAPCHAIN_H
