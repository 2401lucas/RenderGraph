//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_RENDERER_H
#define GPU_PARTICLE_SIM_RENDERER_H

#include "../Resources/ResourceHandle.h"
#include <memory>
#include <vector>
#include <cstdint>
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

/// <summary>
/// Information submitted by the application for rendering
/// </summary>
struct RenderInfo {
    Mesh *mesh = nullptr;
    Material *material = nullptr;
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

/// <summary>
/// Main renderer class.
/// Manages rendering pipeline, collects submissions from application,
/// optimizes and batches, then executes via RenderGraph.
/// </summary>
class Renderer {
public:
    explicit Renderer(Window *window, Device *device, ResourceManager *);

    ~Renderer();

    void Update(float deltaTime);

    // ===== Device Access =====
    Device *GetDevice() { return m_device; }

    // ===== Frame Management =====

    /// <summary>
    /// Begin a new frame. Clears submission queue.
    /// </summary>
    void BeginFrame();

    /// <summary>
    /// End frame. Processes submissions, executes RenderGraph, presents.
    /// </summary>
    void EndFrame();

    // ===== Submission API (Called by Application) =====

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
    /// Set directional light (sun)
    /// </summary>
    void SetDirectionalLight(const float direction[3], const float color[3], float intensity);

    // ===== Configuration =====

    /// <summary>
    /// Enable or disable shadow rendering
    /// </summary>
    void EnableShadows(bool enable) { m_shadowsEnabled = enable; }

    /// <summary>
    /// Set shadow map resolution
    /// </summary>
    void SetShadowMapSize(uint32_t size) { m_shadowMapSize = size; }

    /// <summary>
    /// Enable or disable post-processing
    /// </summary>
    void EnablePostProcessing(bool enable) { m_postProcessingEnabled = enable; }

    /// <summary>
    /// Check if window should close
    /// </summary>
    bool ShouldClose() const;

    // ===== Statistics =====

    struct Statistics {
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t instancedDrawCalls = 0;
        uint32_t instanceCount = 0;
        float cpuFrameTime = 0.0f;
        float gpuFrameTime = 0.0f;
    };

    const Statistics &GetStatistics() const { return m_statistics; }

private:
    // ===== Syncronization =====
    struct FrameResources {
        uint64_t fenceValue = 0;
    };

    FrameResources m_frameResources[FrameCount];
    uint64_t m_currentFenceValue = 0;

    // ===== Core Resources =====
    Window *m_window;
    ResourceManager *m_resourceManager;
    Device *m_device;
    std::unique_ptr<RenderGraph> m_renderGraph;
    std::unique_ptr<Swapchain> m_swapchain;

    // Command infrastructure
    std::unique_ptr<CommandQueue> m_graphicsQueue = nullptr;
    std::unique_ptr<CommandQueue> m_computeQueue = nullptr;
    std::unique_ptr<CommandQueue> m_transferQueue = nullptr;

    // TODO: MANAGE PROPERLY
    std::unique_ptr<Pipeline> m_mainPipeline;
    std::unique_ptr<Buffer> m_perObjData;

    uint32_t m_frameIndex = 0;

    // ===== Submission Data =====

    struct PerObjData {
        float cameraPV[16];
        float cameraPerspective[16];
        float cameraView[16];
        float model[16];
    };


    std::vector<RenderInfo> m_submissions;
    std::vector<RenderBatch> m_batches;

    // Scene data
    Camera *m_camera;

    struct {
        float direction[3];
        float color[3];
        float intensity;
    } m_directionalLight;

    // ===== Configuration =====

    bool m_shadowsEnabled = false;
    uint32_t m_shadowMapSize = 2048;
    bool m_postProcessingEnabled = false;

    // ===== State =====

    bool m_isFrameStarted = false;
    Statistics m_statistics;
    float m_deltaTime = 0.0f;

    // ===== Internal Methods =====

    // Submission processing
    void ProcessSubmissions();

    void SortSubmissions();

    void BatchSubmissions();

    void CalculateSortKeys();

    // RenderGraph setup
    void BuildRenderGraph();

    void ExecuteRenderGraph();

    // TODO: make own class
    // Rendering functions (passed to RenderGraph)
    void RenderShadows(RenderPassContext &ctx);

    void RenderMain(RenderPassContext &ctx);

    void RenderParticles(RenderPassContext &ctx);

    void RenderPostProcess(RenderPassContext &ctx);

    void RenderUI(RenderPassContext &ctx);

    // Helpers
    void WaitForGPU();

    void UpdateStatistics();
};

#endif //GPU_PARTICLE_SIM_RENDERER_H
