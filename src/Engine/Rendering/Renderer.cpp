//
// Created by 2401Lucas on 2025-10-30.
//
// Engine/Rendering/Renderer.cpp
#include "Renderer.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderPass.h"
#include "RHI/Device.h"
#include "../OS/Window/Window.h"
#include <algorithm>
#include <stdexcept>
#include <chrono>


// ===== Renderer Implementation =====

Renderer::Renderer(Window *window, Device *device, ResourceManager *resourceManager)
    : m_window(window), m_device(device), m_resourceManager(resourceManager)
      , m_isFrameStarted(false) {
    // Initialize camera to default values
    m_camera.position[0] = 0.0f;
    m_camera.position[1] = 0.0f;
    m_camera.position[2] = 5.0f;
    m_camera.forward[0] = 0.0f;
    m_camera.forward[1] = 0.0f;
    m_camera.forward[2] = -1.0f;
    m_camera.up[0] = 0.0f;
    m_camera.up[1] = 1.0f;
    m_camera.up[2] = 0.0f;
    m_camera.fov = 60.0f;
    m_camera.nearPlane = 0.1f;
    m_camera.farPlane = 1000.0f;

    // Initialize directional light
    m_directionalLight.direction[0] = -1.0f;
    m_directionalLight.direction[1] = -1.0f;
    m_directionalLight.direction[2] = -1.0f;
    m_directionalLight.color[0] = 1.0f;
    m_directionalLight.color[1] = 1.0f;
    m_directionalLight.color[2] = 1.0f;
    m_directionalLight.intensity = 1.0f;

    m_commandQueue = m_device->CreateCommandQueue();

    // Create per-frame resources
    for (uint32_t i = 0; i < FrameCount; i++) {
        m_frameResources[i].commandList = m_device->CreateCommandList();
    }

    // Create RenderGraph
    m_renderGraph = std::make_unique<RenderGraph>(m_device);
}

Renderer::~Renderer() {
    WaitForGPU();

    // Cleanup frame resources
    for (auto &frameRes: m_frameResources) {
        if (frameRes.commandList) {
            // m_device->DestroyCommandList(frameRes.commandList);
            frameRes.commandList = nullptr;
        }
    }

    if (m_commandQueue) {
        // m_device->DestroyCommandQueue(m_commandQueue);
        m_commandQueue = nullptr;
    }

    OutputDebugStringA("Renderer: Shutdown complete\n");
}


void Renderer::Update(float deltaTime) {
    m_deltaTime = deltaTime;

    // Update statistics
    UpdateStatistics();
}

// ===== Frame Management =====

void Renderer::BeginFrame() {
    if (m_isFrameStarted) {
        throw std::runtime_error("BeginFrame called twice without EndFrame");
    }

    m_isFrameStarted = true;

    // Clear submissions from previous frame
    m_submissions.clear();
    m_batches.clear();

    // Reset statistics
    m_statistics = Statistics{};
}

void Renderer::EndFrame() {
    if (!m_isFrameStarted) {
        throw std::runtime_error("EndFrame called without BeginFrame");
    }

    // 1. Process submissions (sort, batch, optimize)
    ProcessSubmissions();

    // 2. Build and execute RenderGraph
    BuildRenderGraph();
    ExecuteRenderGraph();

    // 3. Present
    Present();

    m_isFrameStarted = false;

    // Move to next frame
    m_frameIndex = (m_frameIndex + 1) % FrameCount;
}

void Renderer::Present() {
    // Present is handled by RenderGraph/Device
    // This is a placeholder for any additional present logic

    //TODO implement
}

// ===== Submission API =====

void Renderer::Submit(const RenderInfo &info) {
    m_submissions.push_back(info);
}

void Renderer::Submit(const std::vector<RenderInfo> &infos) {
    m_submissions.insert(m_submissions.end(), infos.begin(), infos.end());
}

void Renderer::SetCamera(const Camera &camera) {
    // Copy camera data
    // This would need actual Camera class implementation
    // For now, this is a placeholder
}

void Renderer::SetDirectionalLight(const float direction[3], const float color[3], float intensity) {
    for (int i = 0; i < 3; i++) {
        m_directionalLight.direction[i] = direction[i];
        m_directionalLight.color[i] = color[i];
    }
    m_directionalLight.intensity = intensity;
}

// ===== Submission Processing =====

void Renderer::ProcessSubmissions() {
    if (m_submissions.empty()) {
        return;
    }

    // Calculate sort keys based on distance, material, etc.
    CalculateSortKeys();

    // Sort submissions
    SortSubmissions();

    // Batch similar objects for instanced rendering
    BatchSubmissions();
}

void Renderer::CalculateSortKeys() {
    for (auto &submission: m_submissions) {
        // Calculate distance to camera
        // Todo: implement depth sort
        // submission.distanceToCamera = CalculateDistance(submission.transform, m_camera.position);
        submission.distanceToCamera = 0;
        // Create sort key (material ID << 16 | depth)
        // This ensures we sort by material first, then by depth
        uint32_t materialID = submission.material->GetAlbedoTexture().id;
        uint16_t depthKey = static_cast<uint16_t>(submission.distanceToCamera * 100.0f);

        if (submission.isTransparent) {
            // Transparent objects sort back-to-front
            submission.sortKey = (1u << 31) | (materialID << 16) | (0xFFFF - depthKey);
        } else {
            // Opaque objects sort front-to-back
            submission.sortKey = (0u << 31) | (materialID << 16) | depthKey;
        }
    }
}

void Renderer::SortSubmissions() {
    // Sort by sort key
    std::sort(m_submissions.begin(), m_submissions.end(),
              [](const RenderInfo &a, const RenderInfo &b) {
                  return a.sortKey < b.sortKey;
              });
}

void Renderer::BatchSubmissions() {
    m_batches.clear();

    if (m_submissions.empty()) {
        return;
    }

    // Group consecutive submissions with same mesh and material
    RenderBatch currentBatch;
    currentBatch.mesh = m_submissions[0].mesh;
    currentBatch.material = m_submissions[0].material;
    currentBatch.castsShadows = m_submissions[0].castsShadows;
    currentBatch.transforms.push_back(m_submissions[0].transform);

    for (size_t i = 1; i < m_submissions.size(); i++) {
        const auto &submission = m_submissions[i];

        // Check if we can batch with current batch
        if (submission.mesh == currentBatch.mesh &&
            submission.material == currentBatch.material &&
            submission.castsShadows == currentBatch.castsShadows &&
            !submission.isTransparent) {
            // Add to current batch
            currentBatch.transforms.push_back(submission.transform);
        } else {
            // Save current batch and start new one
            m_batches.push_back(currentBatch);

            currentBatch = RenderBatch{};
            currentBatch.mesh = submission.mesh;
            currentBatch.material = submission.material;
            currentBatch.castsShadows = submission.castsShadows;
            currentBatch.transforms.push_back(submission.transform);
        }
    }

    // Add final batch
    m_batches.push_back(currentBatch);

    // Update statistics
    m_statistics.drawCalls = static_cast<uint32_t>(m_submissions.size());
    m_statistics.instancedDrawCalls = static_cast<uint32_t>(m_batches.size());

    uint32_t totalInstances = 0;
    for (const auto &batch: m_batches) {
        totalInstances += static_cast<uint32_t>(batch.transforms.size());
    }
    m_statistics.instanceCount = totalInstances;
}

// ===== RenderGraph Management =====

void Renderer::BuildRenderGraph() {
    m_renderGraph->Clear();

    uint32_t width = m_window->getWidth();
    uint32_t height = m_window->getHeight();

    // Register External Resources

    m_renderGraph->RegisterExternalTexture("Backbuffer", m_swapchain->GetSwapchainBuffer(m_frameIndex));
    m_renderGraph->SetPresentTarget("Backbuffer");

    // Shadow pass
    if (m_shadowsEnabled && !m_batches.empty()) {
        auto shadowPass = RenderPassBuilder("Shadow")
                .WriteTexture("ShadowMap", m_shadowMapSize, m_shadowMapSize,
                              RenderPassResource::Format::Depth32)
                .Execute([this](RenderPassContext &ctx) {
                    RenderShadows(ctx);
                })
                .Build();

        m_renderGraph->AddPass(std::move(shadowPass));
    }
    // Main geometry pass
    auto mainPass = RenderPassBuilder("Main")
            // .ReadTexture("ShadowMap")
            //.WriteTexture("SceneColor", width, height, RenderPassResource::Format::RGBA16F)
            .WriteTexture("Backbuffer", width, height, RenderPassResource::Format::RGBA16F, ResourceState::RenderTarget,
                          PipelineStage::RenderTarget)
            .WriteTexture("SceneDepth", width, height, RenderPassResource::Format::Depth32, ResourceState::DepthWrite,
                          PipelineStage::DepthStencil)
            .Execute([this](RenderPassContext &ctx) {
                RenderMain(ctx);
            })
            .Build();

    m_renderGraph->AddPass(std::move(mainPass));

    // Particle pass
    // auto particlePass = RenderPassBuilder("Particles")
    //         .ReadTexture("SceneDepth")
    //         .ReadWriteTexture("SceneColor")
    //         .Execute([this](RenderPassContext &ctx) {
    //             RenderParticles(ctx);
    //         })
    //         .Build();
    //
    // m_renderGraph->AddPass(std::move(particlePass));

    // Post-process pass
    if (m_postProcessingEnabled) {
        auto postPass = RenderPassBuilder("PostProcess")
                .ReadTexture("SceneColor")
                .WriteTexture("FinalColor", width, height, RenderPassResource::Format::RGBA8)
                .Execute([this](RenderPassContext &ctx) {
                    RenderPostProcess(ctx);
                })
                .Build();

        m_renderGraph->AddPass(std::move(postPass));
    }

    // UI pass
    // auto uiPass = RenderPassBuilder("UI")
    //         .ReadWriteTexture("FinalColor")
    //         .Execute([this](RenderPassContext &ctx) {
    //             RenderUI(ctx);
    //         })
    //         .Build();
    //
    // m_renderGraph->AddPass(std::move(uiPass));
}

void Renderer::ExecuteRenderGraph() {
    m_renderGraph->Execute();
}

// ===== Rendering Functions =====

//TODO implement
void Renderer::RenderShadows(RenderPassContext &ctx) {
    // Render shadow-casting geometry
    for (const auto &batch: m_batches) {
        if (!batch.castsShadows) continue;

        // Set pipeline for shadow rendering
        // Bind mesh buffers
        // Draw instances

        //TODO implement
        if (batch.transforms.size() == 1) {
            // Single draw
            // ctx.commandList->DrawIndexed(...);
        } else {
            // Instanced draw
            // ctx.commandList->DrawIndexedInstanced(...);
        }
    }
}

void Renderer::RenderMain(RenderPassContext &ctx) {
    // Clear
    constexpr float clearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    Clear(clearColor);
    // Render all batches
    for (const auto &batch: m_batches) {
        // ctx.commandList->SetPipeline(m_resourceManager->GetShader(m_resourceManager->LoadShader(batch.material->GetShader()));); // TODO Some sort of active check to see if pipeline rebind is required?
        ctx.commandList->SetTexture(m_resourceManager->GetTexture(batch.material->GetAlbedoTexture()), 0);
        ctx.commandList->SetVertexBuffer(batch.mesh->GetVertexBuffer());
        ctx.commandList->SetIndexBuffer(batch.mesh->GetIndexBuffer());

        // Update per-object constants
        // Draw

        if (batch.transforms.size() == 1) {
            // Single draw
            ctx.commandList->Draw(batch.mesh->GetVertexCount(), 0);
        } else {
            // Instanced draw
        }
    }
}

//TODO implement
void Renderer::RenderParticles(RenderPassContext &ctx) {
    // Render particle systems
    // Would iterate over submitted particle systems
}

//TODO implement
void Renderer::RenderPostProcess(RenderPassContext &ctx) {
    // Apply post-processing effects
    // Tone mapping, bloom, etc.
}

//TODO implement
void Renderer::RenderUI(RenderPassContext &ctx) {
    // Render UI elements
}

//TODO: REMOVE
// void Renderer::UploadBufferData(Buffer *buffer, const void *data, size_t size) {
//     // Create staging buffer
//     BufferCreateInfo stagingInfo{
//         .size = size,
//         .usage = BufferUsage::Transfer,
//         .memoryType = MemoryType::Upload,
//     };
//
//     Buffer *stagingBuffer = m_device->CreateBuffer(stagingInfo);
//
//     // Map and copy
//     void *mapped = stagingBuffer->Map();
//     memcpy(mapped, data, size);
//     stagingBuffer->Unmap();
//
//     // Copy to GPU buffer
//     CommandList *cmdList = m_frameResources[m_frameIndex].commandList;
//     cmdList->Begin();
//     cmdList->CopyBuffer(stagingBuffer, buffer, size);
//     cmdList->End();
//
//     // Execute
//     m_commandQueue->Execute(cmdList);
//     m_commandQueue->WaitIdle();
//
//     // Cleanup
//     m_device->DestroyBuffer(stagingBuffer);
// }
//
// void Renderer::UploadTextureData(Texture *texture, const void *data, size_t size) {
//     // Create staging buffer
//     BufferCreateInfo stagingInfo{
//         .size = size,
//         .usage = BufferUsage::Transfer,
//         .memoryType = MemoryType::Upload,
//     };
//
//     Buffer *stagingBuffer = m_device->CreateBuffer(stagingInfo);
//
//     // Map and copy
//     void *mapped = stagingBuffer->Map();
//     memcpy(mapped, data, size);
//     stagingBuffer->Unmap();
//
//     // Copy to GPU buffer
//     CommandList *cmdList = m_frameResources[m_frameIndex].commandList;
//     cmdList->Begin();
//     cmdList->TransitionTexture(texture, Undefined, TransferDst);
//     cmdList->CopyBufferToTexture(stagingBuffer, texture);
//     cmdList->TransitionTexture(texture, TransferDst, ShaderReadOnly);
//     cmdList->End();
//
//     // Execute
//     m_commandQueue->Execute(cmdList);
//     m_commandQueue->WaitIdle();
//
//     // Cleanup
//     m_device->DestroyBuffer(stagingBuffer);
// }

void Renderer::Clear(const float color[4]) {
    // Clear is handled by RenderGraph passes
    // This is a placeholder
}

void Renderer::WaitForGPU() {
    if (m_device) {
        m_device->WaitIdle();
    }
}

void Renderer::UpdateStatistics() {
    // Update frame time statistics
    // This would track CPU and GPU times
}

bool Renderer::ShouldClose() const {
    return m_window->shouldClose();
}
