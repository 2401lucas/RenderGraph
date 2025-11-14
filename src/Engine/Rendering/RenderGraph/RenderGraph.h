//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_RENDERGRAPH_H
#define GPU_PARTICLE_SIM_RENDERGRAPH_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>

#include "Rendering/RHI/Device.h"
#include "RenderPass.h"
#include "Rendering/RHI/Buffer.h"
#include "Rendering/RHI/Texture.h"
#include "Rendering/RHI/CommandList.h"

/// <summary>
/// RenderGraph manages the execution of render passes.
///
/// Responsibilities:
/// - Schedules passes in dependency order
/// - Allocates and manages transient resources (per-frame)
/// - Inserts resource barriers automatically
/// - Optimizes resource lifetimes
/// - Manages command list execution internally
/// </summary>
class RenderGraph {
public:
    explicit RenderGraph(Device *device, CommandQueue *commandQueue, uint32_t frameCount = 2);

    ~RenderGraph();

    /// <summary>
    /// Add a render pass to the graph.
    /// Passes are executed in the order determined by dependencies.
    /// </summary>
    void AddPass(std::unique_ptr<RenderPass> pass);

    /// <summary>
    /// Remove all passes from the graph.
    /// Called at the start of each frame setup.
    /// </summary>
    void Clear();

    /// <summary>
    /// Compile and execute the render graph for the current frame.
    /// This performs:
    /// 1. Dependency analysis
    /// 2. Topological sorting
    /// 3. Resource allocation (per-frame)
    /// 4. Barrier insertion
    /// 5. Pass execution
    ///
    /// Returns the command list that was recorded (for submission to queue)
    /// </summary>
    CommandList *Execute();

    /// <summary>
    /// Advance to next frame. Must be called after GPU has finished previous frame.
    /// </summary>
    void NextFrame();

    /// <summary>
    /// Wait for all in-flight frames to complete and destroy old resources
    /// Call before shutdown or when you know GPU is idle
    /// </summary>
    void Flush();

    /// <summary>
    /// Register an external texture with initial state.
    /// Example: swap chain back buffer
    /// </summary>
    void RegisterExternalTexture(const std::string &name, Texture *texture,
                                 TextureUsage initialState = TextureUsage::RenderTarget);

    /// <summary>
    /// Register an external buffer with initial state
    /// </summary>
    void RegisterExternalBuffer(const std::string &name, Buffer *buffer,
                                BufferUsage initialState = BufferUsage::Storage);

    /// <summary>
    /// Mark a texture as the present target (will be transitioned to Present state)
    /// </summary>
    void SetPresentTarget(const std::string &name);

    void SetAutoBarriers(bool enable) { m_autoBarriers = enable; }
    void SetResourceAliasing(bool enable) { m_resourceAliasing = enable; }

    struct Statistics {
        uint32_t passCount = 0;
        uint32_t transientResourceCount = 0;
        uint32_t barrierCount = 0;
        uint64_t transientMemoryUsed = 0;
        float compileTime = 0.0f;
        float executeTime = 0.0f;
    };

    const Statistics &GetStatistics() const { return m_statistics; }
    uint32_t GetCurrentFrameIndex() const { return m_currentFrameIndex; }

private:
    /// <summary>
    /// Transient resource managed by the RenderGraph.
    /// Created and destroyed automatically based on pass requirements.
    /// </summary>
    struct TransientResource {
        std::string name;

        enum class Type {
            Texture,
            Buffer
        } type;

        // For textures
        Texture *texture = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;

        // For buffers
        Buffer *buffer = nullptr;
        uint64_t size = 0;

        // Lifetime tracking (pass indices)
        uint32_t firstUse = UINT32_MAX;
        uint32_t lastUse = 0;

        uint32_t currentStateFlag = 0;
        uint32_t initialStateFlag = 0;

        // Frame tracking - resource valid for this many frames after last use
        uint32_t lastUsedFrame = 0;
        bool canBeDestroyed = false;
    };

    /// <summary>
    /// Per-frame resources that need to survive GPU execution
    /// </summary>
    struct FrameTransientResources {
        std::unordered_map<std::string, TransientResource> resources;
        uint32_t frameIndex = 0;
    };

    /// <summary>
    /// Dependency between two render passes.
    /// Used for topological sorting and barrier insertion.
    /// </summary>
    struct PassDependency {
        RenderPass *producer;
        RenderPass *consumer;
        std::string resourceName;
    };

    /// <summary>
    /// External resource registration with state tracking
    /// </summary>
    struct ExternalResource {
        union {
            Texture *texture;
            Buffer *buffer;
        };

        enum class Type {
            Texture,
            Buffer
        } type;

        uint32_t currentStateFlag;
        uint32_t initialStateFlag; // State at start of graph
        bool isPresentTarget = false;
    };

    struct CompiledPass {
        RenderPass *pass = nullptr;
        uint32_t index = 0;
        std::vector<std::string> inputResourceNames;
        std::vector<std::string> outputResourceNames;
    };

    Device *m_device;
    CommandQueue *m_commandQueue;

    // Per-frame command lists
    std::vector<std::unique_ptr<CommandList> > m_commandLists;
    uint32_t m_currentFrameIndex = 0;
    uint32_t m_frameCount;

    // Pass management
    std::vector<std::unique_ptr<RenderPass> > m_passes;
    std::vector<CompiledPass> m_compiledPasses;
    std::vector<PassDependency> m_dependencies;

    // Resource management - per frame
    std::vector<FrameTransientResources> m_frameResources;

    // External resources - shared across frames
    std::unordered_map<std::string, ExternalResource> m_externalResources;
    std::string m_presentTarget;

    // Configuration
    bool m_autoBarriers = true;
    bool m_resourceAliasing = false;

    Statistics m_statistics;

    void BuildDependencyGraph();

    void TopologicalSort();

    void AllocateResources();

    Texture *CreateTransientTexture(const RenderPassResource &desc);

    Buffer *CreateTransientBuffer(const RenderPassResource &desc);

    void CalculateResourceLifetimes();

    void CleanupOldResources();

    void AliasResources();

    void InsertBarriers(uint32_t passIndex);

    void TransitionExternalResource(const std::string &name, uint32_t newState);

    void ExecutePass(const CompiledPass &compiledPass);

    RenderPassContext BuildPassContext(const CompiledPass &compiledPass);

    TransientResource *GetCurrentFrameResource(const std::string &name);

    TransientResource *GetOrCreateResource(const std::string &name,
                                           const RenderPassResource &desc);

    Texture *GetTexture(const std::string &name);

    Buffer *GetBuffer(const std::string &name);

    bool IsExternalResource(const std::string &name) const;

    void UpdateStatistics();

    void LogRenderGraph();
};

#endif //GPU_PARTICLE_SIM_RENDERGRAPH_H
