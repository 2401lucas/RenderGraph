//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_PARTICLEAPP_H
#define GPU_PARTICLE_SIM_PARTICLEAPP_H


#include "../Engine/Core/Application.h"
#include "../Engine/Core/EventSystem.h"
#include <memory>
#include <vector>

// Forward declarations
class ParticleSystem;
class Camera;

class ParticleApp : public Application {
public:
    ParticleApp() {
    }

    ~ParticleApp() override {
    }

    // ===== Application Interface Implementation =====

    void OnInitialize(Engine &engine) override;

    void Update(Engine &engine, float deltaTime) override;

    void OnRender(Engine &engine) override;

    void OnShutdown(Engine &engine) override;

    // Optional overrides
    void OnResize(Engine &engine, int width, int height) override;

    void OnFocusChanged(Engine &engine, bool hasFocus) override;

    // Configuration
    const char *GetWindowTitle() const override { return "GPU Particle Simulation"; }
    bool GetStartFullscreen() const override { return false; }
    bool GetVSyncEnabled() const override { return true; }

private:
    // Application settings
    bool m_isPaused = false;
    bool m_showDebugInfo = false;
    float m_timeScale = 1.0f;
};


#endif //GPU_PARTICLE_SIM_PARTICLEAPP_H
