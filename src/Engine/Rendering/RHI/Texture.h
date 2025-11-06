//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_TEXTURE_H
#define GPU_PARTICLE_SIM_TEXTURE_H

#include <cstdint>

enum class TextureUsage {
    ShaderResource,
    RenderTarget,
    DepthStencil,
    UnorderedAccess,
    Present,
    CopySource,
    CopyDest,
};

enum class TextureFormat {
    Undefined,
    RGBA32_FLOAT,
    RGB32_FLOAT,
    RG32_FLOAT,
    R32_FLOAT,
    RGBA16_FLOAT,
    RG16_FLOAT,
    R16_FLOAT,
    RGBA8_UNORM,
    Depth32,
    Depth24Stencil8,
    // TODO: add more formats
};

struct TextureCreateInfo {
    uint32_t width;
    uint32_t height;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arraySize = 1;
    TextureFormat format;
    TextureUsage usage;
    const char *debugName = nullptr;
};

class Texture {
public:
    virtual ~Texture() = default;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    TextureFormat format = TextureFormat::Undefined;
    TextureUsage usage;
    uint64_t size = 0;
};

#endif //GPU_PARTICLE_SIM_TEXTURE_H
