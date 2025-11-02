//
// Created by 2401Lucas on 2025-11-01.
//

#ifndef GPU_PARTICLE_SIM_D3D12SHADER_H
#define GPU_PARTICLE_SIM_D3D12SHADER_H
#include "Rendering/RHI/Shader.h"

class D3D12Shader : public Shader {
public:
    D3D12Shader(const std::string &path);
    ~D3D12Shader() override;

    bool Compile() override;

    bool Recompile() override;
};


#endif //GPU_PARTICLE_SIM_D3D12SHADER_H
