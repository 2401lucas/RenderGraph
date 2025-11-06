//
// Created by 2401Lucas on 2025-10-30.
//
// Engine/Rendering/Renderer.cpp
#include "Renderer.h"
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
    // Initialize directional light
    m_directionalLight.direction[0] = -1.0f;
    m_directionalLight.direction[1] = -1.0f;
    m_directionalLight.direction[2] = -1.0f;
    m_directionalLight.color[0] = 1.0f;
    m_directionalLight.color[1] = 1.0f;
    m_directionalLight.color[2] = 1.0f;
    m_directionalLight.intensity = 1.0f;

    CommandQueueCreateInfo graphicsQueueCI = {
        .type = QueueType::Graphics,
        .debugName = "Main Graphics Queue"
    };
    m_graphicsQueue = std::unique_ptr<CommandQueue>(m_device->CreateCommandQueue(graphicsQueueCI));

    // Create RenderGraph
    m_renderGraph = std::make_unique<RenderGraph>(m_device, m_graphicsQueue.get(), FrameCount);

    //TODO REMOVE WHEN PROPERLY IMPLEMENTED
    PipelineCreateInfo pipelineCI{
        .vertexShader = {
            .filepath = "assets/shaders/shaders.hlsl",
            .entry = "VSMain",
            .stage = ShaderStage::Vertex
        },
        .pixelShader = {
            .filepath = "assets/shaders/shaders.hlsl",
            .entry = "PSMain",
            .stage = ShaderStage::Pixel
        },
        .vertexAttributes = {
            {"POSITION", 0, TextureFormat::RGB32_FLOAT, offsetof(Vertex, position)},
            {"NORMAL", 0, TextureFormat::RGB32_FLOAT, offsetof(Vertex, normal)},
            {"TEXCOORD", 0, TextureFormat::RG32_FLOAT, offsetof(Vertex, texCoord)},
            {"TANGENT", 0, TextureFormat::RGB32_FLOAT, offsetof(Vertex, tangent)},
        },
        .vertexAttributeCount = 4,
        .vertexStride = sizeof(Vertex),
        .cullMode = CullMode::Back,
        .wireframe = false,
        .sampleCount = 1,
        .topology = PrimitiveTopology::TriangleList,
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthFunc = CompareFunc::Less,
        .blendMode = BlendMode::None,
        .renderTargetFormats = {TextureFormat::RGBA8_UNORM},
        .renderTargetCount = 1,
        .depthStencilFormat = TextureFormat::Depth32,
        .dynamicViewport = true,
        .dynamicScissor = true,
        .debugName = "MainPipeline",
    };

    m_mainPipeline = std::unique_ptr<Pipeline>(m_device->CreatePipeline(pipelineCI));

    BufferCreateInfo bufferCI = {
        .size = sizeof(PerObjData),
        .usage = BufferUsage::Uniform,
        .memoryType = MemoryType::Upload,
        .debugName = "PerObjDataBuffer",
    };
    m_swapchain = std::unique_ptr<Swapchain>(
        m_device->CreateSwapchain(window->getHwnd(), m_graphicsQueue.get(), window->getWidth(), window->getHeight()));
    m_perObjData = std::unique_ptr<Buffer>(m_device->CreateBuffer(bufferCI));
    m_graphicsQueue->WaitIdle();
    m_perObjData->Map();
}

Renderer::~Renderer() {
    if (m_renderGraph) {
        m_renderGraph->Flush();
    }

    WaitForGPU();

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

    // Wait ONLY for the specific frame we're about to reuse
    // This allows other frames to still be in-flight (better GPU utilization)
    uint32_t nextFrameIndex = (m_frameIndex + 1) % FrameCount;
    uint64_t fenceValueToWaitFor = m_frameResources[nextFrameIndex].fenceValue;
    if (fenceValueToWaitFor > 0) {
        m_graphicsQueue->WaitForFence(fenceValueToWaitFor);
    }

    m_graphicsQueue->BeginFrame(nextFrameIndex);
    m_renderGraph->NextFrame();
    m_frameIndex = nextFrameIndex;

    m_isFrameStarted = true;
    m_submissions.clear();
    m_batches.clear();
    m_statistics = Statistics{};
}

void Renderer::EndFrame() {
    if (!m_isFrameStarted) {
        throw std::runtime_error("EndFrame called without BeginFrame");
    }

    m_camera->SetAspectRatio(m_window->getAspectRatio());

    // 1. Process submissions
    ProcessSubmissions();

    // 2. Build RenderGraph
    BuildRenderGraph();

    // 3. Execute RenderGraph
    CommandList *commandList = m_renderGraph->Execute();

    // 4. Submit to queue with fence
    m_currentFenceValue++;
    m_graphicsQueue->Execute(commandList);
    m_graphicsQueue->Signal(m_currentFenceValue);
    m_graphicsQueue->WaitIdle();
    // Track fence value for this frame
    m_frameResources[m_frameIndex].fenceValue = m_currentFenceValue;

    // 5. Present
    m_swapchain->Present(m_window->isVSync());

    m_isFrameStarted = false;
}

// ===== Submission API =====

void Renderer::Submit(const RenderInfo &info) {
    m_submissions.push_back(info);
}

void Renderer::Submit(const std::vector<RenderInfo> &infos) {
    m_submissions.insert(m_submissions.end(), infos.begin(), infos.end());
}

void Renderer::SetCamera(Camera &camera) {
    m_camera = &camera;
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

    // Register backbuffer as external resource
    // It comes in as Present from last frame, or Undefined on first frame
    Texture *backbuffer = m_swapchain->GetSwapchainBuffer(m_frameIndex);
    m_renderGraph->RegisterExternalTexture("Backbuffer", backbuffer, TextureUsage::Present);
    m_renderGraph->SetPresentTarget("Backbuffer");

    // Shadow pass (if enabled)
    if (m_shadowsEnabled && !m_batches.empty()) {
        auto shadowPass = RenderPassBuilder("Shadow")
                .WriteTexture("ShadowMap", m_shadowMapSize, m_shadowMapSize,
                              RenderPassResource::Format::Depth32,
                              TextureUsage::DepthStencil)
                .Execute([this](RenderPassContext &ctx) {
                    RenderShadows(ctx);
                })
                .Build();

        m_renderGraph->AddPass(std::move(shadowPass));
    }

    // Main geometry pass
    auto mainPass = RenderPassBuilder("Main")
            .ReadTexture("ShadowMap", TextureUsage::ShaderResource) // If shadows enabled
            .WriteTexture("Backbuffer", width, height,
                          RenderPassResource::Format::RGBA16F,
                          TextureUsage::RenderTarget)
            .WriteTexture("SceneDepth", width, height,
                          RenderPassResource::Format::Depth32,
                          TextureUsage::DepthStencil)
            .Execute([this](RenderPassContext &ctx) {
                RenderMain(ctx);
            })
            .Build();

    m_renderGraph->AddPass(std::move(mainPass));

    // Post-process (if enabled)
    if (m_postProcessingEnabled) {
        auto postPass = RenderPassBuilder("PostProcess")
                .ReadTexture("SceneColor", TextureUsage::ShaderResource)
                .WriteTexture("FinalColor", width, height,
                              RenderPassResource::Format::RGBA8,
                              TextureUsage::RenderTarget)
                .Execute([this](RenderPassContext &ctx) {
                    RenderPostProcess(ctx);
                })
                .Build();

        m_renderGraph->AddPass(std::move(postPass));
    }
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
    const float clearColor[4] = {1.0f, 0.1f, 0.1f, 1.0f};
    auto renderTarget = ctx.GetTexture("Backbuffer");
    auto depthTarget = ctx.GetTexture("SceneDepth");

    ctx.commandList->ClearRenderTarget(renderTarget, clearColor);
    //ctx.commandList->ClearDepthStencil(depthTarget, 1.0f, 0);

    // Set render targets
    //ctx.commandList->SetRenderTarget(renderTarget, depthTarget);

    // Set viewport and scissor
    Viewport vp = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_window->getWidth()),
        .height = static_cast<float>(m_window->getHeight()),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    ctx.commandList->SetViewport(vp);

    Rect scissor = {
        .left = 0,
        .top = 0,
        .right = static_cast<int32_t>(m_window->getWidth()),
        .bottom = static_cast<int32_t>(m_window->getHeight())
    };
    ctx.commandList->SetScissor(scissor);

    // Render all batches
    for (const auto &batch: m_batches) {
        // ctx.commandList->SetPipeline(m_mainPipeline.get());
        // ctx.commandList->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
        ctx.commandList->SetTexture(m_resourceManager->GetTexture(batch.material->GetAlbedoTexture()), 0);
        ctx.commandList->SetVertexBuffer(batch.mesh->GetVertexBuffer());
        ctx.commandList->SetIndexBuffer(batch.mesh->GetIndexBuffer());

        // Update per-object constants
        PerObjData data;
        memcpy(m_perObjData->GetMappedPtr(), &data, sizeof(PerObjData));
        ctx.commandList->SetConstantBuffer(m_perObjData.get(), 0);

        // TODO:
        //  Bindless
        //  GPU GENERATED COMMANDS

        if (batch.transforms.size() == 1) {
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
