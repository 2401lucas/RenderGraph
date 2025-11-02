//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_SWAPCHAIN_H
#define GPU_PARTICLE_SIM_SWAPCHAIN_H

#include <Resources/TextureLoader.h>

#include "Texture.h"

enum class SwapchainPresentResult {
    Success,
    Error
};

class Swapchain {
public:
    Swapchain();

    virtual ~Swapchain();

    virtual SwapchainPresentResult Present() = 0;

    virtual const TextureFormat GetColorFormat() const = 0;

    virtual const uint32_t GetImageCount() const = 0;

    virtual Texture* GetSwapchainBuffer(uint32_t frameIndex) const = 0;
};

#endif //GPU_PARTICLE_SIM_SWAPCHAIN_H
