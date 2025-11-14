//
// Created by 2401Lucas on 2025-10-30.
//

#include "RenderPass.h"
#include <algorithm>
#include <stdexcept>

// TODO: Implement this
Texture *RenderPassContext::GetTexture(const std::string &name) const {
    // Search in input textures
    for (auto *texture: inputTextures) {
        return texture;
    }

    // Search in output textures
    for (auto *texture: outputTextures) {
        return texture;
    }

    return nullptr;
}

// TODO: Implement this
Buffer *RenderPassContext::GetBuffer(const std::string &name) const {
    // Similar implementation as GetTexture
    for (auto *buffer: inputBuffers) {
        return buffer;
    }

    for (auto *buffer: outputBuffers) {
        return buffer;
    }

    return nullptr;
}

RenderPass::RenderPass(const std::string &name)
    : m_name(name)
      , m_enabled(true) {
}

RenderPass::~RenderPass() {
}

void RenderPass::AddInput(const RenderPassResource &desc) {
    m_inputs.push_back(desc);
}

void RenderPass::AddOutput(const RenderPassResource &desc) {
    m_outputs.push_back(desc);
}

void RenderPass::AddReadWrite(const RenderPassResource &desc) {
    // Add to both inputs and outputs
    m_inputs.push_back(desc);
    m_outputs.push_back(desc);
}

void RenderPass::Execute(RenderPassContext &context) {
    if (!m_enabled) {
        return;
    }

    if (!m_executeFunc) {
        throw std::runtime_error("RenderPass '" + m_name + "' has no execute function");
    }

    m_executeFunc(context);
}

RenderPassBuilder::RenderPassBuilder(const std::string &name)
    : m_pass(std::make_unique<RenderPass>(name)) {
}

RenderPassBuilder &RenderPassBuilder::ReadTexture(const std::string &name,
                                                  TextureUsage state,
                                                  PipelineStage stage) {
    RenderPassResource resource{
        .name = name,
        .type = RenderPassResource::Type::Texture,
        .access = RenderPassResource::Access::Read,
        .stateFlag = static_cast<uint32_t>(state),
        .stage = stage,
    };

    m_pass->AddInput(resource);
    return *this;
}

RenderPassBuilder &RenderPassBuilder::WriteTexture(const std::string &name,
                                                   uint32_t width, uint32_t height,
                                                   RenderPassResource::Format format,
                                                   TextureUsage state,
                                                   PipelineStage stage) {
    RenderPassResource resource{
        .name = name,
        .type = RenderPassResource::Type::Texture,
        .access = RenderPassResource::Access::Write,
        .stateFlag = static_cast<uint32_t>(state),
        .stage = stage,
        .width = width,
        .height = height,
        .format = format,
    };

    m_pass->AddOutput(resource);
    return *this;
}

RenderPassBuilder &RenderPassBuilder::ReadWriteTexture(const std::string &name, uint32_t width, uint32_t height,
                                                       RenderPassResource::Format format, TextureUsage state,
                                                       PipelineStage stage) {
    RenderPassResource resource{
        .name = name,
        .type = RenderPassResource::Type::Texture,
        .access = RenderPassResource::Access::ReadWrite,
        .stateFlag = static_cast<uint32_t>(state),
        .stage = stage,
        .width = width,
        .height = height,
        .format = format,
    };

    m_pass->AddReadWrite(resource);
    return *this;
}

RenderPassBuilder &RenderPassBuilder::ReadBuffer(const std::string &name, BufferUsage state,
                                                 PipelineStage stage) {
    RenderPassResource resource{
        .name = name,
        .type = RenderPassResource::Type::Buffer,
        .access = RenderPassResource::Access::Read,
        .stateFlag = static_cast<uint32_t>(state),
        .stage = stage,
    };

    m_pass->AddInput(resource);
    return *this;
}

RenderPassBuilder &RenderPassBuilder::WriteBuffer(const std::string &name, uint64_t size, BufferUsage state,
                                                  PipelineStage stage) {
    RenderPassResource resource{
        .name = name,
        .type = RenderPassResource::Type::Buffer,
        .access = RenderPassResource::Access::Write,
        .stateFlag = static_cast<uint32_t>(state),
        .stage = stage,
        .size = size,
    };

    m_pass->AddOutput(resource);
    return *this;
}

RenderPassBuilder &RenderPassBuilder::Execute(RenderPassExecuteFunc func) {
    m_pass->SetExecuteFunc(func);
    return *this;
}

RenderPassBuilder &RenderPassBuilder::Enable(bool enabled) {
    m_pass->SetEnabled(enabled);
    return *this;
}

std::unique_ptr<RenderPass> RenderPassBuilder::Build() {
    if (!m_pass->IsValid()) {
        throw std::runtime_error("RenderPass must have an execute function");
    }
    return std::move(m_pass);
}
