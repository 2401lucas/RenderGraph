//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_TEXTURE_H
#define GPU_PARTICLE_SIM_TEXTURE_H

enum class TextureFormat {
    Undefined,
    RGBA8_UNORM,
    RGBA16_FLOAT,
    RGBA32_FLOAT,
    Depth32,
    // TODO: add more formats
};

struct TextureCreateInfo {
    uint32_t width;
    uint32_t height;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arraySize = 1;
    TextureFormat format;
    const char *debugName = nullptr;
};

class Texture {
public:
    Texture();
    virtual ~Texture() = default;

    // Properties
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    uint32_t GetMipLevels() const { return m_mipLevels; }
    TextureFormat GetFormat() const { return m_format; }

    virtual void *GetNativeHandle() const = 0;

    // GPU resource
    void *GetGPUImage() const { return m_gpuResourceImage; }
    void *GetGPUImageView() const { return m_gpuResourceView; }
    void *GetGPUImageSampler() const { return m_gpuResourceSampler; }
    // Memory usage
    uint64_t GetGPUMemorySize() const { return m_gpuMemorySize; }

private:
    void *m_gpuResourceImage = nullptr; // ID3D12Resource* or VkImage
    void *m_gpuResourceView = nullptr; //  or VkImageView
    void *m_gpuResourceSampler = nullptr; //  or VkImageSampler
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_mipLevels = 1;
    TextureFormat m_format = TextureFormat::Undefined;
    uint64_t m_gpuMemorySize = 0;
};

#endif //GPU_PARTICLE_SIM_TEXTURE_H
