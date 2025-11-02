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

    // Lifetime tracking
    uint32_t firstUse = 0; // First pass index that uses this resource
    uint32_t lastUse = 0; // Last pass index that uses this resource
    bool isAlive = false;

    ResourceState currentState = ResourceState::Undefined;

    // Per-pass state requirements
    struct PassUsage {
        uint32_t passIndex;
        ResourceState requiredState;
        ResourceAccess access;
        PipelineStage stage;
    };

    std::vector<PassUsage> usages;
};

/// <summary>
/// Dependency between two render passes.
/// Used for topological sorting and barrier insertion.
/// </summary>
struct PassDependency {
    RenderPass *producer; // Pass that writes the resource
    RenderPass *consumer; // Pass that reads the resource
    std::string resourceName;
};

/// <summary>
/// RenderGraph manages the execution of render passes.
///
/// Responsibilities:
/// - Schedules passes in dependency order
/// - Allocates and manages transient resources
/// - Inserts resource barriers automatically
/// - Optimizes resource lifetimes
/// </summary>
class RenderGraph {
public:
    explicit RenderGraph(Device *device);

    ~RenderGraph();

    // ===== Pass Management =====

    /// <summary>
    /// Add a render pass to the graph.
    /// Passes are executed in the order determined by dependencies.
    /// </summary>
    void AddPass(std::unique_ptr<RenderPass> pass);

    /// <summary>
    /// Remove all passes from the graph.
    /// Called at the start of each frame.
    /// </summary>
    void Clear();

    // ===== Execution =====

    /// <summary>
    /// Compile and execute the render graph.
    /// This performs:
    /// 1. Dependency analysis
    /// 2. Topological sorting
    /// 3. Resource allocation
    /// 4. Barrier insertion
    /// 5. Pass execution
    /// </summary>
    void Execute();

    // ===== Resource Management =====

    /// <summary>
    /// Register an external resource (not managed by graph).
    /// Example: swap chain back buffer
    /// </summary>
    void RegisterExternalTexture(const std::string &name, Texture *texture);

    /// <summary>
    /// Register an external buffer
    /// </summary>
    void RegisterExternalBuffer(const std::string &name, Buffer *buffer);

    /// <summary>
    /// Set the present target for automatic layout transition
    /// </summary>
    void SetPresentTarget(const std::string &name);

    /// <summary>
    /// Get a transient resource by name (for debugging)
    /// </summary>
    Texture *GetTexture(const std::string &name) const;

    Buffer *GetBuffer(const std::string &name) const;

    // ===== Configuration =====

    /// <summary>
    /// Enable/disable automatic barrier insertion
    /// </summary>
    void SetAutoBarriers(bool enable) { m_autoBarriers = enable; }

    /// <summary>
    /// Enable/disable resource aliasing optimization
    /// Resources with non-overlapping lifetimes can share memory
    /// </summary>
    void SetResourceAliasing(bool enable) { m_resourceAliasing = enable; }

    // ===== Statistics =====

    struct Statistics {
        uint32_t passCount = 0;
        uint32_t transientResourceCount = 0;
        uint32_t barrierCount = 0;
        uint64_t transientMemoryUsed = 0; // Bytes
        float compileTime = 0.0f; // Milliseconds
        float executeTime = 0.0f; // Milliseconds
    };

    const Statistics &GetStatistics() const { return m_statistics; }

private:
    // ===== Internal Structures =====

    struct CompiledPass {
        RenderPass *pass = nullptr;
        uint32_t index = 0;
        std::vector<TransientResource *> inputResources;
        std::vector<TransientResource *> outputResources;
    };

    // ===== Core Components =====

    Device *m_device;
    CommandList *m_commandList = nullptr;

    // Pass management
    std::vector<std::unique_ptr<RenderPass> > m_passes;
    std::vector<CompiledPass> m_compiledPasses;
    std::vector<PassDependency> m_dependencies;
    std::string m_presentTarget;

    // Resource management
    std::unordered_map<std::string, TransientResource> m_transientResources;
    std::unordered_map<std::string, Texture *> m_externalTextures;
    std::unordered_map<std::string, Buffer *> m_externalBuffers;

    // Configuration
    bool m_autoBarriers = true;
    bool m_resourceAliasing = false;

    // Statistics
    Statistics m_statistics;

    // ===== Compilation =====

    /// <summary>
    /// Analyze pass dependencies and build dependency graph
    /// </summary>
    void BuildDependencyGraph();

    /// <summary>
    /// Sort passes in execution order using topological sort
    /// </summary>
    void TopologicalSort();

    /// <summary>
    /// Detect circular dependencies (deadlock detection)
    /// </summary>
    void DetectCycles();

    // ===== Resource Management =====

    /// <summary>
    /// Allocate all transient resources needed by passes
    /// </summary>
    void AllocateResources();

    /// <summary>
    /// Create a transient texture
    /// </summary>
    Texture *CreateTransientTexture(const RenderPassResource &desc);

    /// <summary>
    /// Create a transient buffer
    /// </summary>
    Buffer *CreateTransientBuffer(const RenderPassResource &desc);

    /// <summary>
    /// Calculate resource lifetimes (first/last use)
    /// </summary>
    void CalculateResourceLifetimes();

    /// <summary>
    /// Free resources that are no longer needed
    /// </summary>
    void ReleaseDeadResources(uint32_t currentPassIndex);

    /// <summary>
    /// Alias resources with non-overlapping lifetimes
    /// </summary>
    void AliasResources();

    // ===== Barrier Insertion =====

    /// <summary>
    /// Insert resource barriers between passes
    /// </summary>
    void InsertBarriers(uint32_t passIndex);

    /// <summary>
    /// Transition a texture to a new state
    /// </summary>
    void TransitionTexture(Texture *texture, const std::string &passName);

    /// <summary>
    /// Transition a buffer to a new state
    /// </summary>
    void TransitionBuffer(Buffer *buffer, const std::string &passName);

    // ===== Execution =====

    /// <summary>
    /// Execute a single compiled pass
    /// </summary>
    void ExecutePass(const CompiledPass &compiledPass);

    /// <summary>
    /// Build context for pass execution
    /// </summary>
    RenderPassContext BuildPassContext(const CompiledPass &compiledPass);

    // ===== Helpers =====

    /// <summary>
    /// Check if a resource is external
    /// </summary>
    bool IsExternalResource(const std::string &name) const;

    /// <summary>
    /// Get or create a transient resource
    /// </summary>
    TransientResource *GetOrCreateResource(const std::string &name,
                                           const RenderPassResource &desc);

    /// <summary>
    /// Update statistics
    /// </summary>
    void UpdateStatistics();

    /// <summary>
    /// Log render graph info (debug)
    /// </summary>
    void LogRenderGraph();
};

#endif //GPU_PARTICLE_SIM_RENDERGRAPH_H
