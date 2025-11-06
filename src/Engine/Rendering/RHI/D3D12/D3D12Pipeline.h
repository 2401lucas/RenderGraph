//
// Created by 2401Lucas on 2025-11-04.
//

#ifndef GPU_PARTICLE_SIM_D3D12PIPELINE_H
#define GPU_PARTICLE_SIM_D3D12PIPELINE_H
#include "Rendering/RHI/Pipeline.h"

#include "D3D12Common.h"


class D3D12Pipeline : public Pipeline {
public:
    ~D3D12Pipeline() = default;

    void *GetNativeHandle() const override;

    ComPtr<ID3D12PipelineState> pso;
    ComPtr<ID3D12RootSignature> rootSignature;
};


#endif //GPU_PARTICLE_SIM_D3D12PIPELINE_H
