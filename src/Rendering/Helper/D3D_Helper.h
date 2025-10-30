//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_D3D_HELPER_H
#define GPU_PARTICLE_SIM_D3D_HELPER_H
#include <directx/d3d12.h>

#include <Windows.h>
#include <comdef.h>
#include <string>
#include <iostream>
#include <sstream>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

inline void GetHardwareAdapter(IDXGIFactory1 *pFactory, IDXGIAdapter1 **ppAdapter) {
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6)))) {
        for (
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                IID_PPV_ARGS(&adapter)));
            ++adapterIndex) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
    }

    if (adapter.Get() == nullptr) {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
    }

    *ppAdapter = adapter.Detach();
}

inline void WaitForGPUIdle(ComPtr<ID3D12Device>& device, ComPtr<ID3D12CommandQueue>& queue)
{
    // Create a fence for synchronization
    ComPtr<ID3D12Fence> fence;
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

    HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!eventHandle)
        throw std::runtime_error("Failed to create synchronization event.");

    // Signal and wait
    const UINT64 fenceValue = 1;
    queue->Signal(fence.Get(), fenceValue);

    if (fence->GetCompletedValue() < fenceValue)
    {
        fence->SetEventOnCompletion(fenceValue, eventHandle);
        WaitForSingleObject(eventHandle, INFINITE);
    }

    CloseHandle(eventHandle);
}

inline std::string DXErrorToString(HRESULT hr)
{
    switch (hr)
    {
        case DXGI_ERROR_DEVICE_HUNG:           return "DXGI_ERROR_DEVICE_HUNG: The device stopped responding due to badly formed commands.";
        case DXGI_ERROR_DEVICE_REMOVED:        return "DXGI_ERROR_DEVICE_REMOVED: The video card has been physically removed or a driver upgrade occurred.";
        case DXGI_ERROR_DEVICE_RESET:          return "DXGI_ERROR_DEVICE_RESET: The device failed due to a badly formed command or invalid state.";
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR: Internal driver error.";
        case DXGI_ERROR_INVALID_CALL:          return "DXGI_ERROR_INVALID_CALL: The application made an invalid call.";
        case DXGI_ERROR_WAS_STILL_DRAWING:     return "DXGI_ERROR_WAS_STILL_DRAWING: The GPU was still processing commands.";
        case E_OUTOFMEMORY:                    return "E_OUTOFMEMORY: Ran out of memory.";
        case E_INVALIDARG:                     return "E_INVALIDARG: One or more arguments are invalid.";
        case E_FAIL:                           return "E_FAIL: An unspecified error occurred.";
        default:
        {
            // Use _com_error to get system message text
            _com_error err(hr);
            std::wstringstream ss;
            ss << L"Unknown HRESULT (0x" << std::hex << hr << L"): " << err.ErrorMessage();
            std::wstring wmsg = ss.str();
            return std::string(wmsg.begin(), wmsg.end());
        }
    }
}

// Logs a DirectX failure with file and line info
inline void DXCheckImpl(HRESULT hr, const char* expr, const char* file, int line)
{
    if (FAILED(hr))
    {
        std::string msg = DXErrorToString(hr);
        std::cerr << "\n[DirectX Error]\n"
                  << "Expression: " << expr << "\n"
                  << "Result: " << msg << "\n"
                  << "File: " << file << "\n"
                  << "Line: " << line << "\n" << std::endl;

        MessageBoxA(nullptr, msg.c_str(), "DirectX Error", MB_ICONERROR | MB_OK);
        abort();
    }
}

#define DX_CHECK(expr) DXCheckImpl((expr), #expr, __FILE__, __LINE__)

#endif //GPU_PARTICLE_SIM_D3D_HELPER_H
