//
// Created by 2401Lucas on 2025-10-31.
//

#ifndef GPU_PARTICLE_SIM_PIPELINE_H
#define GPU_PARTICLE_SIM_PIPELINE_H

#include "Shader.h"
#include "Texture.h"

enum class PipelineStage {
    Top, // Beginning of pipeline
    VertexShader,
    PixelShader,
    ComputeShader,
    RenderTarget,
    DepthStencil,
    Transfer,
    Bottom, // End of pipeline
};

enum class BlendMode {
    None,
    Alpha,
    Additive,
    Multiply
};

enum class CullMode {
    None,
    Front,
    Back
};

enum class CompareFunc {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

struct PipelineDesc {
    // ShaderHandle  vertexShader = nullptr;
    // ShaderHandle  pixelShader = nullptr;
    // ShaderHandle  computeShader = nullptr

    // Input layout
    struct VertexAttribute {
        const char *semantic;
        uint32_t index;
        //TextureFormat format;
        uint32_t offset;
    };

    VertexAttribute *vertexAttributes = nullptr;
    uint32_t vertexAttributeCount = 0;
    uint32_t vertexStride = 0;

    // Rasterizer state
    CullMode cullMode = CullMode::Back;
    bool wireframe = false;

    // Depth/Stencil state
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    CompareFunc depthFunc = CompareFunc::Less;

    // Blend state
    BlendMode blendMode = BlendMode::None;

    // Render target formats
    // TextureFormat renderTargetFormats[8] = {};
    // uint32_t renderTargetCount = 1;
    // TextureFormat depthStencilFormat = TextureFormat::Depth32;

    const char *debugName = nullptr;
};

class Pipeline {
public:
    virtual ~Pipeline() = default;

    virtual void *GetNativeHandle() const = 0;
};

#endif //GPU_PARTICLE_SIM_PIPELINE_H
