//
// Created by 2401Lucas on 2025-11-06.
//

#include "D3D12BindlessDescriptorManager.h"
#include <stdexcept>

D3D12BindlessDescriptorManager::D3D12BindlessDescriptorManager(ID3D12Device *device)
    : m_device(device) {
    if (!device) {
        throw std::runtime_error("D3D12BindlessDescriptorManager: device is null");
    }

    m_resourceDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_samplerDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    Initialize();
}

D3D12BindlessDescriptorManager::~D3D12BindlessDescriptorManager() {
    m_allocatedResources.clear();
}

// Create SINGLE shader-visible heap for CBV/SRV/UAV
// Layout: [SRVs: 0-89,999] [UAVs: 90,000-99,999]
void D3D12BindlessDescriptorManager::Initialize() {
    D3D12_DESCRIPTOR_HEAP_DESC resourceHeapDesc = {};
    resourceHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    resourceHeapDesc.NumDescriptors = TOTAL_CBV_SRV_UAV_DESCRIPTORS;
    resourceHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    resourceHeapDesc.NodeMask = 0;

    DX_CHECK(m_device->CreateDescriptorHeap(&resourceHeapDesc, IID_PPV_ARGS(&m_resourceHeap)));
    m_resourceHeap->SetName(L"Bindless Resource Heap (CBV/SRV/UAV)");

    // Create shader-visible sampler heap
    D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
    samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerHeapDesc.NumDescriptors = MAX_BINDLESS_SAMPLERS;
    samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    samplerHeapDesc.NodeMask = 0;

    DX_CHECK(m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerHeap)));
    m_samplerHeap->SetName(L"Bindless Sampler Heap");

    // Create null SRV at index 0
    D3D12_CPU_DESCRIPTOR_HANDLE nullSRVHandle = m_resourceHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
    nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    nullSrvDesc.Texture2D.MipLevels = 1;
    m_device->CreateShaderResourceView(nullptr, &nullSrvDesc, nullSRVHandle);

    // Create null UAV at UAV start index
    D3D12_CPU_DESCRIPTOR_HANDLE nullUAVHandle = m_resourceHeap->GetCPUDescriptorHandleForHeapStart();
    nullUAVHandle.ptr += UAV_HEAP_START * m_resourceDescriptorSize;
    D3D12_UNORDERED_ACCESS_VIEW_DESC nullUavDesc = {};
    nullUavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    nullUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    m_device->CreateUnorderedAccessView(nullptr, nullptr, &nullUavDesc, nullUAVHandle);

    // Reserve index 0 for both
    m_nextSRVIndex = SRV_HEAP_START + 1;
    m_nextUAVIndex = UAV_HEAP_START + 1;
    m_nextSamplerIndex = 1;

    CreateDefaultSamplers();

    char msg[256];
    sprintf_s(
        msg,
        "Bindless System Initialized:\n  SRVs: indices %u-%u (%u slots)\n  UAVs: indices %u-%u (%u slots)\n  Samplers: %u slots\n",
        SRV_HEAP_START, SRV_HEAP_START + MAX_BINDLESS_SRVS - 1, MAX_BINDLESS_SRVS,
        UAV_HEAP_START, UAV_HEAP_START + MAX_BINDLESS_UAVS - 1, MAX_BINDLESS_UAVS,
        MAX_BINDLESS_SAMPLERS);
    OutputDebugStringA(msg);
}

void D3D12BindlessDescriptorManager::CreateDefaultSamplers() {
    D3D12_SAMPLER_DESC linearDesc = {};
    linearDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    linearDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearDesc.MipLODBias = 0.0f;
    linearDesc.MaxAnisotropy = 1;
    linearDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    linearDesc.MinLOD = 0.0f;
    linearDesc.MaxLOD = D3D12_FLOAT32_MAX;

    m_defaultLinearSampler = AllocateSampler(&linearDesc);

    D3D12_SAMPLER_DESC pointDesc = linearDesc;
    pointDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    m_defaultPointSampler = AllocateSampler(&pointDesc);

    D3D12_SAMPLER_DESC anisoDesc = linearDesc;
    anisoDesc.Filter = D3D12_FILTER_ANISOTROPIC;
    anisoDesc.MaxAnisotropy = 16;
    m_defaultAnisotropicSampler = AllocateSampler(&anisoDesc);
}

BindlessHandle D3D12BindlessDescriptorManager::AllocateSRV(
    ID3D12Resource *resource,
    const D3D12_SHADER_RESOURCE_VIEW_DESC *desc) {
    std::lock_guard<std::mutex> lock(m_resourceMutex);

    uint32_t index;
    if (!m_freeSRVIndices.empty()) {
        index = m_freeSRVIndices.front();
        m_freeSRVIndices.pop();
    } else {
        if (m_nextSRVIndex >= UAV_HEAP_START) {
            throw std::runtime_error("Bindless SRV heap exhausted");
        }
        index = m_nextSRVIndex++;
    }

    // Calculate CPU handle in the SRV section
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_resourceHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += index * m_resourceDescriptorSize;

    m_device->CreateShaderResourceView(resource, desc, cpuHandle);

    if (resource) {
        m_allocatedResources[index] = resource;
    }

    BindlessHandle handle;
    handle.index = index; // Absolute index in heap (0-89,999)
    handle.type = BindlessDescriptorType::SRV;
    return handle;
}

BindlessHandle D3D12BindlessDescriptorManager::AllocateUAV(
    ID3D12Resource *resource,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC *desc) {
    std::lock_guard<std::mutex> lock(m_resourceMutex);

    uint32_t index;
    if (!m_freeUAVIndices.empty()) {
        index = m_freeUAVIndices.front();
        m_freeUAVIndices.pop();
    } else {
        if (m_nextUAVIndex >= TOTAL_CBV_SRV_UAV_DESCRIPTORS) {
            throw std::runtime_error("Bindless UAV heap exhausted");
        }
        index = m_nextUAVIndex++;
    }

    // Calculate CPU handle in the UAV section (offset by UAV_HEAP_START)
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_resourceHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += index * m_resourceDescriptorSize;

    m_device->CreateUnorderedAccessView(resource, nullptr, desc, cpuHandle);

    if (resource) {
        m_allocatedResources[index] = resource;
    }

    BindlessHandle handle;
    handle.index = index - UAV_HEAP_START; // RELATIVE index for shader (0-9,999)
    handle.type = BindlessDescriptorType::UAV;
    return handle;
}

BindlessHandle D3D12BindlessDescriptorManager::AllocateCBV(
    const D3D12_CONSTANT_BUFFER_VIEW_DESC *desc) {
    std::lock_guard<std::mutex> lock(m_resourceMutex);

    uint32_t index;
    if (!m_freeSRVIndices.empty()) {
        index = m_freeSRVIndices.front();
        m_freeSRVIndices.pop();
    } else {
        if (m_nextSRVIndex >= UAV_HEAP_START) {
            throw std::runtime_error("Bindless SRV heap exhausted");
        }
        index = m_nextSRVIndex++;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_resourceHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += index * m_resourceDescriptorSize;

    m_device->CreateConstantBufferView(desc, cpuHandle);

    BindlessHandle handle;
    handle.index = index;
    handle.type = BindlessDescriptorType::CBV;
    return handle;
}

BindlessHandle D3D12BindlessDescriptorManager::AllocateSampler(const D3D12_SAMPLER_DESC *desc) {
    std::lock_guard<std::mutex> lock(m_samplerMutex);

    uint32_t index;
    if (!m_freeSamplerIndices.empty()) {
        index = m_freeSamplerIndices.front();
        m_freeSamplerIndices.pop();
    } else {
        if (m_nextSamplerIndex >= MAX_BINDLESS_SAMPLERS) {
            throw std::runtime_error("Bindless sampler heap exhausted");
        }
        index = m_nextSamplerIndex++;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_samplerHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += index * m_samplerDescriptorSize;

    m_device->CreateSampler(desc, cpuHandle);

    BindlessHandle handle;
    handle.index = index;
    handle.type = BindlessDescriptorType::Sampler;
    return handle;
}

void D3D12BindlessDescriptorManager::Free(BindlessHandle handle) {
    if (!handle.IsValid()) return;

    if (handle.type == BindlessDescriptorType::Sampler) {
        std::lock_guard<std::mutex> lock(m_samplerMutex);
        m_freeSamplerIndices.push(handle.index);
    } else if (handle.type == BindlessDescriptorType::UAV) {
        std::lock_guard<std::mutex> lock(m_resourceMutex);
        uint32_t absoluteIndex = handle.index + UAV_HEAP_START;
        m_freeUAVIndices.push(absoluteIndex);
        m_allocatedResources.erase(absoluteIndex);
    } else {
        std::lock_guard<std::mutex> lock(m_resourceMutex);
        m_freeSRVIndices.push(handle.index);
        m_allocatedResources.erase(handle.index);
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12BindlessDescriptorManager::GetSRVHeapStart() const {
    // SRVs start at beginning of heap
    return m_resourceHeap->GetGPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12BindlessDescriptorManager::GetUAVHeapStart() const {
    // UAVs start at offset UAV_HEAP_START
    D3D12_GPU_DESCRIPTOR_HANDLE handle = m_resourceHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += UAV_HEAP_START * m_resourceDescriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12BindlessDescriptorManager::GetSamplerHeapStart() const {
    return m_samplerHeap->GetGPUDescriptorHandleForHeapStart();
}

uint32_t D3D12BindlessDescriptorManager::GetDefaultLinearSampler() const {
    return m_defaultLinearSampler.index;
}

uint32_t D3D12BindlessDescriptorManager::GetDefaultPointSampler() const {
    return m_defaultPointSampler.index;
}

uint32_t D3D12BindlessDescriptorManager::GetDefaultAnisotropicSampler() const {
    return m_defaultAnisotropicSampler.index;
}
