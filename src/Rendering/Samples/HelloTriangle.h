//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_HELLOTRIANGLE_H
#define GPU_PARTICLE_SIM_HELLOTRIANGLE_H

#include "Window/Window.h"

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <directxmath.h>
#include <d3dcompiler.h>
#include <dxguids/dxguids.h>

#include <dxgi1_6.h>
#include <filesystem>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;


class HelloTriangle {
public:
    HelloTriangle(int width, int height);

    ~HelloTriangle();

    void Start();

    void Update();

    void Render();

private:
    void SetupWindow(int width, int height);

    void SetupD3D();

    std::wstring GetAssetFullPath(const std::wstring str) {
        char buffer[MAX_PATH];
        GetModuleFileNameA(nullptr, buffer, MAX_PATH);

        std::filesystem::path exePath(buffer);
        std::filesystem::path basePath = exePath.parent_path();

        std::filesystem::path fullPath = basePath / str;
        return fullPath;
    }

    void SetupAssets();

    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    static constexpr UINT FrameCount = 2;

    std::vector<Window> m_Window;

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    UINT m_rtvDescriptorSize;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;
};


#endif //GPU_PARTICLE_SIM_HELLOTRIANGLE_H
