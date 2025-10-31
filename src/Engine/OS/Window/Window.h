//
// Created by 2401Lucas on 2025-10-29.
//

#ifndef GPU_PARTICLE_SIM_WINDOW_H
#define GPU_PARTICLE_SIM_WINDOW_H

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>

class Window;

struct WindowConfig {
    int width = 1280;
    int height = 720;
    std::string title = "Window";
    bool resizable = true;
    bool fullscreen = false;
    bool vsync = true;
};

class Window {
public:
    // --- Global Management ---
    static bool InitGLFW();

    static void TerminateGLFW();

    static void PollAllEvents();

    static bool HasOpenWindows();

    static const std::unordered_set<Window *> &GetAllWindows();

    static void CloseAll();

    // --- Constructors ---
    explicit Window(const WindowConfig &config);

    ~Window();

    Window(Window &&other) noexcept;

    Window &operator=(Window &&other) noexcept;

    Window(const Window &) = delete;

    Window &operator=(const Window &) = delete;

    // --- Core API ---
    void makeContextCurrent() const;

    static void clearContext();

    bool shouldClose() const;

    void close();

    void setTitle(const std::string &title);

    void setVSync(bool enabled);

    bool isVSync() const { return m_VSyncEnabled; }

    void setSize(int width, int height);

    void setPosition(int x, int y);

    void show();

    void hide();

    void focus();

    // --- Input ---
    bool isKeyPressed(int key) const;

    bool isMouseButtonPressed(int button) const;

    void getCursorPosition(double &x, double &y) const;

    void setCursorMode(int mode);

    // --- Event Loop ---
    void processEvents();

    // --- Callbacks ---
    void setResizeCallback(std::function<void(int, int)> cb) {
        m_ResizeCallback = std::move(cb);
    }

    void setKeyCallback(std::function<void(int, int, int, int)> cb) {
        m_KeyCallback = std::move(cb);
    }

    void setMouseButtonCallback(std::function<void(int, int, int)> cb) {
        m_MouseButtonCallback = std::move(cb);
    }

    void setCursorPosCallback(std::function<void(double, double)> cb) {
        m_CursorPosCallback = std::move(cb);
    }

    // --- Getters ---
    int getWidth() const { return m_Width; }
    int getHeight() const { return m_Height; }

    float getAspectRatio() const {
        return m_Height > 0 ? static_cast<float>(m_Width) / m_Height : 0.0f;
    }

    GLFWwindow *getHandle() const { return m_Window; }
    HWND getHwnd() const { return glfwGetWin32Window(m_Window); }
    bool wasResized() const { return m_WasResized; }
    void resetResizeFlag() { m_WasResized = false; }

private:
    GLFWwindow *m_Window = nullptr;
    int m_Width = 0;
    int m_Height = 0;
    bool m_WasResized = false;
    bool m_VSyncEnabled = true;

    std::function<void(int, int)> m_ResizeCallback;
    std::function<void(int, int, int, int)> m_KeyCallback;
    std::function<void(int, int, int)> m_MouseButtonCallback;
    std::function<void(double, double)> m_CursorPosCallback;

    void cleanup();

    // --- Static State ---
    static std::unordered_set<Window *> &GetWindowRegistry();

    static std::mutex &GetRegistryMutex();

    static bool &GetGLFWInitFlag();

    // --- Static Callbacks ---
    static void errorCallback(int errorCode, const char *description);

    static void framebufferResizeCallback(GLFWwindow *, int, int);

    static void keyCallback(GLFWwindow *, int, int, int, int);

    static void mouseButtonCallback(GLFWwindow *, int, int, int);

    static void cursorPosCallback(GLFWwindow *, double, double);

    static Window *getWindowInstance(GLFWwindow *);
};

#endif
