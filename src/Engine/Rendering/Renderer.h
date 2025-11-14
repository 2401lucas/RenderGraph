//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_RENDERER_H
#define GPU_PARTICLE_SIM_RENDERER_H

#include "../Resources/ResourceHandle.h"
#include <memory>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Resources/ResourceManager.h"

#include "Core/Transform.h"
#include "Core/Camera.h"
#include "RenderGraph/RenderGraph.h"
#include "OS/Window/Window.h"
#include "RHI/Device.h"
#include "RHI/Buffer.h"
#include "RHI/CommandList.h"
#include "RHI/CommandQueue.h"
#include "RHI/Pipeline.h"

struct alignas(256) PerFrameData {
    glm::mat4 viewProjection;
    glm::vec3 cameraPosition;
    float time;
    glm::vec3 lightDirection;
    float lightIntensity;
    glm::vec3 lightColor;
    uint32_t frameIndex;

    // Padding to 256-byte alignment
    uint32_t padding[36];
};

struct alignas(256) PerObjectData {
    glm::mat4 worldMatrix;
    glm::mat4 normalMatrix; // For correct normal transformation
    uint32_t albedoTextureIndex;
    uint32_t normalTextureIndex;
    uint32_t metallicRoughnessIndex;
    uint32_t emissiveTextureIndex;
    glm::vec4 albedoFactor;
    float metallicFactor;
    float roughnessFactor;
    uint32_t materialFlags;
    uint32_t objectID;

    // Padding to 256-byte alignment
    uint32_t padding[20];
};

/// <summary>
/// Information submitted by the application for rendering
/// </summary>
struct RenderInfo {
    MeshHandle mesh;
    MaterialHandle material;
    Transform transform;

    // Rendering flags
    bool castsShadows = true;
    bool receivesShadows = true;
    bool isTransparent = false;

    // For sorting
    float distanceToCamera = 0.0f;
    uint32_t sortKey = 0;
};

/// <summary>
/// Batched render command for instanced rendering
/// </summary>
struct RenderBatch {
    Mesh *mesh = nullptr;
    Material *material = nullptr;
    std::vector<Transform> transforms;
    bool castsShadows = true;
};

class Renderer {
public:
    explicit Renderer(Window *window, Device *device, ResourceManager *);

    ~Renderer();

    void Update(float deltaTime);

    Device *GetDevice() { return m_device; }

    void BeginFrame();

    void EndFrame();

    // Submission API (Called by Applications)

    /// <summary>
    /// Submit an object for rendering this frame
    /// </summary>
    void Submit(const RenderInfo &info);

    /// <summary>
    /// Submit multiple objects at once
    /// </summary>
    void Submit(const std::vector<RenderInfo> &infos);

    /// <summary>
    /// Set the active camera for this frame
    /// </summary>
    void SetCamera(Camera &camera);

    /// <summary>
    /// Set directional light
    /// </summary>
    void SetDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity);

    void EnableShadows(bool enable) { m_shadowsEnabled = enable; }

    void SetShadowMapSize(uint32_t size) { m_shadowMapSize = size; }

    void EnablePostProcessing(bool enable) { m_postProcessingEnabled = enable; }

    struct Statistics {
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t instancedDrawCalls = 0;
        uint32_t instanceCount = 0;
        float cpuFrameTime = 0.0f;
        float gpuFrameTime = 0.0f;
    };

    const Statistics &GetStatistics() const { return m_statistics; }

    void Resize();
private:
    struct FrameResources {
        uint64_t fenceValue = 0;
        // Per-frame constant buffers for multi-frame buffering
        std::unique_ptr<Buffer> perFrameBuffer;
        std::unique_ptr<Buffer> perObjectBuffer;
    };

    FrameResources m_frameResources[FrameCount];
    uint64_t m_currentFenceValue = 0;

    // Core Resources
    Window *m_window;
    ResourceManager *m_resourceManager;
    Device *m_device;
    std::unique_ptr<RenderGraph> m_renderGraph;
    std::unique_ptr<Swapchain> m_swapchain;

    TextureHandle m_defaultTexture;
    TextureHandle m_defaultNormalMap;
    TextureHandle m_defaultMetallicRoughness;

    // Command infrastructure
    std::unique_ptr<CommandQueue> m_graphicsQueue = nullptr;
    std::unique_ptr<CommandQueue> m_computeQueue = nullptr;
    std::unique_ptr<CommandQueue> m_transferQueue = nullptr;

    // Pipelines
    std::unique_ptr<Pipeline> m_mainPipeline;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_frameIndex = 0;
    uint32_t m_objectIDCounter = 0;

    // Submission Data
    std::vector<RenderInfo> m_submissions;
    std::vector<RenderBatch> m_batches;

    // Scene data
    Camera *m_camera;

    struct DirectionalLight {
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
    } m_directionalLight;

    // Configuration
    bool m_shadowsEnabled = false;
    uint32_t m_shadowMapSize = 2048;
    bool m_postProcessingEnabled = false;

    // Internal Resouces
    bool m_isFrameStarted = false;
    Statistics m_statistics;
    float m_deltaTime = 0.0f;
    float m_totalTime = 0.0f;

    // Frame resource management
    void CreateFrameResources();
    FrameResources& GetCurrentFrameResources() { return m_frameResources[m_frameIndex]; }

    // Submission processing
    void ProcessSubmissions();
    void SortSubmissions();
    void BatchSubmissions();
    void CalculateSortKeys();

    // RenderGraph setup
    void BuildRenderGraph();

    // TODO: Move
    // Rendering functions (passed to RenderGraph)
    void RenderShadows(RenderPassContext &ctx);
    void RenderMain(RenderPassContext &ctx);
    void RenderParticles(RenderPassContext &ctx);
    void RenderPostProcess(RenderPassContext &ctx);
    void RenderUI(RenderPassContext &ctx);

    // Helpers
    void UpdatePerFrameData();
    void UpdatePerObjectData(Transform& transform, Material* material, uint32_t objectID);
    void WaitForGPU();
    void UpdateStatistics();
};

#endif //GPU_PARTICLE_SIM_RENDERER_H