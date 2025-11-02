//
// Created by 2401Lucas on 2025-10-31.
//

#include "D3D12Device.h"

#include <format>

inline std::string DXErrorToString(HRESULT hr) {
    switch (hr) {
        case DXGI_ERROR_DEVICE_HUNG: return
                    "DXGI_ERROR_DEVICE_HUNG: The device stopped responding due to badly formed commands.";
        case DXGI_ERROR_DEVICE_REMOVED: return
                    "DXGI_ERROR_DEVICE_REMOVED: The video card has been physically removed or a driver upgrade occurred.";
        case DXGI_ERROR_DEVICE_RESET: return
                    "DXGI_ERROR_DEVICE_RESET: The device failed due to a badly formed command or invalid state.";
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR: Internal driver error.";
        case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL: The application made an invalid call.";
        case DXGI_ERROR_WAS_STILL_DRAWING: return
                    "DXGI_ERROR_WAS_STILL_DRAWING: The GPU was still processing commands.";
        case E_OUTOFMEMORY: return "E_OUTOFMEMORY: Ran out of memory.";
        case E_INVALIDARG: return "E_INVALIDARG: One or more arguments are invalid.";
        case E_FAIL: return "E_FAIL: An unspecified error occurred.";
    }
    return "Unknown error.";
}

// Logs a DirectX failure with file and line info
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

ComPtr<IDXGIAdapter4> SelectAdapter(ComPtr<IDXGIFactory6> factory, uint32_t preferredIndex) {
    ComPtr<IDXGIAdapter4> chosenAdapter;
    ComPtr<IDXGIAdapter1> adapter1;
    UINT index = 0;
    // Prefer high-performance adapters (discrete GPU)
    for (UINT i = 0; SUCCEEDED(
             factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter1))); ++
         i) {
        ComPtr<IDXGIAdapter4> adapter4;
        DX_CHECK(adapter1.As(&adapter4));

        DXGI_ADAPTER_DESC3 desc;
        adapter4->GetDesc3(&desc);

        // skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
            continue;
        }

        // Check D3D12 feature support quickly
        if (SUCCEEDED(D3D12CreateDevice(adapter4.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
            if (index == preferredIndex) {
                chosenAdapter = adapter4;
                break;
            }
            ++index;
            // fallback: if we exhaust preferredIndex, first compatible adapter will be used
            if (!chosenAdapter) {
                chosenAdapter = adapter4;
            }
        }
    }

    // If no discrete/high-performance adapter found, try the default adapter
    if (!chosenAdapter) {
        ComPtr<IDXGIAdapter4> defaultAdapter;
        ComPtr<IDXGIAdapter1> ad1;
        if (SUCCEEDED(factory->EnumAdapters1(0, &ad1))) {
            DX_CHECK(ad1.As(&defaultAdapter));
            chosenAdapter = defaultAdapter;
        }
    }

    return chosenAdapter;
}

D3D12Device::D3D12Device(const DeviceCreateInfo &info) {
    UINT flags = 0u;
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugInterface;
    if (info.enableDebugLayer) {
        flags |= DXGI_CREATE_FACTORY_DEBUG;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
            debugInterface->EnableDebugLayer();
            // Optionally enable additional debug options (GPU-based validation) if desired
            if (info.enableGPUValidation) {
                ComPtr<ID3D12Debug1> debug1;
                if (SUCCEEDED(debugInterface.As(&debug1))) {
                    debug1->SetEnableGPUBasedValidation(TRUE);
                }
            }
        }
    }
#endif

    ComPtr<IDXGIFactory6> factory;
    DX_CHECK(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));

    m_adapter = SelectAdapter(factory, m_desc.preferredAdapterIndex);
    if (!m_adapter) {
        throw std::runtime_error("No suitable DXGI adapter found.");
    }

    DX_CHECK(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));


    // Optionally enable debug info queue settings
#if defined(_DEBUG)
    if (m_desc.enableDebugLayer) {
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(m_device.As(&infoQueue))) {
            // Break on errors
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

            // D3D12_MESSAGE_ID denyIds[] = { ... };
            // D3D12_INFO_QUEUE_FILTER filter = {};
            // filter.DenyList.NumIDs = _countof(denyIds);
            // filter.DenyList.pIDList = denyIds;
            // infoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    // Create a default command queue
    D3D12_COMMAND_QUEUE_DESC qdesc = {};
    qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    qdesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    DX_CHECK(m_device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&m_commandQueue)));
}

D3D12Device::~D3D12Device() {
}

CommandList *D3D12Device::CreateCommandList() {
    return nullptr;
}

Swapchain *D3D12Device::CreateSwapchain(void *windowHandle, uint32_t width, uint32_t height) {
    return nullptr;
}

void D3D12Device::DestroyBuffer(Buffer *buffer) {
}

void D3D12Device::DestroyTexture(Texture *texture) {
}

void D3D12Device::DestroyShader(Shader *shader) {
}

void D3D12Device::DestroyPipeline(Pipeline *pipeline) {
}

bool D3D12Device::SupportsRayTracing() const {
    return false;
}

bool D3D12Device::SupportsMeshShaders() const {
    return false;
}

uint64_t D3D12Device::GetVideoMemoryBudget() const {
    return 0;
}

void D3D12Device::WaitIdle() {
}

CommandQueue *D3D12Device::CreateCommandQueue() {
    return nullptr;
}

Buffer *D3D12Device::CreateBuffer(const BufferCreateInfo &desc) {
    return nullptr;
}

Texture *D3D12Device::CreateTexture(const TextureCreateInfo &desc) {
    return nullptr;
}

Shader *D3D12Device::CreateShader(const std::string &path) {
    return nullptr;
}

Pipeline *D3D12Device::CreatePipeline(const PipelineDesc &desc) {
    return nullptr;
}

void D3D12Device::UploadBufferData(Buffer *buffer, const void *data, size_t size) {
}

void D3D12Device::UploadTextureData(Texture *texture, const void *data, size_t size) {
}
