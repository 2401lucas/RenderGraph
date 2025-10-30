#include "Window.h"

#include <iostream>

// --- Static State ---
std::unordered_set<Window*>& Window::GetWindowRegistry() {
  static std::unordered_set<Window*> registry;
  return registry;
}

std::mutex& Window::GetRegistryMutex() {
  static std::mutex mutex;
  return mutex;
}

bool& Window::GetGLFWInitFlag() {
  static bool initialized = false;
  return initialized;
}

// --- Global Control ---
bool Window::InitGLFW() {
  if (GetGLFWInitFlag()) return true;
  glfwSetErrorCallback(errorCallback);
  if (!glfwInit()) {
    std::cerr << "GLFW: Initialization failed.\n";
    return false;
  }
  GetGLFWInitFlag() = true;
  return true;
}

void Window::TerminateGLFW() {
  if (GetGLFWInitFlag()) {
    glfwTerminate();
    GetGLFWInitFlag() = false;
  }
}

void Window::PollAllEvents() { glfwPollEvents(); }

bool Window::HasOpenWindows() {
  std::lock_guard<std::mutex> lock(GetRegistryMutex());
  return !GetWindowRegistry().empty();
}

const std::unordered_set<Window*>& Window::GetAllWindows() {
  std::lock_guard<std::mutex> lock(GetRegistryMutex());
  return GetWindowRegistry();
}

void Window::CloseAll() {
  std::lock_guard<std::mutex> lock(GetRegistryMutex());
  for (auto* w : GetWindowRegistry()) w->close();
}

// --- Construction ---
Window::Window(const WindowConfig& config) {
  if (!GetGLFWInitFlag())
    throw std::runtime_error("GLFW not initialized. Call Window::InitGLFW().");

  glfwWindowHint(GLFW_RESIZABLE, config.resizable);
  glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

  GLFWmonitor* monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
  m_Window = glfwCreateWindow(config.width, config.height, config.title.c_str(),
                              monitor, nullptr);

  if (!m_Window) throw std::runtime_error("Failed to create GLFW window.");

  m_Width = config.width;
  m_Height = config.height;
  m_VSyncEnabled = config.vsync;

  glfwSetWindowUserPointer(m_Window, this);
  glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
  glfwSetKeyCallback(m_Window, keyCallback);
  glfwSetMouseButtonCallback(m_Window, mouseButtonCallback);
  glfwSetCursorPosCallback(m_Window, cursorPosCallback);

  makeContextCurrent();
  setVSync(config.vsync);

  try {
    std::lock_guard<std::mutex> lock(GetRegistryMutex());
    GetWindowRegistry().insert(this);
  } catch (...) {
    glfwDestroyWindow(m_Window);
    m_Window = nullptr;
    throw;
  }
}

Window::~Window() { cleanup(); }

void Window::cleanup() {
  if (!m_Window) return;

  {
    std::lock_guard<std::mutex> lock(GetRegistryMutex());
    GetWindowRegistry().erase(this);
  }

  clearContext();
  glfwDestroyWindow(m_Window);
  m_Window = nullptr;
}

Window::Window(Window&& other) noexcept { *this = std::move(other); }

Window& Window::operator=(Window&& other) noexcept {
  if (this != &other) {
    cleanup();

    m_Window = other.m_Window;
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_VSyncEnabled = other.m_VSyncEnabled;
    m_WasResized = other.m_WasResized;
    m_ResizeCallback = std::move(other.m_ResizeCallback);
    m_KeyCallback = std::move(other.m_KeyCallback);
    m_MouseButtonCallback = std::move(other.m_MouseButtonCallback);
    m_CursorPosCallback = std::move(other.m_CursorPosCallback);

    other.m_Window = nullptr;

    if (m_Window) {
      glfwSetWindowUserPointer(m_Window, this);
      std::lock_guard<std::mutex> lock(GetRegistryMutex());
      GetWindowRegistry().erase(&other);
      GetWindowRegistry().insert(this);
    }
  }
  return *this;
}

// --- Core API ---
void Window::makeContextCurrent() const { glfwMakeContextCurrent(m_Window); }

void Window::clearContext() { glfwMakeContextCurrent(nullptr); }

bool Window::shouldClose() const {
  return m_Window && glfwWindowShouldClose(m_Window);
}

void Window::close() {
  if (m_Window) glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
}

void Window::setTitle(const std::string& title) {
  if (m_Window) glfwSetWindowTitle(m_Window, title.c_str());
}

void Window::setVSync(bool enabled) {
  if (m_Window) {
    makeContextCurrent();
    glfwSwapInterval(enabled ? 1 : 0);
    m_VSyncEnabled = enabled;
  }
}

void Window::setSize(int width, int height) {
  if (m_Window) {
    glfwSetWindowSize(m_Window, width, height);
    m_Width = width;
    m_Height = height;
  }
}

void Window::setPosition(int x, int y) {
  if (m_Window) glfwSetWindowPos(m_Window, x, y);
}

void Window::show() {
  if (m_Window) glfwShowWindow(m_Window);
}
void Window::hide() {
  if (m_Window) glfwHideWindow(m_Window);
}
void Window::focus() {
  if (m_Window) glfwFocusWindow(m_Window);
}

void Window::processEvents() {
  if (m_Window) glfwPollEvents();
}

// --- Input ---
bool Window::isKeyPressed(int key) const {
  return m_Window && glfwGetKey(m_Window, key) == GLFW_PRESS;
}

bool Window::isMouseButtonPressed(int button) const {
  return m_Window && glfwGetMouseButton(m_Window, button) == GLFW_PRESS;
}

void Window::getCursorPosition(double& x, double& y) const {
  if (m_Window)
    glfwGetCursorPos(m_Window, &x, &y);
  else {
    x = 0;
    y = 0;
  }
}

void Window::setCursorMode(int mode) {
  if (m_Window) glfwSetInputMode(m_Window, GLFW_CURSOR, mode);
}

// --- Callbacks ---
void Window::errorCallback(int errorCode, const char* description) {
  std::cerr << "GLFW Error [" << errorCode << "]: " << description << "\n";
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width,
                                       int height) {
  if (auto* self = getWindowInstance(window)) {
    self->m_Width = width;
    self->m_Height = height;
    self->m_WasResized = true;
    if (self->m_ResizeCallback) self->m_ResizeCallback(width, height);
  }
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action,
                         int mods) {
  if (auto* self = getWindowInstance(window); self && self->m_KeyCallback)
    self->m_KeyCallback(key, scancode, action, mods);
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action,
                                 int mods) {
  if (auto* self = getWindowInstance(window);
      self && self->m_MouseButtonCallback)
    self->m_MouseButtonCallback(button, action, mods);
}

void Window::cursorPosCallback(GLFWwindow* window, double x, double y) {
  if (auto* self = getWindowInstance(window); self && self->m_CursorPosCallback)
    self->m_CursorPosCallback(x, y);
}

Window* Window::getWindowInstance(GLFWwindow* window) {
  return static_cast<Window*>(glfwGetWindowUserPointer(window));
}
