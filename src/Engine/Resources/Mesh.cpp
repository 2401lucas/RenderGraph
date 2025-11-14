//
// Created by 2401Lucas on 2025-10-30.
//

#include "Mesh.h"

#include <stdexcept>

#include "../Rendering/Renderer.h"

Mesh::Mesh(Device *device, const MeshData &data)
    : m_device(device)
      , m_cpuData(data)
      , m_vertexCount(static_cast<uint32_t>(data.vertices.size()))
      , m_indexCount(static_cast<uint32_t>(data.indices.size())) {
    if (data.vertices.empty()) {
        throw std::runtime_error("Cannot create mesh with no vertices");
    }

    // Create vertex buffer
    size_t vertexBufferSize = data.vertices.size() * sizeof(Vertex);
    m_vertexBuffer = m_device->CreateBuffer({
        .size = vertexBufferSize,
        .stride = sizeof(Vertex),
        .usage = BufferUsage::Vertex,
        .memoryType = MemoryType::GPU
    });

    // Upload vertex data
    m_device->UploadBufferData(m_vertexBuffer, data.vertices.data(), vertexBufferSize);

    // Create index buffer
    if (!data.indices.empty()) {
        size_t indexBufferSize = data.indices.size() * sizeof(uint32_t);
        m_indexBuffer = m_device->CreateBuffer({
            .size = indexBufferSize,
            .stride = sizeof(uint32_t),
            .usage = BufferUsage::Index,
            .memoryType = MemoryType::GPU
        });

        m_device->UploadBufferData(m_indexBuffer, data.indices.data(), indexBufferSize);
    }

    m_gpuMemorySize = vertexBufferSize + (data.indices.size() * sizeof(uint32_t));
}

Mesh::~Mesh() {
    // Cleanup buffers
    if (m_vertexBuffer) {
        m_device->DestroyBuffer(m_vertexBuffer);
        m_vertexBuffer = nullptr;
    }

    if (m_indexBuffer) {
        m_device->DestroyBuffer(m_indexBuffer);
        m_indexBuffer = nullptr;
    }
}
