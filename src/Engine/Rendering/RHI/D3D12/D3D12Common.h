//
// Created by 2401Lucas on 2025-11-04.
//

#ifndef GPU_PARTICLE_SIM_D3D12COMMON_H
#define GPU_PARTICLE_SIM_D3D12COMMON_H
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <format>
// --- Include Windows and DirectX safely ---
#include <Windows.h>
#include <wrl/client.h>
#include <DirectX/d3d12.h>
#include <DirectX/d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

// --- Common namespaces ---
using Microsoft::WRL::ComPtr;

inline std::string DXErrorToString(HRESULT hr) {
    switch (hr) {
        case DXGI_ERROR_DEVICE_HUNG:
            return "DXGI_ERROR_DEVICE_HUNG: The device stopped responding due to badly formed commands.";
        case DXGI_ERROR_DEVICE_REMOVED:
            return
                    "DXGI_ERROR_DEVICE_REMOVED: The video card has been physically removed or a driver upgrade occurred.";
        case DXGI_ERROR_DEVICE_RESET:
            return "DXGI_ERROR_DEVICE_RESET: The device failed due to a badly formed command or invalid state.";
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
            return "DXGI_ERROR_DRIVER_INTERNAL_ERROR: Internal driver error.";
        case DXGI_ERROR_INVALID_CALL:
            return "DXGI_ERROR_INVALID_CALL: The application made an invalid call.";
        case DXGI_ERROR_WAS_STILL_DRAWING:
            return "DXGI_ERROR_WAS_STILL_DRAWING: The GPU was still processing commands.";
        case E_OUTOFMEMORY:
            return "E_OUTOFMEMORY: Ran out of memory.";
        case E_INVALIDARG:
            return "E_INVALIDARG: One or more arguments are invalid.";
        case E_FAIL:
            return "E_FAIL: An unspecified error occurred.";
        default:
            return std::format("Unknown HRESULT: 0x{:08X}", static_cast<unsigned int>(hr));
    }
}

inline void DXCheckImpl(HRESULT hr, const char *expr, const char *file, int line) {
    if (FAILED(hr)) {
        std::string msg = DXErrorToString(hr);
        std::string output = std::format(
            "\n[DirectX Error]\n"
            "Expression: {}\n"
            "Result: {}\n"
            "File: {}\n"
            "Line: {}\n",
            expr, msg, file, line
        );
        OutputDebugStringA(output.c_str());
        MessageBoxA(nullptr, msg.c_str(), "DirectX Error", MB_ICONERROR | MB_OK);
        abort();
    }
}

#define DX_CHECK(expr) DXCheckImpl((expr), #expr, __FILE__, __LINE__)

#endif //GPU_PARTICLE_SIM_D3D12COMMON_H
