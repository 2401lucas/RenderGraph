//
// Created by 2401Lucas on 2025-11-02.
//

#ifndef GPU_PARTICLE_SIM_RESOURCE_H
#define GPU_PARTICLE_SIM_RESOURCE_H

// enum class ResourceState {
//     Undefined, // Initial state
//     Common, // Generic state
//     RenderTarget, // Being written as render target
//     DepthWrite, // Being written as depth buffer
//     DepthRead, // Being read as depth (read-only depth)
//     ShaderResource, // Being read in shader
//     UnorderedAccess, // Being read/written in compute
//     CopySource, // Being copied from
//     CopyDest, // Being copied to
//     Present, // Ready for presentation
//     IndirectArgument, // Used as indirect draw args
// };

enum class ResourceAccess {
    None,
    Read,
    Write,
    ReadWrite
};

#endif //GPU_PARTICLE_SIM_RESOURCE_H
