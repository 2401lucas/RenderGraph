//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_MATERIAL_H
#define GPU_PARTICLE_SIM_MATERIAL_H


#include "ResourceHandle.h"
#include <string>

struct MaterialProperties {
    float baseColor[4] = {1, 1, 1, 1};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float emissive[3] = {0, 0, 0};
    float alphaCutoff = 0.5f;
};

class Material {
public:
    Material();

    ~Material();

    // Textures
    void SetAlbedoTexture(TextureHandle texture) { m_albedoTexture = texture; }
    void SetNormalTexture(TextureHandle texture) { m_normalTexture = texture; }
    void SetMetallicRoughnessTexture(TextureHandle texture) { m_metallicRoughnessTexture = texture; }
    void SetEmissiveTexture(TextureHandle texture) { m_emissiveTexture = texture; }

    TextureHandle GetAlbedoTexture() const { return m_albedoTexture; }
    TextureHandle GetNormalTexture() const { return m_normalTexture; }
    TextureHandle GetMetallicRoughnessTexture() const { return m_metallicRoughnessTexture; }
    TextureHandle GetEmissiveTexture() const { return m_emissiveTexture; }

    // Properties
    MaterialProperties &GetProperties() { return m_properties; }
    const MaterialProperties &GetProperties() const { return m_properties; }

    // TODO: GET SHADER/PIPELINE INFO
    // Shader
    // void SetShader(ShaderHandle shader) { m_shader = shader; }
    // ShaderHandle GetShader() const { return m_shader; }

private:
    TextureHandle m_albedoTexture;
    TextureHandle m_normalTexture;
    TextureHandle m_metallicRoughnessTexture;
    TextureHandle m_emissiveTexture;
    // ShaderHandle m_shader;
    MaterialProperties m_properties;
};


#endif //GPU_PARTICLE_SIM_MATERIAL_H
