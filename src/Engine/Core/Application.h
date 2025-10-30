//
// Created by 2401Lucas on 2025-10-30.
//

#ifndef GPU_PARTICLE_SIM_APPLICATION_H
#define GPU_PARTICLE_SIM_APPLICATION_H
class Engine;
class Renderer;
class InputSystem;
class AudioSystem;
class PhysicsSystem;
class EventSystem;

/// <summary>
/// Base application interface that all applications must inherit from.
/// The engine calls these methods at appropriate times during the frame.
/// Applications override these to implement their specific behavior.
/// </summary>
class Application {
public:
    virtual ~Application() = default;

    // ===== Lifecycle Methods =====

    /// <summary>
    /// Called once after the engine has initialized all subsystems.
    /// Use this to load resources, create initial objects, setup scene, etc.
    /// </summary>
    /// <param name="engine">Reference to the engine for accessing subsystems</param>
    virtual void OnInitialize(Engine &engine) = 0;

    /// <summary>
    /// Called every frame before rendering.
    /// Use this for game logic, physics updates, input handling, etc.
    /// </summary>
    /// <param name="engine">Reference to the engine</param>
    /// <param name="deltaTime">Time elapsed since last frame in seconds</param>
    virtual void OnUpdate(Engine &engine, float deltaTime) = 0;

    /// <summary>
    /// Called every frame for rendering.
    /// Submit rendering commands, update particle systems, etc.
    /// </summary>
    /// <param name="engine">Reference to the engine</param>
    virtual void OnRender(Engine &engine) = 0;

    /// <summary>
    /// Called once before the engine shuts down.
    /// Use this to cleanup application-specific resources.
    /// </summary>
    /// <param name="engine">Reference to the engine</param>
    virtual void OnShutdown(Engine &engine) = 0;

    // ===== Optional Override Methods =====

    /// <summary>
    /// Called when the window is resized.
    /// Override to handle resolution changes, update camera aspect ratio, etc.
    /// </summary>
    virtual void OnResize(Engine &engine, int width, int height) {
    }

    /// <summary>
    /// Called when the application loses/gains focus.
    /// Override to pause/resume gameplay, mute audio, etc.
    /// </summary>
    virtual void OnFocusChanged(Engine &engine, bool hasFocus) {
    }

    // ===== Configuration Methods =====

    /// <summary>
    /// Returns the window title to display.
    /// </summary>
    virtual const char *GetWindowTitle() const { return "Application"; }

    /// <summary>
    /// Returns the initial window width.
    /// </summary>
    virtual int GetWindowWidth() const { return 1280; }

    /// <summary>
    /// Returns the initial window height.
    /// </summary>
    virtual int GetWindowHeight() const { return 720; }

    /// <summary>
    /// Returns whether the window should start in fullscreen mode.
    /// </summary>
    virtual bool GetStartFullscreen() const { return false; }

    /// <summary>
    /// Returns whether VSync should be enabled.
    /// </summary>
    virtual bool GetVSyncEnabled() const { return true; }

protected:
    // Helper methods for derived classes to access subsystems easily
    // These avoid having to call engine.GetRenderer() repeatedly

    inline Renderer &GetRenderer(Engine &engine);

    inline InputSystem &GetInput(Engine &engine);

    inline AudioSystem &GetAudio(Engine &engine);

    inline PhysicsSystem &GetPhysics(Engine &engine);

    inline EventSystem &GetEvents(Engine &engine);
};
#endif //GPU_PARTICLE_SIM_APPLICATION_H
