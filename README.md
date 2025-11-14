# Name_In_Progress Engine

# Features

## Rendering Features

- Render API abstraction (DX12/Vulkan)
- Render Graph
- Resource Manager
- Bindless
- Material Loading
- Mesh Rendering

## Engine Features

- Input System w/ Rebindable inputs & saving to disk

## In Progress For V1.0

## TODO

* RenderPasses into classes?
* Shader Hot Reloading
* Indirect Drawing
* GPU Draw command generation
* Tiled Lighting
* PBR
* Cascaded Shadow maps
* IBL
* PBR
* Wireframe Rendering
* Culling(CPU+GPU)
* Compute post processing
* FXAA
* CMAA
* SSAO
* Robust UI System
* Per object Rendering is handled by application, when implementing bindless maybe allow the renderer to have more
  control over the mesh and rendering. Also maybe SoA of model data from App->Renderer
* Current a ton of model info is sent in a uniform buffer, I think it would be better to upload to a storage buffer and
  index. This would increase the max model count from 256 to 16384 because in D3D12, uniform buffers are limited to
  65536 bytes and curently each model requires 256 bytes of data

# Projects

## Particle Simulation