//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_TEXTURELOADER_H
#define GPU_PARTICLE_SIM_TEXTURELOADER_H

#pragma once
#include <vector>
#include <string>
#include <cstdint>

#include "Rendering/RHI/Texture.h"

struct TextureData {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    TextureFormat format = TextureFormat::Undefined;
    std::vector<uint8_t> data;
    std::string name;
};

class TextureLoader {
public:
    // Load texture from file (auto-detects format)
    static TextureData LoadFromFile(const std::string& path);

    // Format-specific loaders
    static TextureData LoadPNG(const std::string& path);
    static TextureData LoadJPG(const std::string& path);
    static TextureData LoadTGA(const std::string& path);
    static TextureData LoadDDS(const std::string& path);
    static TextureData LoadHDR(const std::string& path);

    // Generate mipmaps
    static void GenerateMipmaps(TextureData& texture);

    // Create solid color texture
    static TextureData CreateSolidColor(uint32_t width, uint32_t height,
                                        uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    // Create checkerboard pattern
    static TextureData CreateCheckerboard(uint32_t width, uint32_t height,
                                          uint32_t checkerSize = 8);
};
#endif //GPU_PARTICLE_SIM_TEXTURELOADER_H