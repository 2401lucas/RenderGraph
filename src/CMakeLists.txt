find_package(glfw3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)

# Gather all .cpp and .h files recursively
file(GLOB_RECURSE SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)


add_executable(GPU_Particle_Sim ${SOURCE_FILES}
        Engine/Resources/MeshLoader.cpp
        Engine/Resources/MeshLoader.h
        Engine/Resources/ResourceManager.cpp
        Engine/Resources/ResourceManager.h
        Engine/Resources/TextureLoader.cpp
        Engine/Resources/TextureLoader.h
        Engine/Resources/Mesh.cpp
        Engine/Resources/Mesh.h
        Engine/Resources/Material.cpp
        Engine/Resources/Material.h
        Engine/Rendering/RHI/Buffer.h
        Engine/Resources/ResourceHandle.h
        Engine/Rendering/RHI/Device.h
        Engine/Rendering/RHI/Texture.h
        Engine/Rendering/RHI/CommandList.h
        Engine/Rendering/RHI/Pipeline.h
        Engine/Rendering/RHI/CommandQueue.h
        Engine/Rendering/RHI/D3D12/D3D12Device.h
        Engine/Rendering/RHI/Device.cpp
        Engine/Rendering/RHI/D3D12/D3D12Device.cpp
        Engine/Rendering/RHI/Swapchain.h
        Engine/Rendering/RHI/Fence.h
        Engine/Rendering/RHI/D3D12/D3D12Shader.cpp
        Engine/Rendering/RHI/D3D12/D3D12Shader.h
        Engine/Core/Camera.cpp
        Engine/Core/Camera.h
        Engine/Core/Transform.cpp
        Engine/Core/Transform.h
        Engine/Rendering/RHI/Resource.h
)

# Add include directories (for internal headers)
target_include_directories(GPU_Particle_Sim PRIVATE "Engine/")

# Link DX12 libraries
target_link_libraries(GPU_Particle_Sim PRIVATE d3d12)
target_link_libraries(GPU_Particle_Sim PRIVATE dxgi)
target_link_libraries(GPU_Particle_Sim PRIVATE dxguid)
target_link_libraries(GPU_Particle_Sim PRIVATE d3dcompiler)
target_link_libraries(GPU_Particle_Sim PRIVATE Microsoft::DirectX-Headers)

target_link_libraries(GPU_Particle_Sim PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(GPU_Particle_Sim PUBLIC glfw)

target_compile_definitions(GPU_Particle_Sim PUBLIC DEBUG_EVENTS)
target_compile_definitions(GPU_Particle_Sim PUBLIC RHI_D3D12)
# target_compile_definitions(GPU_Particle_Sim PUBLIC RHI_VULKAN)