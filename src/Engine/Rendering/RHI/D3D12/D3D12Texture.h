//
// Created by 2401Lucas on 2025-11-04.
//

#ifndef GPU_PARTICLE_SIM_D3D12TEXTURE_H
#define GPU_PARTICLE_SIM_D3D12TEXTURE_H
#include "Rendering/RHI/Texture.h"

#include "D3D12Common.h"


class D3D12Texture : public Texture {
public:
    ComPtr<ID3D12Resource> resource;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
};


#endif //GPU_PARTICLE_SIM_D3D12TEXTURE_H
