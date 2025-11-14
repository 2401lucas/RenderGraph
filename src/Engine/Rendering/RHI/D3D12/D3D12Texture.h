//
// Created by 2401Lucas on 2025-11-04.
//

#ifndef GPU_PARTICLE_SIM_D3D12TEXTURE_H
#define GPU_PARTICLE_SIM_D3D12TEXTURE_H
#include "Rendering/RHI/Texture.h"

#include "D3D12Common.h"
#include "D3D12BindlessDescriptorManager.h"

class D3D12Texture : public Texture {
public:
    ComPtr<ID3D12Resource> resource;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

    // Bindless handles
    BindlessHandle srvHandle; // For reading in shaders
    BindlessHandle uavHandle; // For RWTexture access

    uint32_t GetBindlessIndex() const override {
        // Prefer SRV, fall back to UAV
        if (srvHandle.IsValid()) return srvHandle.index;
        if (uavHandle.IsValid()) return uavHandle.index;
        return INVALID_DESCRIPTOR_INDEX;
    }

    uint32_t GetBindlessUAVIndex() const {
        return uavHandle.IsValid() ? uavHandle.index : INVALID_DESCRIPTOR_INDEX;
    }
};


#endif //GPU_PARTICLE_SIM_D3D12TEXTURE_H
