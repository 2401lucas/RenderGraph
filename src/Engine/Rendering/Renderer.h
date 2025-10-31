//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_RENDERER_H
#define GPU_PARTICLE_SIM_RENDERER_H

#include "../OS/Window/Window.h"

class Renderer {
public:
    Renderer(Window *);

    // ===== Renderer-Specific API =====
    void BeginFrame();

    void EndFrame();

    void Present();
};


#endif //GPU_PARTICLE_SIM_RENDERER_H
