//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_RESOURCEMANAGER_H
#define GPU_PARTICLE_SIM_RESOURCEMANAGER_H
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <queue>
#include <thread>
#include <unordered_map>

#include "Material.h"
#include "Mesh.h"
#include "ResourceHandle.h"
#include "Rendering/RHI/Shader.h"
#include "Rendering/RHI/Texture.h"
#include "Rendering/RHI/Device.h"

enum ResourcePoolState {
    Unloaded, // Resource not loaded
    Loading, // Currently loading (async)
    Loaded, // Fully loaded and ready to use
    Failed // Loading failed
};

template<typename ResourceType, typename HandleType>
class ResourcePool {
public:
    struct ResourceEntry {
        std::string path;
        std::unique_ptr<ResourceType> resource;
        ResourcePoolState state = ResourcePoolState::Unloaded;
        uint32_t refCount = 0;
        uint32_t generation = 0;
        std::chrono::steady_clock::time_point lastAccessTime;
        uint64_t memorySize = 0;
    };

    HandleType Add(const std::string &path, std::unique_ptr<ResourceType> resource) {
        std::lock_guard<std::mutex> lock(m_mutex);

        HandleType handle{
            .id = m_nextId++,
            .generation = 0
        };

        ResourceEntry entry{
            .path = path,
            .resource = std::move(resource),
            .state = ResourcePoolState::Loaded,
            .refCount = 1,
            .generation = 0,
            .lastAccessTime = std::chrono::steady_clock::now()
        };

        m_resources[handle.id] = std::move(entry);
        m_pathToId[path] = handle.id;

        return handle;
    }

    HandleType CreatePlaceholder(const std::string &path) {
        std::lock_guard<std::mutex> lock(m_mutex);

        HandleType handle{
            .id = m_nextId++,
            .generation = 0
        };

        ResourceEntry entry{
            .path = path,
            .resource = nullptr,
            .state = ResourcePoolState::Loading,
            .refCount = 1,
            .generation = 0,
            .lastAccessTime = std::chrono::steady_clock::now()
        };

        m_resources[handle.id] = std::move(entry);
        m_pathToId[path] = handle.id;

        return handle;
    }

    void UpdatePlaceholder(HandleType handle, std::unique_ptr<ResourceType> resource) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(handle.id);
        if (it != m_resources.end() && it->second.generation == handle.generation) {
            it->second.resource = std::move(resource);
            it->second.state = ResourcePoolState::Loaded;
            it->second.lastAccessTime = std::chrono::steady_clock::now();
        }
    }

    ResourceType *Get(HandleType handle) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(handle.id);
        if (it == m_resources.end() && it->second.generation != handle.generation && it->second.state !=
            ResourcePoolState::Loaded) { return nullptr; }

        it->second.lastAccessTime = std::chrono::steady_clock::now();
        return it->second.resource.get();
    }

    ResourcePoolState GetState(HandleType handle) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(handle.id);
        if (it == m_resources.end()) return ResourcePoolState::Unloaded;
        if (it->second.generation != handle.generation) return ResourcePoolState::Unloaded;

        return it->second.state;
    }

    void Remove(HandleType handle) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(handle.id);
        if (it == m_resources.end()) { return; }
        m_pathToId.erase(it->second.path);
        m_resources.erase(it);
    }

    HandleType FindByPath(const std::string &path) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_pathToId.find(path);
        if (it != m_pathToId.end()) {
            auto resIt = m_resources.find(it->second);
            if (resIt != m_resources.end()) {
                HandleType handle{
                    .id = it->second,
                    .generation = resIt->second.generation
                };
            }
        }
        return HandleType{};
    }

    std::string GetPath(HandleType handle) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(handle.id);
        if (it != m_resources.end() && it->second.generation == handle.generation) {
            return it->second.path;
        }
        return "";
    }

    void AddRef(HandleType handle) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(handle.id);
        if (it != m_resources.end() && it->second.generation == handle.generation) {
            it->second.refCount++;
        }
    }

    bool Release(HandleType handle) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(handle.id);
        if (it != m_resources.end() && it->second.generation == handle.generation) {
            if (it->second.refCount > 0) {
                it->second.refCount--;
                return it->second.refCount == 0;
            }
        }
        return false;
    }

    uint64_t GetTotalMemory() const {
        std::lock_guard<std::mutex> lock(m_mutex);

        uint64_t total = 0;
        for (const auto &[id, entry]: m_resources) {
            if (entry.state == ResourcePoolState::Loaded) {
                total += entry.memorySize;
            }
        }
        return total;
    }

    std::vector<HandleType> GetLRUResources(uint64_t targetMemory) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Collect all loaded resources with timestamps
        std::vector<std::pair<std::chrono::steady_clock::time_point, HandleType> > candidates;
        for (const auto &[id, entry]: m_resources) {
            if (entry.state == ResourcePoolState::Loaded && entry.refCount == 0) {
                HandleType handle;
                handle.id = id;
                handle.generation = entry.generation;
                candidates.push_back({entry.lastAccessTime, handle});
            }
        }

        // Sort by access time (oldest first)
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto &a, const auto &b) {
                      return a.first < b.first;
                  });

        // Collect handles until we've freed enough memory
        std::vector<HandleType> result;
        uint64_t freedMemory = 0;
        for (const auto &[time, handle]: candidates) {
            auto it = m_resources.find(handle.id);
            if (it != m_resources.end()) {
                result.push_back(handle);
                freedMemory += it->second.memorySize;

                if (freedMemory >= targetMemory) {
                    break;
                }
            }
        }

        return result;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_resources.clear();
        m_pathToId.clear();
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<uint64_t, ResourceEntry> m_resources;
    std::unordered_map<std::string, uint64_t> m_pathToId;
    uint64_t m_nextId = 1;
};

//TODO Asset Streaming
class ResourceManager {
public:
    explicit ResourceManager(Device *device);

    ~ResourceManager();

    ResourceManager(const ResourceManager &) = delete;

    ResourceManager &operator=(const ResourceManager &) = delete;


    // Resource State Queries
    ResourcePoolState GetResourceState(MeshHandle);

    ResourcePoolState GetResourceState(TextureHandle);

    bool IsLoaded(MeshHandle);

    bool IsLoaded(TextureHandle);

    // Resource Creation
    MeshHandle LoadMesh(const std::string &path);

    TextureHandle LoadTexture(const std::string &path);

    MaterialHandle LoadMaterial(const std::string &path);

    ShaderHandle LoadShader(const std::string &path);

    // Resource Deletion
    void UnloadMesh(MeshHandle);

    void UnloadTexture(TextureHandle);

    void UnloadMaterial(MaterialHandle);

    void UnloadShader(ShaderHandle);


    void UnloadAllMeshes();

    void UnloadAllTextures();

    // Resource Access
    Mesh *GetMesh(MeshHandle);

    Texture *GetTexture(TextureHandle);

    Material *GetMaterial(MaterialHandle);

    Shader *GetShader(ShaderHandle);

    // Hot Reloads
    void ReloadShader(ShaderHandle);

    void ReloadTexture(TextureHandle);

    void EnableHotReload(bool enable);


    // Reference Counting
    void AddRef(MeshHandle);

    void Release(MeshHandle);

    // Memory Management
    uint64_t GetTotalGPUMemoryUsed() const;

    uint64_t GetGPUMemoryBudget() const;

    void TrimMemory();

    void Update();

private:
    Device *m_device;

    // Resource Pools
    ResourcePool<Mesh, MeshHandle> m_meshPool;
    ResourcePool<Texture, TextureHandle> m_texturePool;
    ResourcePool<Material, MaterialHandle> m_materialPool;
    ResourcePool<Shader, ShaderHandle> m_shaderPool;

    // Async Loading
    std::queue<std::function<void()> > loadingQueue;
    std::queue<std::function<void()> > uploadQueue;
    std::mutex queueMutex;
    std::thread loadingThread;
    bool stopLoading = false;

    // Hot reloading
    bool hotReloadEnabled = false;
    std::unordered_map<std::string, std::filesystem::file_time_type> fileTimestamps;

    // Memory
    uint64_t m_gpuMemorySize = 0; // TODO: Get from RendererDevice
    uint64_t m_gpuMemoryUsed = 0;
};


#endif //GPU_PARTICLE_SIM_RESOURCEMANAGER_H
