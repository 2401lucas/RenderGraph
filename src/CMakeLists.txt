find_package(glfw3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)

# Gather all .cpp and .h files recursively
file(GLOB_RECURSE SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)


add_executable(GPU_Particle_Sim  ${SOURCE_FILES})

# Add include directories (for internal headers)
target_include_directories(GPU_Particle_Sim PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/OS")
target_include_directories(GPU_Particle_Sim PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Rendering")

# Link DX12 libraries
target_link_libraries(GPU_Particle_Sim PRIVATE d3d12)
target_link_libraries(GPU_Particle_Sim PRIVATE dxgi)
target_link_libraries(GPU_Particle_Sim PRIVATE dxguid)
target_link_libraries(GPU_Particle_Sim PRIVATE d3dcompiler)
target_link_libraries(GPU_Particle_Sim PRIVATE Microsoft::DirectX-Headers)

target_link_libraries(GPU_Particle_Sim PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(GPU_Particle_Sim PUBLIC glfw)

target_compile_definitions(GPU_Particle_Sim PUBLIC _DEBUG)