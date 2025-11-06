//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_RENDERPASS_H
#define GPU_PARTICLE_SIM_RENDERPASS_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>

#include "Rendering/RHI/CommandList.h"
#include "Rendering/RHI/Resource.h"

/// <summary>
/// Resource description for render pass inputs/outputs
/// </summary>
struct RenderPassResource {
    std::string name;

    enum class Type {
        Texture,
        Buffer
    } type;

    enum class Access {
        Read,
        Write,
        ReadWrite
    } access;

    // ResourceState state = ResourceState::Common;
    uint32_t stateFlag = 0;

    PipelineStage stage = PipelineStage::PixelShader;

    // For textures
    uint32_t width = 0;
    uint32_t height = 0;

    enum class Format {
        RGBA8,
        RGBA16F,
        RGBA32F,
        Depth32,
        R32
    } format;

    // For buffers
    uint64_t size = 0;
};

/// <summary>
/// Context provided to render pass execution function.
/// Contains command list and resources for the pass.
/// </summary>
struct RenderPassContext {
    CommandList *commandList = nullptr;

    // Resources for this pass
    std::vector<Texture *> inputTextures;
    std::vector<Texture *> outputTextures;
    std::vector<Buffer *> inputBuffers;
    std::vector<Buffer *> outputBuffers;

    // Frame info
    uint32_t frameIndex = 0;
    float deltaTime = 0.0f;

    // Helper to get resource by name
    Texture *GetTexture(const std::string &name) const;

    Buffer *GetBuffer(const std::string &name) const;
};

/// <summary>
/// Execution function signature for render passes.
/// Takes context and performs rendering operations.
/// </summary>
using RenderPassExecuteFunc = std::function<void(RenderPassContext &)>;

/// <summary>
/// Represents a single rendering pass in the render graph.
/// A pass declares its resource dependencies and execution function.
/// </summary>
class RenderPass {
public:
    RenderPass(const std::string &name);

    ~RenderPass();

    // ===== Configuration =====

    /// <summary>
    /// Set the execution function for this pass
    /// </summary>
    void SetExecuteFunc(RenderPassExecuteFunc func) { m_executeFunc = func; }

    /// <summary>
    /// Declare an input resource (read-only)
    /// </summary>
    void AddInput(const RenderPassResource &desc);

    /// <summary>
    /// Declare an output resource (write)
    /// </summary>
    void AddOutput(const RenderPassResource &desc);

    /// <summary>
    /// Declare a read-write resource
    /// </summary>
    void AddReadWrite(const RenderPassResource &desc);

    /// <summary>
    /// Set whether this pass should be enabled
    /// </summary>
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    // ===== Queries =====

    const std::string &GetName() const { return m_name; }
    bool IsEnabled() const { return m_enabled; }

    const bool IsValid() const { return m_executeFunc != nullptr; }

    const std::vector<RenderPassResource> &GetInputs() const { return m_inputs; }
    const std::vector<RenderPassResource> &GetOutputs() const { return m_outputs; }

    /// <summary>
    /// Execute this pass with the given context
    /// </summary>
    void Execute(RenderPassContext &context);

private:
    std::string m_name;
    bool m_enabled = true;

    std::vector<RenderPassResource> m_inputs;
    std::vector<RenderPassResource> m_outputs;

    RenderPassExecuteFunc m_executeFunc;

    friend class RenderGraph;
};

/// <summary>
/// Builder for constructing render passes.
/// Provides a "fluent" API for configuring passes.
/// </summary>
class RenderPassBuilder {
public:
    RenderPassBuilder(const std::string &name);

    RenderPassBuilder &ReadTexture(const std::string &name,
                                   TextureUsage state = TextureUsage::ShaderResource,
                                   PipelineStage stage = PipelineStage::PixelShader);

    RenderPassBuilder &WriteTexture(const std::string &name,
                                    uint32_t width, uint32_t height,
                                    RenderPassResource::Format format,
                                    TextureUsage state = TextureUsage::RenderTarget,
                                    PipelineStage stage = PipelineStage::RenderTarget);

    RenderPassBuilder &ReadWriteTexture(const std::string &name, uint32_t width, uint32_t height,
                                        RenderPassResource::Format format,
                                        TextureUsage state = TextureUsage::UnorderedAccess,
                                        PipelineStage stage = PipelineStage::ComputeShader);

    RenderPassBuilder &ReadBuffer(const std::string &name, BufferUsage state = BufferUsage::UnorderedAccess,
                                  PipelineStage stage = PipelineStage::ComputeShader);

    RenderPassBuilder &WriteBuffer(const std::string &name, uint64_t size,
                                   BufferUsage state = BufferUsage::UnorderedAccess,
                                   PipelineStage stage = PipelineStage::ComputeShader);

    RenderPassBuilder &Execute(RenderPassExecuteFunc func);

    RenderPassBuilder &Enable(bool enabled);

    std::unique_ptr<RenderPass> Build();

private:
    std::unique_ptr<RenderPass> m_pass;
};
#endif //GPU_PARTICLE_SIM_RENDERPASS_H
