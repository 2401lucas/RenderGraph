//
// Created by 2401Lucas on 2025-10-30.
//

#include "RenderGraph.h"
#include "RenderPass.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <queue>
#include <chrono>

RenderGraph::RenderGraph(Device *device)
    : m_device(device) {
    // Create command list for graph execution
    m_commandList = m_device->CreateCommandList();
}

RenderGraph::~RenderGraph() {
    Clear();

    // Cleanup command list
    if (m_commandList) {
        // m_device->DestroyCommandList(m_commandList);
        m_commandList = nullptr;
    }
}


// ===== Pass Management =====

void RenderGraph::AddPass(std::unique_ptr<RenderPass> pass) {
    if (!pass) {
        throw std::runtime_error("Cannot add null pass to RenderGraph");
    }

    m_passes.push_back(std::move(pass));
}

void RenderGraph::Clear() {
    // Clear passes
    m_passes.clear();
    m_compiledPasses.clear();
    m_dependencies.clear();

    // Clean up transient resources
    for (auto &[name, resource]: m_transientResources) {
        if (resource.texture) {
            m_device->DestroyTexture(resource.texture);
        }
        if (resource.buffer) {
            m_device->DestroyBuffer(resource.buffer);
        }
    }
    m_transientResources.clear();

    // Clear external resource references (don't delete, we don't own them)
    m_externalTextures.clear();
    m_externalBuffers.clear();
}

// ===== Execution =====

void RenderGraph::Execute() {
    auto startTime = std::chrono::high_resolution_clock::now();

    // 1. Build dependency graph
    BuildDependencyGraph();

    // 2. Sort passes topologically
    TopologicalSort();

    // 3. Allocate transient resources
    AllocateResources();

    // 4. Calculate resource lifetimes
    CalculateResourceLifetimes();

    // 5. Alias resources if enabled
    if (m_resourceAliasing) {
        AliasResources();
    }

    auto compileTime = std::chrono::high_resolution_clock::now();
    m_statistics.compileTime = std::chrono::duration<float, std::milli>(
        compileTime - startTime).count();

    // Log graph for debugging
#if defined(_DEBUG)
    LogRenderGraph();
#endif

    // 6. Execute passes
    for (size_t i = 0; i < m_compiledPasses.size(); i++) {
        const auto &compiledPass = m_compiledPasses[i];

        if (!compiledPass.pass->IsEnabled()) {
            continue;
        }

        // Insert barriers before pass
        if (m_autoBarriers) {
            InsertBarriers(static_cast<uint32_t>(i));
        }

        // Execute the pass
        ExecutePass(compiledPass);

        // Release resources no longer needed
        ReleaseDeadResources(static_cast<uint32_t>(i));
    }

    // Automatically transition presentTarget to be ready to present
    if (!m_presentTarget.empty()) {
        auto it = m_externalTextures.find(m_presentTarget);
        if (it != m_externalTextures.end()) {
            Texture *presentTexture = it->second;

            // Transition to Present state
            m_commandList->TransitionTexture(
                presentTexture,
                ResourceState::RenderTarget, // Assume coming from RT
                ResourceState::Present
            );
        }
    }

    auto executeTime = std::chrono::high_resolution_clock::now();
    m_statistics.executeTime = std::chrono::duration<float, std::milli>(
        executeTime - compileTime).count();

    // Update statistics
    UpdateStatistics();
}

// ===== Compilation: Dependency Analysis =====

void RenderGraph::BuildDependencyGraph() {
    m_dependencies.clear();

    // Build map of resource producers (passes that write resources)
    std::unordered_map<std::string, RenderPass *> producers;

    for (auto &pass: m_passes) {
        for (const auto &output: pass->GetOutputs()) {
            producers[output.name] = pass.get();
        }
    }

    // Find dependencies: if pass reads a resource, it depends on the pass that writes it
    for (auto &pass: m_passes) {
        for (const auto &input: pass->GetInputs()) {
            auto it = producers.find(input.name);
            if (it != producers.end()) {
                // Found a dependency
                PassDependency dep;
                dep.producer = it->second;
                dep.consumer = pass.get();
                dep.resourceName = input.name;
                m_dependencies.push_back(dep);
            }
            // If not found, assume it's an external resource
        }
    }
}

void RenderGraph::TopologicalSort() {
    m_compiledPasses.clear();

    if (m_passes.empty()) {
        return;
    }

    // Build adjacency list and in-degree count
    std::unordered_map<RenderPass *, std::vector<RenderPass *> > adjacencyList;
    std::unordered_map<RenderPass *, int> inDegree;

    // Initialize
    for (auto &pass: m_passes) {
        inDegree[pass.get()] = 0;
        adjacencyList[pass.get()] = {};
    }

    // Build graph
    for (const auto &dep: m_dependencies) {
        adjacencyList[dep.producer].push_back(dep.consumer);
        inDegree[dep.consumer]++;
    }

    // Kahn's algorithm for topological sort
    std::queue<RenderPass *> queue;

    // Add all nodes with no incoming edges
    for (auto &pass: m_passes) {
        if (inDegree[pass.get()] == 0) {
            queue.push(pass.get());
        }
    }

    uint32_t index = 0;
    while (!queue.empty()) {
        RenderPass *current = queue.front();
        queue.pop();

        // Add to sorted list
        CompiledPass compiled;
        compiled.pass = current;
        compiled.index = index++;
        m_compiledPasses.push_back(compiled);

        // Reduce in-degree for neighbors
        for (RenderPass *neighbor: adjacencyList[current]) {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                queue.push(neighbor);
            }
        }
    }

    // Check for cycles
    if (m_compiledPasses.size() != m_passes.size()) {
        throw std::runtime_error("RenderGraph contains circular dependencies!");
    }
}

void RenderGraph::DetectCycles() {
    // Already handled by TopologicalSort
    // If sort produces fewer passes than input, there's a cycle
}

// ===== Resource Management =====

void RenderGraph::AllocateResources() {
    for (auto &compiled: m_compiledPasses) {
        RenderPass *pass = compiled.pass;

        // Allocate output resources
        for (const auto &output: pass->GetOutputs()) {
            if (IsExternalResource(output.name)) {
                continue; // External resources are not managed by graph
            }

            auto *resource = GetOrCreateResource(output.name, output);
            compiled.outputResources.push_back(resource);
        }

        // Reference input resources
        for (const auto &input: pass->GetInputs()) {
            if (IsExternalResource(input.name)) {
                continue;
            }

            auto it = m_transientResources.find(input.name);
            if (it != m_transientResources.end()) {
                compiled.inputResources.push_back(&it->second);
            }
        }
    }
}

Texture *RenderGraph::CreateTransientTexture(const RenderPassResource &desc) {
    TextureCreateInfo textureCI{
        .width = desc.width,
        .height = desc.height,
        .depth = 1,
        .mipLevels = 1,
        .arraySize = 1,
    };

    // Map format
    switch (desc.format) {
        case RenderPassResource::Format::RGBA8:
            textureCI.format = TextureFormat::RGBA8_UNORM;
            break;
        case RenderPassResource::Format::RGBA16F:
            textureCI.format = TextureFormat::RGBA16_FLOAT;
            break;
        case RenderPassResource::Format::RGBA32F:
            textureCI.format = TextureFormat::RGBA32_FLOAT;
            break;
        case RenderPassResource::Format::Depth32:
            textureCI.format = TextureFormat::Depth32;
            break;
        default:
            break;
    }

    return m_device->CreateTexture(textureCI);
}

Buffer *RenderGraph::CreateTransientBuffer(const RenderPassResource &desc) {
    BufferCreateInfo bufferInfo;
    bufferInfo.size = desc.size;
    bufferInfo.usage = BufferUsage::Storage;
    bufferInfo.memoryType = MemoryType::GPU;

    return m_device->CreateBuffer(bufferInfo);
}

void RenderGraph::CalculateResourceLifetimes() {
    // Reset lifetimes
    for (auto &[name, resource]: m_transientResources) {
        resource.firstUse = UINT32_MAX;
        resource.lastUse = 0;
    }

    // Calculate first and last use for each resource
    for (const auto &compiled: m_compiledPasses) {
        uint32_t passIndex = compiled.index;

        // Check inputs
        for (const auto &input: compiled.pass->GetInputs()) {
            auto it = m_transientResources.find(input.name);
            if (it != m_transientResources.end()) {
                it->second.firstUse = std::min(it->second.firstUse, passIndex);
                it->second.lastUse = std::max(it->second.lastUse, passIndex);
            }
        }

        // Check outputs
        for (const auto &output: compiled.pass->GetOutputs()) {
            auto it = m_transientResources.find(output.name);
            if (it != m_transientResources.end()) {
                it->second.firstUse = std::min(it->second.firstUse, passIndex);
                it->second.lastUse = std::max(it->second.lastUse, passIndex);
            }
        }
    }
}

void RenderGraph::ReleaseDeadResources(uint32_t currentPassIndex) {
    for (auto &[name, resource]: m_transientResources) {
        if (resource.isAlive && resource.lastUse == currentPassIndex) {
            // Resource is no longer needed, free it
            if (resource.texture) {
                m_device->DestroyTexture(resource.texture);
                resource.texture = nullptr;
            }
            if (resource.buffer) {
                m_device->DestroyBuffer(resource.buffer);
                resource.buffer = nullptr;
            }
            resource.isAlive = false;
        }
    }
}

void RenderGraph::AliasResources() {
    // Find resources with non-overlapping lifetimes that could share memory
    // This is a simplified version - real implementation would be more sophisticated

    std::vector<TransientResource *> textures;
    for (auto &[name, resource]: m_transientResources) {
        if (resource.type == TransientResource::Type::Texture) {
            textures.push_back(&resource);
        }
    }

    // Sort by first use
    std::sort(textures.begin(), textures.end(),
              [](const TransientResource *a, const TransientResource *b) {
                  return a->firstUse < b->firstUse;
              });

    // Try to alias resources
    for (size_t i = 0; i < textures.size(); i++) {
        for (size_t j = i + 1; j < textures.size(); j++) {
            // Check if lifetimes don't overlap
            if (textures[i]->lastUse < textures[j]->firstUse) {
                // TODO  alias these resources
            }
        }
    }
}

// ===== Barrier Insertion =====
// TODO: Barriers for Buffers?
void RenderGraph::InsertBarriers(uint32_t passIndex) {
    const auto &compiled = m_compiledPasses[passIndex];
    for (const auto &input: compiled.pass->GetInputs()) {
        auto it = m_transientResources.find(input.name);
        if (it == m_transientResources.end()) {
            // External resource or doesn't exist
            continue;
        }

        TransientResource &resource = it->second;
        // Check if state transition is needed
        ResourceState currentState = resource.currentState;
        ResourceState requiredState = input.state;

        if (currentState != requiredState) {
            // Insert barrier
            if (resource.type == TransientResource::Type::Texture) {
                m_commandList->TransitionTexture(
                    resource.texture,
                    currentState,
                    requiredState
                );

                // Update tracked state
                resource.currentState = requiredState;

                m_statistics.barrierCount++;
            }
        }
    }

    // Similar for outputs
    for (const auto &output: compiled.pass->GetOutputs()) {
        auto it = m_transientResources.find(output.name);
        if (it == m_transientResources.end()) {
            // External resource or doesn't exist
            continue;
        }

        TransientResource &resource = it->second;
        ResourceState currentState = resource.currentState;
        ResourceState requiredState = output.state;

        if (currentState != requiredState) {
            if (resource.type == TransientResource::Type::Texture) {
                m_commandList->TransitionTexture(
                    resource.texture,
                    currentState,
                    requiredState
                );
            } else {
                m_commandList->TransitionBuffer(
                    resource.buffer,
                    currentState,
                    requiredState
                );
            }
            // Update tracked state
            resource.currentState = requiredState;

            m_statistics.barrierCount++;
        }
    }
}

// ===== Execution =====

void RenderGraph::ExecutePass(const CompiledPass &compiledPass) {
    // Build context for this pass
    RenderPassContext context = BuildPassContext(compiledPass);

    // Execute the pass
    compiledPass.pass->Execute(context);
}

RenderPassContext RenderGraph::BuildPassContext(const CompiledPass &compiledPass) {
    RenderPassContext context;
    context.commandList = m_commandList;
    context.frameIndex = 0; // Would get from frame manager
    context.deltaTime = 0.016f; // Would get from timer

    // Populate input textures
    for (const auto &input: compiledPass.pass->GetInputs()) {
        if (input.type == RenderPassResource::Type::Texture) {
            Texture *texture = GetTexture(input.name);
            if (texture) {
                context.inputTextures.push_back(texture);
            }
        } else if (input.type == RenderPassResource::Type::Buffer) {
            Buffer *buffer = GetBuffer(input.name);
            if (buffer) {
                context.inputBuffers.push_back(buffer);
            }
        }
    }

    // Populate output textures
    for (const auto &output: compiledPass.pass->GetOutputs()) {
        if (output.type == RenderPassResource::Type::Texture) {
            Texture *texture = GetTexture(output.name);
            if (texture) {
                context.outputTextures.push_back(texture);
            }
        } else if (output.type == RenderPassResource::Type::Buffer) {
            Buffer *buffer = GetBuffer(output.name);
            if (buffer) {
                context.outputBuffers.push_back(buffer);
            }
        }
    }

    return context;
}

// ===== External Resources =====

void RenderGraph::RegisterExternalTexture(const std::string &name, Texture *texture) {
    m_externalTextures[name] = texture;
}

void RenderGraph::RegisterExternalBuffer(const std::string &name, Buffer *buffer) {
    m_externalBuffers[name] = buffer;
}

void RenderGraph::SetPresentTarget(const std::string &name) {
    m_presentTarget = name;
}

Texture *RenderGraph::GetTexture(const std::string &name) const {
    // Check external textures first
    auto extIt = m_externalTextures.find(name);
    if (extIt != m_externalTextures.end()) {
        return extIt->second;
    }

    // Check transient resources
    auto it = m_transientResources.find(name);
    if (it != m_transientResources.end()) {
        return it->second.texture;
    }

    return nullptr;
}

Buffer *RenderGraph::GetBuffer(const std::string &name) const {
    // Check external buffers first
    auto extIt = m_externalBuffers.find(name);
    if (extIt != m_externalBuffers.end()) {
        return extIt->second;
    }

    // Check transient resources
    auto it = m_transientResources.find(name);
    if (it != m_transientResources.end()) {
        return it->second.buffer;
    }

    return nullptr;
}

// ===== Helpers =====

bool RenderGraph::IsExternalResource(const std::string &name) const {
    return m_externalTextures.find(name) != m_externalTextures.end() ||
           m_externalBuffers.find(name) != m_externalBuffers.end();
}

TransientResource *RenderGraph::GetOrCreateResource(const std::string &name,
                                                    const RenderPassResource &desc) {
    auto it = m_transientResources.find(name);
    if (it != m_transientResources.end()) {
        return &it->second;
    }

    // Create new transient resource
    TransientResource resource;
    resource.name = name;
    resource.type = desc.type == RenderPassResource::Type::Texture
                        ? TransientResource::Type::Texture
                        : TransientResource::Type::Buffer;
    resource.isAlive = true;

    if (resource.type == TransientResource::Type::Texture) {
        resource.texture = CreateTransientTexture(desc);
        resource.width = desc.width;
        resource.height = desc.height;
    } else {
        resource.buffer = CreateTransientBuffer(desc);
        resource.size = desc.size;
    }

    m_transientResources[name] = resource;
    return &m_transientResources[name];
}

void RenderGraph::UpdateStatistics() {
    m_statistics.passCount = static_cast<uint32_t>(m_compiledPasses.size());
    m_statistics.transientResourceCount = static_cast<uint32_t>(m_transientResources.size());

    // Calculate memory usage
    uint64_t memoryUsed = 0;
    for (const auto &[name, resource]: m_transientResources) {
        if (resource.type == TransientResource::Type::Texture) {
            // Estimate texture memory (simplified)
            memoryUsed += static_cast<uint64_t>(resource.width) * resource.height * 4;
        } else {
            memoryUsed += resource.size;
        }
    }
    m_statistics.transientMemoryUsed = memoryUsed;
}

void RenderGraph::LogRenderGraph() {
    printf("\n===== RenderGraph =====\n");

    char msg[512];
    sprintf_s(msg, "Passes: %u\n", m_statistics.passCount);
    printf(msg);

    sprintf_s(msg, "Dependencies: %zu\n", m_dependencies.size());
    printf(msg);

    sprintf_s(msg, "Transient Resources: %u\n", m_statistics.transientResourceCount);
    printf(msg);

    sprintf_s(msg, "Memory Used: %.2f MB\n",
              m_statistics.transientMemoryUsed / (1024.0f * 1024.0f));
    printf(msg);

    sprintf_s(msg, "Compile Time: %.2f ms\n", m_statistics.compileTime);
    printf(msg);

    printf("\nPass Execution Order:\n");
    for (const auto &compiled: m_compiledPasses) {
        sprintf_s(msg, "  %u: %s%s\n",
                  compiled.index,
                  compiled.pass->GetName().c_str(),
                  compiled.pass->IsEnabled() ? "" : " (disabled)");
        printf(msg);
    }

    printf("\nResource Lifetimes:\n");
    for (const auto &[name, resource]: m_transientResources) {
        sprintf_s(msg, "  %s: [%u, %u]\n",
                  name.c_str(), resource.firstUse, resource.lastUse);
        printf(msg);
    }

    printf("========================\n\n");
}
