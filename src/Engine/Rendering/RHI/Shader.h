//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_SHADER_H
#define GPU_PARTICLE_SIM_SHADER_H


#include <cstdint>
#include <string>
#include <vector>

enum class ShaderStage {
    Vertex,
    Pixel,
    Compute,
    Geometry,
    Hull,
    Domain
};

struct ShaderCode {
    std::vector<uint8_t> bytecode;
    std::string entryPoint = "main";
    ShaderStage stage;
};

class Shader {
public:
    Shader(const std::string &path) : m_path(path) {

    }

    virtual ~Shader() = default;

    // Compilation
    virtual bool Compile() = 0;

    virtual bool Recompile() = 0;

    // Access compiled bytecode
    const ShaderCode &GetVertexShader() const { return m_vertexShader; }
    const ShaderCode &GetPixelShader() const { return m_pixelShader; }
    const ShaderCode &GetComputeShader() const { return m_computeShader; }

    bool HasVertexShader() const { return !m_vertexShader.bytecode.empty(); }
    bool HasPixelShader() const { return !m_pixelShader.bytecode.empty(); }
    bool HasComputeShader() const { return !m_computeShader.bytecode.empty(); }

    const std::string &GetPath() const { return m_path; }

protected:
    std::string m_path;
    ShaderCode m_vertexShader;
    ShaderCode m_pixelShader;
    ShaderCode m_computeShader;
};

#endif //GPU_PARTICLE_SIM_SHADER_H
