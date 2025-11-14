//
// Created by 2401Lucas on 2025-10-30.
//

#include "ParticleApp.h"

void ParticleApp::OnInitialize(Engine &engine) {
    auto &events = engine.GetEvents();
    events.EmitQueued(Events::APP_INITIALIZED);

    auto &resources = engine.GetResourceManager();
    entity.meshHandle = resources.LoadMesh("assets/Cube/cube.obj");
    entity.materialHandle = resources.LoadMaterial("assets/cube_BaseColor.png");
    entity.transform.SetPosition({0, 0, 0});
    entity.transform.SetRotation({45, 45, 0});
    entity.transform.SetScale({1, 1, 1});

    auto &renderer = engine.GetRenderer();
    m_camera = std::make_unique<Camera>(Transform({0, 0, 5}, {0, 0, 0}, {1, 1, 1}), (1280.f / 720.f), 60, 0.0001f,
                                        500.0f);

    renderer.SetCamera(*m_camera);

    auto &input = engine.GetInput();
    input.registerAxis("Y Axis", KeyboardBinding(GLFW_KEY_W), KeyboardBinding(GLFW_KEY_S));
    input.registerAxis("X Axis", KeyboardBinding(GLFW_KEY_A), KeyboardBinding(GLFW_KEY_D));
}

void ParticleApp::Update(Engine &engine, float deltaTime) {
    auto &input = engine.GetInput();
    if (input.isKeyDown(GLFW_KEY_ESCAPE)) {
        engine.RequestExit();
    }

    auto xMov = input.getAxis("X Axis");
    auto yMov = input.getAxis("Y Axis");

    auto &camera_tranform = m_camera->GetTransform();
    auto &pos = camera_tranform.GetPosition();
    camera_tranform.SetPosition({pos.x + (xMov * m_speed * deltaTime), pos.y, pos.z + (yMov * m_speed * deltaTime)});
}

void ParticleApp::OnRender(Engine &engine) {
    auto &renderer = engine.GetRenderer();
    auto &resources = engine.GetResourceManager();

    renderer.BeginFrame();
    entity.transform.SetPosition({0, 0, 0});
    entity.transform.SetRotation({45, 45, 0});
    entity.transform.SetScale({1, 1, 1});

    RenderInfo info{
        .mesh = entity.meshHandle,
        .material = entity.materialHandle,
        .transform = entity.transform,
        .castsShadows = true
    };
    renderer.Submit(info);

    entity.transform.SetPosition({3, 0, 3});
    entity.transform.SetRotation({0, 0, 0});
    entity.transform.SetScale({1, 1, 1});
    info = RenderInfo{
        .mesh = entity.meshHandle,
        .material = entity.materialHandle,
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
