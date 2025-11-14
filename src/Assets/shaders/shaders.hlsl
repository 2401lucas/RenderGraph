// Currently this is setup for DX12. When the Vulkan Backend is implemented I am unsure if we should try to use one Shader for both or a shader DX12/Vulkan.


// ===== CONSTANT BUFFERS =====

// Per-frame data (bound to b0, root parameter 4)
cbuffer PerFrameData : register(b0) {
    float4x4 viewProjection;
    float3 cameraPosition;
    float time;
    float3 lightDirection;
    float lightIntensity;
    float3 lightColor;
    uint frameIndex;
    uint padding[44]; // Align to 256 bytes
};

// Per-object data (bound to b1, root parameter 5)
cbuffer PerObjectData : register(b1) {
    float4x4 worldMatrix;
    float4x4 normalMatrix;
    uint albedoTextureIndex;
    uint normalTextureIndex;
    uint metallicRoughnessIndex;
    uint emissiveTextureIndex;
    float4 albedoFactor;
    float metallicFactor;
    float roughnessFactor;
    uint materialFlags;
    uint objectID;
    uint padding2[24]; // Align to 256 bytes
};

// ===== BINDLESS RESOURCES =====

// Unbounded texture array (space0 for SRVs)
Texture2D<float4> bindlessTextures[] : register(t0, space0);

// Unbounded sampler array
SamplerState bindlessSamplers[] : register(s0, space0);

// ===== VERTEX SHADER =====

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    
    // Transform position
    float4 worldPos = mul(worldMatrix, float4(input.position, 1.0));
    output.position = mul(viewProjection, worldPos);
    output.worldPos = worldPos.xyz;
    
    // Transform normal and tangent
    output.normal = normalize(mul((float3x3)normalMatrix, input.normal));
    output.tangent = normalize(mul((float3x3)normalMatrix, input.tangent));
    
    // Calculate bitangent
    output.bitangent = cross(output.normal, output.tangent);
    
    // Pass through texture coordinates
    output.texCoord = input.texCoord;
    
    return output;
}

// ===== PIXEL SHADER =====

// Simple Phong lighting helper
float3 CalculatePhongLighting(
    float3 normal,
    float3 viewDir,
    float3 albedo,
    float metallic,
    float roughness)
{
    // Normalize vectors
    float3 N = normalize(normal);
    float3 V = normalize(viewDir);
    float3 L = normalize(-lightDirection);
    float3 H = normalize(V + L);
    
    // Diffuse
    float NdotL = max(dot(N, L), 0.0);
    float3 diffuse = albedo * lightColor * NdotL;

    // Specular (simplified)
    float shininess = (1.0 - roughness) * 128.0;
    float spec = pow(max(dot(N, H), 0.0), shininess);
    float3 specular = lightColor * spec * metallic;

    // Ambient
    float3 ambient = albedo * 0.1;
    
    return (ambient + diffuse + specular) * lightIntensity;
}

float4 PSMain(PSInput input) : SV_TARGET {
    // Sample textures using bindless indices
    // Linear sampler at index 1 (created by BindlessDescriptorManager)
    float4 albedoSample = bindlessTextures[albedoTextureIndex].Sample(bindlessSamplers[1], input.texCoord);
    float3 albedo = albedoSample.rgb * albedoFactor.rgb;
    
    // Sample normal map if available (index 0 means no texture)
    float3 normal = input.normal;
    if (normalTextureIndex != 0) {
        float3 normalSample = bindlessTextures[normalTextureIndex].Sample(bindlessSamplers[1], input.texCoord).rgb;
        normalSample = normalSample * 2.0 - 1.0; // Convert from [0,1] to [-1,1]
        
        // Transform normal from tangent space to world space
        float3 T = normalize(input.tangent);
        float3 B = normalize(input.bitangent);
        float3 N = normalize(input.normal);
        float3x3 TBN = float3x3(T, B, N);
        normal = normalize(mul(normalSample, TBN));
    }
    
    // Sample metallic/roughness if available
    float metallic = metallicFactor;
    float roughness = roughnessFactor;
    if (metallicRoughnessIndex != 0) {
        float2 mr = bindlessTextures[metallicRoughnessIndex].Sample(bindlessSamplers[1], input.texCoord).rg;
        metallic *= mr.r;
        roughness *= mr.g;
    }
    
    // Calculate lighting
    float3 viewDir = cameraPosition - input.worldPos;
    float3 color = CalculatePhongLighting(normal, viewDir, albedo, metallic, roughness);
    
    // Add emissive if available
    if (emissiveTextureIndex != 0) {
        float3 emissive = bindlessTextures[emissiveTextureIndex].Sample(bindlessSamplers[1], input.texCoord).rgb;
        color += emissive;
    }
    
    // Simple tone mapping
    color = color / (color + 1.0);
    
    // Gamma correction
    color = pow(color, 1.0 / 2.2);
    
    return float4(color, 1.0);
}

// ===== SIMPLE SHADER (No normal mapping) =====

float4 PSMainSimple(PSInput input) : SV_TARGET {
    // Just sample albedo texture
    float4 albedo = bindlessTextures[albedoTextureIndex].Sample(bindlessSamplers[1], input.texCoord);
    albedo *= albedoFactor;
    
    // Simple diffuse lighting
    float3 N = normalize(input.normal);
    float3 L = normalize(-lightDirection);
    float NdotL = max(dot(N, L), 0.0);
    
    float3 color = albedo.rgb * lightColor * NdotL * lightIntensity;
    color += albedo.rgb * 0.1; // Ambient
    
    return float4(color, albedo.a);
}