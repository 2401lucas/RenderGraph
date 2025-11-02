//
// Created by 2401Lucas on 2025-11-01.
//

#include "D3D12Shader.h"
#include <d3dcompiler.h>
#include <fstream>

#include <wrl/client.h>
using namespace Microsoft::WRL;

D3D12Shader::D3D12Shader(const std::string &path) : Shader(path) {
}

D3D12Shader::~D3D12Shader() {

}

bool D3D12Shader::Compile() {
    // Read shader source
    std::ifstream file(m_path);
    if (!file.is_open()) {
        return false;
    }

    std::string source((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

    // Compile vertex shader
    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;

    HRESULT hr = D3DCompile(
        source.c_str(),
        source.size(),
        m_path.c_str(),
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &vsBlob,
        &errorBlob
    );

    if (SUCCEEDED(hr) && vsBlob) {
        m_vertexShader.bytecode.resize(vsBlob->GetBufferSize());
        memcpy(m_vertexShader.bytecode.data(), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
        m_vertexShader.stage = ShaderStage::Vertex;
    }

    // Compile pixel shader
    hr = D3DCompile(
        source.c_str(),
        source.size(),
        m_path.c_str(),
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &psBlob,
        &errorBlob
    );

    if (SUCCEEDED(hr) && psBlob) {
        m_pixelShader.bytecode.resize(psBlob->GetBufferSize());
        memcpy(m_pixelShader.bytecode.data(), psBlob->GetBufferPointer(), psBlob->GetBufferSize());
        m_pixelShader.stage = ShaderStage::Pixel;
    }

    return true;
}

bool D3D12Shader::Recompile() {
    // Clear old bytecode
    m_vertexShader.bytecode.clear();
    m_pixelShader.bytecode.clear();
    m_computeShader.bytecode.clear();

    // Recompile
    return Compile();
}
