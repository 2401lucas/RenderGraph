//
// Created by 2401Lucas on 2025-10-30.
//

#include "ParticleApp.h"

void ParticleApp::OnInitialize(Engine &engine) {
    auto &events = engine.GetEvents();


    events.EmitQueued(Events::APP_INITIALIZED);
}

void ParticleApp::Update(Engine &engine, float deltaTime) {
    auto &input = engine.GetInput();
    if (input.isKeyDown(GLFW_KEY_ESCAPE)) {
        engine.RequestExit();
    }
}

void ParticleApp::OnRender(Engine &engine) {
}

void ParticleApp::OnShutdown(Engine &engine) {
}

void ParticleApp::OnResize(Engine &engine, int width, int height) {
}

void ParticleApp::OnFocusChanged(Engine &engine, bool hasFocus) {
}
