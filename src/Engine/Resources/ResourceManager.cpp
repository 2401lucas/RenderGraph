//
// Created by 2401Lucas on 2025-10-30.
//

#include "ResourceManager.h"

#include "TextureLoader.h"

ResourceManager::ResourceManager(Device *device) : m_device(device), m_gpuMemorySize(0), m_gpuMemoryUsed(0) {
    m_gpuMemorySize = device->GetVideoMemoryBudget();

    TextureData textureData = TextureLoader::CreateCheckerboard(512, 512);
    TextureCreateInfo textureCI{
        .width = textureData.width,
        .height = textureData.height,
        .depth = textureData.depth,
        .mipLevels = textureData.mipLevels,
        .arraySize = 1,
        .format = textureData.format,
    };
    std::unique_ptr<Texture> texture(device->CreateTexture(textureCI));
    device->UploadTextureData(texture.get(), textureData.data.data(),
                              textureData.data.size() * sizeof(uint8_t));
    m_texturePool.Add("default", std::move(texture));
}

ResourceManager::~ResourceManager() {
    m_meshPool.Clear();
}

ResourcePoolState ResourceManager::GetResourceState(MeshHandle handle) {
    return m_meshPool.GetState(handle);
}

ResourcePoolState ResourceManager::GetResourceState(TextureHandle handle) {
    return m_texturePool.GetState(handle);
}

bool ResourceManager::IsLoaded(MeshHandle handle) {
    return GetResourceState(handle) == ResourcePoolState::Loaded;
}

bool ResourceManager::IsLoaded(TextureHandle handle) {
    return GetResourceState(handle) == ResourcePoolState::Loaded;
}

MeshHandle ResourceManager::LoadMesh(const std::string &path) {
    MeshHandle existingHandle = m_meshPool.FindByPath(path);

    if (existingHandle.IsValid()) {
        m_meshPool.AddRef(existingHandle);
        return existingHandle;
    }

    try {
        MeshData meshData = MeshLoader::LoadFromFile(path);
        auto mesh = std::make_unique<Mesh>(m_device, meshData);
        MeshHandle handle = m_meshPool.Add(path, std::move(mesh));
        return handle;
    } catch (const std::exception &e) {
        printf("Failed to load mesh: ");
        printf(path.c_str());
        printf(" - ");
        printf(e.what());
        printf("\n");
        return MeshHandle{};
    }
}

TextureHandle ResourceManager::LoadTexture(const std::string &path) {
    TextureHandle existingHandle = m_texturePool.FindByPath(path);

    if (existingHandle.IsValid()) {
        m_texturePool.AddRef(existingHandle);
        return existingHandle;
    }

    try {
        TextureData textureData = TextureLoader::LoadFromFile(path);
        TextureCreateInfo textureCI{
            .width = textureData.width,
            .height = textureData.height,
            .depth = textureData.depth,
            .mipLevels = textureData.mipLevels,
            .arraySize = 1,
            .format = textureData.format,
        };
        std::unique_ptr<Texture> texture(m_device->CreateTexture(textureCI));
        m_device->UploadTextureData(texture.get(), textureData.data.data(),
                                    textureData.data.size() * sizeof(uint8_t));
        TextureHandle handle = m_texturePool.Add(path, std::move(texture));
        return handle;
    } catch (const std::exception &e) {
        printf("Failed to load texture: ");
        printf(path.c_str());
        printf(" - ");
        printf(e.what());
        printf("\n");
        return TextureHandle{};
    }
}

MaterialHandle ResourceManager::LoadMaterial(const std::string &path) {
    MaterialHandle existingHandle = m_materialPool.FindByPath(path);

    if (existingHandle.IsValid()) {
        m_materialPool.AddRef(existingHandle);
        return existingHandle;
    }

    return MaterialHandle{};
}

ShaderHandle ResourceManager::LoadShader(const std::string &path) {
    ShaderHandle existingHandle = m_shaderPool.FindByPath(path);

    if (existingHandle.IsValid()) {
        m_shaderPool.AddRef(existingHandle);
        return existingHandle;
    }

    try {
        std::unique_ptr<Shader> shader(m_device->CreateShader(path));
        ShaderHandle handle = m_shaderPool.Add(path, std::move(shader));
        return handle;
    } catch (const std::exception &e) {
        printf("Failed to load shader: ");
        printf(path.c_str());
        printf(" - ");
        printf(e.what());
        printf("\n");
        return ShaderHandle{};
    }
}

void ResourceManager::UnloadMesh(MeshHandle handle) {
    m_meshPool.Remove(handle);
}

void ResourceManager::UnloadTexture(TextureHandle handle) {
    m_texturePool.Remove(handle);
}

void ResourceManager::UnloadMaterial(MaterialHandle handle) {
    m_materialPool.Remove(handle);
}

void ResourceManager::UnloadShader(ShaderHandle handle) {
    m_shaderPool.Remove(handle);
}

void ResourceManager::UnloadAllTextures() {
    m_texturePool.Clear();
}

void ResourceManager::UnloadAllMeshes() {
    m_meshPool.Clear();
}

Mesh *ResourceManager::GetMesh(MeshHandle handle) {
    return m_meshPool.Get(handle);
}

Texture *ResourceManager::GetTexture(TextureHandle handle) {
    return m_texturePool.Get(handle);
}

Material *ResourceManager::GetMaterial(MaterialHandle handle) {
    return m_materialPool.Get(handle);
}

Shader *ResourceManager::GetShader(ShaderHandle handle) {
    return m_shaderPool.Get(handle);
}

void ResourceManager::ReloadShader(ShaderHandle handle) {
}

void ResourceManager::ReloadTexture(TextureHandle handle) {
}

void ResourceManager::EnableHotReload(bool enable) {
}

void ResourceManager::AddRef(MeshHandle handle) {
    m_meshPool.AddRef(handle);
}

void ResourceManager::Release(MeshHandle handle) {
    if (m_meshPool.Release(handle)) {
        // Ref count reached 0, unload
        UnloadMesh(handle);
    }
}

uint64_t ResourceManager::GetTotalGPUMemoryUsed() const {
    return m_gpuMemoryUsed;
}

uint64_t ResourceManager::GetGPUMemoryBudget() const {
    return m_gpuMemorySize;
}

void ResourceManager::TrimMemory() {
}

void ResourceManager::Update() {
}
