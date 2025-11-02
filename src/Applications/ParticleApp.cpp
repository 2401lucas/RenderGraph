//
// Created by 2401Lucas on 2025-10-30.
//

#include "ParticleApp.h"

void ParticleApp::OnInitialize(Engine &engine) {
    auto &events = engine.GetEvents();
    events.EmitQueued(Events::APP_INITIALIZED);

    auto &resources = engine.GetResourceManager();
    entity.meshHandle = resources.LoadMesh("assets/cube.obj");
    entity.transform = Transform::Translation(0, 0, 0);

    auto &renderer = engine.GetRenderer();
    m_camera = std::make_unique<Camera>(Transform::Translation(0, 0, 5), (1280.f / 720.f), 60, 0.0001f, 500.0f);
}

void ParticleApp::Update(Engine &engine, float deltaTime) {
    auto &input = engine.GetInput();
    if (input.isKeyDown(GLFW_KEY_ESCAPE)) {
        engine.RequestExit();
    }
}

void ParticleApp::OnRender(Engine &engine) {
    auto &renderer = engine.GetRenderer();
    auto &resources = engine.GetResourceManager();

    renderer.BeginFrame();
    renderer.SetCamera(*m_camera);

    RenderInfo info{
        .mesh = resources.GetMesh(entity.meshHandle),
        .material = resources.GetMaterial(entity.materialHandle), // TODO: CREATE MATERIAL
        .transform = entity.transform,
        .castsShadows = true
    };
    renderer.Submit(info);

    renderer.EndFrame();
}

void ParticleApp::OnShutdown(Engine &engine) {
    auto &events = engine.GetEvents();
    events.EmitQueued(Events::APP_SHUTDOWN);
}

void ParticleApp::OnResize(Engine &engine, int width, int height) {
}

void ParticleApp::OnFocusChanged(Engine &engine, bool hasFocus) {
}
