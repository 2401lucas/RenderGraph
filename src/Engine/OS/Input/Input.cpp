#include "Input.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

// Static registry for InputManager instances (to handle scroll/char callbacks)
namespace {
std::unordered_map<GLFWwindow*, InputManager*> g_InputManagers;
std::mutex g_RegistryMutex;

// RAII helper for exception-safe registration
struct RegistryGuard {
  GLFWwindow* window;
  InputManager* manager;
  bool registered = false;

  RegistryGuard(GLFWwindow* w, InputManager* m) : window(w), manager(m) {
    std::lock_guard<std::mutex> lock(g_RegistryMutex);
    g_InputManagers[window] = manager;
    registered = true;
  }

  ~RegistryGuard() {
    if (registered) {
      std::lock_guard<std::mutex> lock(g_RegistryMutex);
      g_InputManagers.erase(window);
    }
  }

  void release() { registered = false; }
};
}  // namespace

InputManager::InputManager(Window* window) : m_Window(window) {
  if (!m_Window) {
    throw std::runtime_error("InputManager requires a valid Window pointer");
  }

  // Initialize mouse position
  double mx, my;
  glfwGetCursorPos(m_Window->getHandle(), &mx, &my);
  m_MousePos = glm::vec2(static_cast<float>(mx), static_cast<float>(my));
  m_PrevMousePos = m_MousePos;

  // Initialize gamepad states (up to 4 gamepads)
  m_GamepadStates.resize(4);

  // Exception-safe registration
  RegistryGuard guard(m_Window->getHandle(), this);

  // Register callbacks via Window's callback system
  // Note: Capturing 'this' is safe because callbacks won't fire until after
  // construction
  m_Window->setKeyCallback([this](int key, int scancode, int action, int mods) {
    onKey(key, scancode, action, mods);
  });

  m_Window->setMouseButtonCallback([this](int button, int action, int mods) {
    onMouseButton(button, action, mods);
  });

  m_Window->setCursorPosCallback(
      [this](double x, double y) { onCursorPos(x, y); });

  // Set scroll and char callbacks directly
  glfwSetScrollCallback(m_Window->getHandle(),
                        [](GLFWwindow* wnd, double x, double y) {
                          std::lock_guard<std::mutex> lock(g_RegistryMutex);
                          auto it = g_InputManagers.find(wnd);
                          if (it != g_InputManagers.end()) {
                            it->second->onScroll(x, y);
                          }
                        });

  glfwSetCharCallback(m_Window->getHandle(),
                      [](GLFWwindow* wnd, unsigned int codepoint) {
                        std::lock_guard<std::mutex> lock(g_RegistryMutex);
                        auto it = g_InputManagers.find(wnd);
                        if (it != g_InputManagers.end()) {
                          it->second->onCharInput(codepoint);
                        }
                      });

  guard.release();  // Successfully constructed, keep registration
}

InputManager::~InputManager() {
  // Unregister from global map
  {
    std::lock_guard<std::mutex> lock(g_RegistryMutex);
    g_InputManagers.erase(m_Window->getHandle());
  }

  // Clear Window callbacks
  if (m_Window && m_Window->getHandle()) {
    m_Window->setKeyCallback(nullptr);
    m_Window->setMouseButtonCallback(nullptr);
    m_Window->setCursorPosCallback(nullptr);
    glfwSetScrollCallback(m_Window->getHandle(), nullptr);
    glfwSetCharCallback(m_Window->getHandle(), nullptr);
  }
}

void InputManager::Update() {
  if (!m_Window) return;

  std::lock_guard<std::mutex> lock(m_Mutex);

  updateGamepads();

  // Mouse position is updated via onCursorPos callback
  // Store previous position for next frame's delta calculation
  m_PrevMousePos = m_MousePos;

  // Reset wheel delta after frame
  m_WheelDelta = 0.0;
}

void InputManager::updateGamepads() {
  double now = getTime();

  for (int i = 0; i < 4; ++i) {
    auto& state = m_GamepadStates[i];
    bool wasConnected = state.connected;

    // Check connection
    int present = glfwJoystickPresent(GLFW_JOYSTICK_1 + i);
    state.connected = (present == GLFW_TRUE);

    // Fire connection/disconnection events
    if (state.connected && !wasConnected) {
      pushEvent(InputEvent::GamepadConnected(i, now));
    } else if (!state.connected && wasConnected) {
      pushEvent(InputEvent::GamepadDisconnected(i, now));
    }

    if (!state.connected) {
      state.axes.assign(6, 0.0f);
      state.buttons.assign(15, false);
      state.prevButtons.assign(15, false);
      continue;
    }

    // Get gamepad name
    const char* name = glfwGetJoystickName(GLFW_JOYSTICK_1 + i);
    if (name) {
      state.name = std::string(name);
    }

    // Check if gamepad supports standard mapping
    int isGamepad = glfwJoystickIsGamepad(GLFW_JOYSTICK_1 + i);
    if (isGamepad != GLFW_TRUE) continue;

    // Get gamepad state
    GLFWgamepadstate glfwState;
    if (glfwGetGamepadState(GLFW_JOYSTICK_1 + i, &glfwState) == GLFW_TRUE) {
      // Update axes
      state.axes[static_cast<int>(GamepadAxis::LeftX)] =
          glfwState.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
      state.axes[static_cast<int>(GamepadAxis::LeftY)] =
          glfwState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
      state.axes[static_cast<int>(GamepadAxis::RightX)] =
          glfwState.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
      state.axes[static_cast<int>(GamepadAxis::RightY)] =
          glfwState.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
      state.axes[static_cast<int>(GamepadAxis::LeftTrigger)] =
          glfwState.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
      state.axes[static_cast<int>(GamepadAxis::RightTrigger)] =
          glfwState.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];

      // Update buttons
      state.prevButtons = state.buttons;
      for (int btn = 0; btn < 15; ++btn) {
        bool nowDown = (glfwState.buttons[btn] == GLFW_PRESS);
        bool wasDown = state.buttons[btn];
        state.buttons[btn] = nowDown;

        InputBinding binding{InputSource::Gamepad, btn, i};
        updateButtonState(binding, nowDown);

        // Trigger action events for this button
        if (nowDown && !wasDown) {
          // Button was just pressed
          for (const auto& [actionName, actionBinding] : m_ActionBindings) {
            for (const auto& b : actionBinding.bindings) {
              if (b == binding) {
                triggerActionPressed(actionName);
                break;
              }
            }
          }
        } else if (!nowDown && wasDown) {
          // Button was just released
          for (const auto& [actionName, actionBinding] : m_ActionBindings) {
            for (const auto& b : actionBinding.bindings) {
              if (b == binding) {
                triggerActionReleased(actionName);
                break;
              }
            }
          }
        }
      }
    }
  }
}

void InputManager::processEvents() {
  // Move events out of queue (minimal lock time)
  std::deque<InputEvent> events;
  {
    std::lock_guard<std::mutex> lock(m_QueueMutex);
    events = std::move(m_EventQueue);
    m_EventQueue.clear();
  }

  // Process events WITHOUT holding any locks (safe for callbacks to call
  // InputManager methods)
  for (const auto& event : events) {
    switch (event.type) {
      case InputEventType::ActionPressed:
        if (m_ActionPressedCallback) {
          m_ActionPressedCallback(event.actionOrAxisName);
        }
        break;

      case InputEventType::ActionReleased:
        if (m_ActionReleasedCallback) {
          m_ActionReleasedCallback(event.actionOrAxisName);
        }
        break;

      case InputEventType::AxisChanged:
        if (m_AxisChangedCallback) {
          m_AxisChangedCallback(event.actionOrAxisName, event.value);
        }
        break;

      case InputEventType::GamepadConnected:
        if (m_GamepadConnectedCallback) {
          m_GamepadConnectedCallback(event.gamepadIndex);
        }
        break;

      case InputEventType::GamepadDisconnected:
        if (m_GamepadDisconnectedCallback) {
          m_GamepadDisconnectedCallback(event.gamepadIndex);
        }
        break;

      case InputEventType::TextInput:
        if (m_TextInputCallback) {
          m_TextInputCallback(event.codepoint);
        }
        break;
    }
  }
}

void InputManager::pushEvent(InputEvent event) {
  std::lock_guard<std::mutex> lock(m_QueueMutex);

  // Prevent queue overflow
  if (m_EventQueue.size() >= m_MaxQueueSize) {
    // Drop oldest event
    m_EventQueue.pop_front();
  }

  m_EventQueue.push_back(std::move(event));
}

void InputManager::triggerActionPressed(const std::string& actionName) {
  if (m_CallbackMode == CallbackMode::Immediate) {
    if (m_ActionPressedCallback) {
      m_ActionPressedCallback(actionName);
    }
  } else {
    pushEvent(InputEvent::ActionPressed(actionName, getTime()));
  }
}

void InputManager::triggerActionReleased(const std::string& actionName) {
  if (m_CallbackMode == CallbackMode::Immediate) {
    if (m_ActionReleasedCallback) {
      m_ActionReleasedCallback(actionName);
    }
  } else {
    pushEvent(InputEvent::ActionReleased(actionName, getTime()));
  }
}

void InputManager::updateButtonState(const InputBinding& binding, bool isDown) {
  auto& state = m_InputStates[binding];
  state.prevDown = state.down;

  if (isDown != state.down) {
    state.down = isDown;
    state.lastChangeTime = getTime();
    state.consumed = false;
  }
}

bool InputManager::isBindingDown(const InputBinding& binding) const {
  switch (binding.source) {
    case InputSource::Keyboard:
      return glfwGetKey(m_Window->getHandle(), binding.code) == GLFW_PRESS;

    case InputSource::Mouse:
      return glfwGetMouseButton(m_Window->getHandle(), binding.code) ==
             GLFW_PRESS;

    case InputSource::Gamepad:
      if (binding.gamepadIndex < 0 || binding.gamepadIndex >= 4) return false;
      {
        const auto& state = m_GamepadStates[binding.gamepadIndex];
        if (!state.connected || binding.code < 0 || binding.code >= 15)
          return false;
        return state.buttons[binding.code];
      }

    default:
      return false;
  }
}

bool InputManager::wasBindingPressed(const InputBinding& binding) const {
  auto it = m_InputStates.find(binding);
  if (it == m_InputStates.end()) return false;

  const auto& state = it->second;
  return !state.prevDown && state.down;
}

bool InputManager::wasBindingReleased(const InputBinding& binding) const {
  auto it = m_InputStates.find(binding);
  if (it == m_InputStates.end()) return false;

  const auto& state = it->second;
  return state.prevDown && !state.down;
}

float InputManager::applyDeadzone(float value, float deadzone) const {
  if (std::abs(value) < deadzone) return 0.0f;

  // Rescale from [deadzone, 1.0] to [0.0, 1.0]
  float sign = (value > 0.0f) ? 1.0f : -1.0f;
  float magnitude = std::abs(value);
  return sign * ((magnitude - deadzone) / (1.0f - deadzone));
}

// --- Action API ---

void InputManager::registerAction(const std::string& actionName,
                                  const InputBinding& binding) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  ActionBinding action;
  action.name = actionName;
  action.bindings.push_back(binding);
  m_ActionBindings[actionName] = action;

  m_InputStates[binding] = ButtonState{};
}

void InputManager::addActionBinding(const std::string& actionName,
                                    const InputBinding& binding) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto it = m_ActionBindings.find(actionName);
  if (it != m_ActionBindings.end()) {
    it->second.bindings.push_back(binding);
    m_InputStates[binding] = ButtonState{};
  }
}

bool InputManager::isActionDown(const std::string& actionName) const {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto it = m_ActionBindings.find(actionName);
  if (it == m_ActionBindings.end()) return false;

  const auto& action = it->second;

  // Check modifier requirements
  if (action.requiresModifiers) {
    // Query current modifier state
    int currentMods = 0;
    if (glfwGetKey(m_Window->getHandle(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(m_Window->getHandle(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
      currentMods |= GLFW_MOD_SHIFT;
    }
    if (glfwGetKey(m_Window->getHandle(), GLFW_KEY_LEFT_CONTROL) ==
            GLFW_PRESS ||
        glfwGetKey(m_Window->getHandle(), GLFW_KEY_RIGHT_CONTROL) ==
            GLFW_PRESS) {
      currentMods |= GLFW_MOD_CONTROL;
    }
    if (glfwGetKey(m_Window->getHandle(), GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(m_Window->getHandle(), GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
      currentMods |= GLFW_MOD_ALT;
    }
    if (glfwGetKey(m_Window->getHandle(), GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
        glfwGetKey(m_Window->getHandle(), GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS) {
      currentMods |= GLFW_MOD_SUPER;
    }

    // Check if required modifiers match
    if ((currentMods & action.requiredMods) != action.requiredMods) {
      return false;
    }
  }

  for (const auto& binding : action.bindings) {
    if (isBindingDown(binding)) {
      return true;
    }
  }

  return false;
}

bool InputManager::wasActionPressed(const std::string& actionName) const {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto it = m_ActionBindings.find(actionName);
  if (it == m_ActionBindings.end()) return false;

  const auto& action = it->second;
  for (const auto& binding : action.bindings) {
    if (wasBindingPressed(binding)) {
      return true;
    }
  }

  return false;
}

bool InputManager::wasActionReleased(const std::string& actionName) const {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto it = m_ActionBindings.find(actionName);
  if (it == m_ActionBindings.end()) return false;

  const auto& action = it->second;
  for (const auto& binding : action.bindings) {
    if (wasBindingReleased(binding)) {
      return true;
    }
  }

  return false;
}

bool InputManager::wasActionFirstPressed(const std::string& actionName) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto it = m_ActionBindings.find(actionName);
  if (it == m_ActionBindings.end()) return false;

  const auto& action = it->second;
  for (const auto& binding : action.bindings) {
    auto stateIt = m_InputStates.find(binding);
    if (stateIt != m_InputStates.end()) {
      auto& state = stateIt->second;
      if (!state.prevDown && state.down && !state.consumed) {
        state.consumed = true;
        return true;
      }
    }
  }

  return false;
}

void InputManager::remapAction(const std::string& actionName,
                               const InputBinding& oldBinding,
                               const InputBinding& newBinding) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto it = m_ActionBindings.find(actionName);
  if (it == m_ActionBindings.end()) return;

  auto& bindings = it->second.bindings;
  auto bindIt = std::find(bindings.begin(), bindings.end(), oldBinding);
  if (bindIt != bindings.end()) {
    *bindIt = newBinding;
    m_InputStates[newBinding] = ButtonState{};
  }
}

void InputManager::clearActionBindings(const std::string& actionName) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_ActionBindings.erase(actionName);
}

// --- Axis API ---

void InputManager::registerAxis(const std::string& axisName,
                                const InputBinding& negativeBinding,
                                const InputBinding& positiveBinding,
                                float scale) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  AxisBinding axis;
  axis.name = axisName;
  axis.negativeBinding = negativeBinding;
  axis.positiveBinding = positiveBinding;
  axis.scale = scale;
  axis.isMouseAxis = false;
  axis.isGamepadAxis = false;

  m_AxisBindings[axisName] = axis;

  m_InputStates[negativeBinding] = ButtonState{};
  m_InputStates[positiveBinding] = ButtonState{};
}

void InputManager::registerGamepadAxis(const std::string& axisName,
                                       GamepadAxis axis, int gamepadIndex,
                                       float scale, float deadzone) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  AxisBinding axisBinding;
  axisBinding.name = axisName;
  axisBinding.gamepadAxis = axis;
  axisBinding.gamepadIndex = gamepadIndex;
  axisBinding.scale = scale;
  axisBinding.deadzone = deadzone;
  axisBinding.isGamepadAxis = true;
  axisBinding.isMouseAxis = false;

  m_AxisBindings[axisName] = axisBinding;
}

// MouseX & MouseY is always available
float InputManager::getAxis(const std::string& axisName) const {
  std::lock_guard<std::mutex> lock(m_Mutex);

  auto it = m_AxisBindings.find(axisName);
  if (it == m_AxisBindings.end()) return 0.0f;

  const auto& axis = it->second;

  // Handle gamepad axis
  if (axis.isGamepadAxis) {
    if (axis.gamepadIndex < 0 || axis.gamepadIndex >= 4) return 0.0f;
    const auto& state = m_GamepadStates[axis.gamepadIndex];
    if (!state.connected) return 0.0f;

    int axisIndex = static_cast<int>(axis.gamepadAxis);
    if (axisIndex < 0 || axisIndex >= 6) return 0.0f;

    float value = state.axes[axisIndex];
    value = applyDeadzone(value, axis.deadzone);
    return value * axis.scale;
  }

  // Handle mouse axis
  if (axis.isMouseAxis) {
    if (axisName == "MouseX") return m_MouseDelta.x * axis.scale;
    if (axisName == "MouseY") return m_MouseDelta.y * axis.scale;
    return 0.0f;
  }

  // Handle digital axis (keyboard/mouse buttons)
  float value = 0.0f;
  if (isBindingDown(axis.negativeBinding)) value -= 1.0f;
  if (isBindingDown(axis.positiveBinding)) value += 1.0f;

  return value * axis.scale;
}

glm::vec2 InputManager::getAxis2D(const std::string& axisNameX,
                                  const std::string& axisNameY) const {
  return glm::vec2(getAxis(axisNameX), getAxis(axisNameY));
}

// --- Gamepad API ---

bool InputManager::isGamepadConnected(int gamepadIndex) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (gamepadIndex < 0 || gamepadIndex >= 4) return false;
  return m_GamepadStates[gamepadIndex].connected;
}

std::string InputManager::getGamepadName(int gamepadIndex) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (gamepadIndex < 0 || gamepadIndex >= 4) return "";
  return m_GamepadStates[gamepadIndex].name;
}

int InputManager::getConnectedGamepadCount() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  int count = 0;
  for (const auto& state : m_GamepadStates) {
    if (state.connected) ++count;
  }
  return count;
}

bool InputManager::isGamepadButtonDown(GamepadButton button,
                                       int gamepadIndex) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (gamepadIndex < 0 || gamepadIndex >= 4) return false;

  const auto& state = m_GamepadStates[gamepadIndex];
  if (!state.connected) return false;

  int btnIndex = static_cast<int>(button);
  if (btnIndex < 0 || btnIndex >= 15) return false;

  return state.buttons[btnIndex];
}

bool InputManager::wasGamepadButtonPressed(GamepadButton button,
                                           int gamepadIndex) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (gamepadIndex < 0 || gamepadIndex >= 4) return false;

  const auto& state = m_GamepadStates[gamepadIndex];
  if (!state.connected) return false;

  int btnIndex = static_cast<int>(button);
  if (btnIndex < 0 || btnIndex >= 15) return false;

  return state.buttons[btnIndex] && !state.prevButtons[btnIndex];
}

bool InputManager::wasGamepadButtonReleased(GamepadButton button,
                                            int gamepadIndex) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (gamepadIndex < 0 || gamepadIndex >= 4) return false;

  const auto& state = m_GamepadStates[gamepadIndex];
  if (!state.connected) return false;

  int btnIndex = static_cast<int>(button);
  if (btnIndex < 0 || btnIndex >= 15) return false;

  return !state.buttons[btnIndex] && state.prevButtons[btnIndex];
}

float InputManager::getGamepadAxis(GamepadAxis axis, int gamepadIndex) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (gamepadIndex < 0 || gamepadIndex >= 4) return 0.0f;

  const auto& state = m_GamepadStates[gamepadIndex];
  if (!state.connected) return 0.0f;

  int axisIndex = static_cast<int>(axis);
  if (axisIndex < 0 || axisIndex >= 6) return 0.0f;

  float value = state.axes[axisIndex];
  return applyDeadzone(value, m_DefaultDeadzone);
}

glm::vec2 InputManager::getGamepadLeftStick(int gamepadIndex) const {
  return glm::vec2(getGamepadAxis(GamepadAxis::LeftX, gamepadIndex),
                   getGamepadAxis(GamepadAxis::LeftY, gamepadIndex));
}

glm::vec2 InputManager::getGamepadRightStick(int gamepadIndex) const {
  return glm::vec2(getGamepadAxis(GamepadAxis::RightX, gamepadIndex),
                   getGamepadAxis(GamepadAxis::RightY, gamepadIndex));
}

void InputManager::setGamepadVibration(int gamepadIndex, float leftMotor,
                                       float rightMotor) {
  // Note: GLFW doesn't support rumble/vibration natively
  // TODO: use a platform-specific API
}

// --- Mouse API ---

glm::vec2 InputManager::getMousePosition() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_MousePos;
}

glm::vec2 InputManager::getMouseDelta() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_MouseDelta;
}

float InputManager::getMouseWheel() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return static_cast<float>(m_WheelDelta);
}

void InputManager::setCursorMode(int mode) {
  if (m_Window) {
    glfwSetInputMode(m_Window->getHandle(), GLFW_CURSOR, mode);
    resetMouseDelta();  // Prevent huge delta on next frame
  }
}

void InputManager::resetMouseDelta() {
  std::lock_guard<std::mutex> lock(m_Mutex);

  // Get current position and reset delta
  double mx, my;
  glfwGetCursorPos(m_Window->getHandle(), &mx, &my);
  m_MousePos = glm::vec2(static_cast<float>(mx), static_cast<float>(my));
  m_PrevMousePos = m_MousePos;
  m_MouseDelta = glm::vec2(0.0f);
}

// --- Raw Input ---

bool InputManager::isKeyDown(int key) const {
  if (!m_Window) return false;
  return glfwGetKey(m_Window->getHandle(), key) == GLFW_PRESS;
}

bool InputManager::isMouseButtonDown(int button) const {
  if (!m_Window) return false;
  return glfwGetMouseButton(m_Window->getHandle(), button) == GLFW_PRESS;
}

// --- Persistence ---

bool InputManager::saveBindings(const std::string& filename) const {
  std::lock_guard<std::mutex> lock(m_Mutex);

  nlohmann::json j;

  // Save actions
  for (const auto& [name, action] : m_ActionBindings) {
    nlohmann::json actionJson;
    nlohmann::json bindingsArray = nlohmann::json::array();

    for (const auto& binding : action.bindings) {
      nlohmann::json bindingJson;
      bindingJson["source"] = static_cast<int>(binding.source);
      bindingJson["code"] = binding.code;
      bindingJson["gamepadIndex"] = binding.gamepadIndex;
      bindingsArray.push_back(bindingJson);
    }

    actionJson["bindings"] = bindingsArray;
    actionJson["requiresModifiers"] = action.requiresModifiers;
    actionJson["requiredMods"] = action.requiredMods;
    j["actions"][name] = actionJson;
  }

  // Save axes
  for (const auto& [name, axis] : m_AxisBindings) {
    nlohmann::json axisJson;
    axisJson["negativeSource"] = static_cast<int>(axis.negativeBinding.source);
    axisJson["negativeCode"] = axis.negativeBinding.code;
    axisJson["positiveSource"] = static_cast<int>(axis.positiveBinding.source);
    axisJson["positiveCode"] = axis.positiveBinding.code;
    axisJson["scale"] = axis.scale;
    axisJson["deadzone"] = axis.deadzone;
    axisJson["isMouseAxis"] = axis.isMouseAxis;
    axisJson["isGamepadAxis"] = axis.isGamepadAxis;

    if (axis.isGamepadAxis) {
      axisJson["gamepadAxis"] = static_cast<int>(axis.gamepadAxis);
      axisJson["gamepadIndex"] = axis.gamepadIndex;
    }

    j["axes"][name] = axisJson;
  }

  std::ofstream ofs(filename);
  if (!ofs) return false;

  ofs << j.dump(2);
  return true;
}

bool InputManager::loadBindings(const std::string& filename) {
  std::ifstream ifs(filename);
  if (!ifs) return false;

  nlohmann::json j;
  try {
    ifs >> j;
  } catch (const std::exception& e) {
    std::cerr << "Failed to parse input bindings: " << e.what() << "\n";
    return false;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);

  // Load actions
  if (j.contains("actions")) {
    for (auto& [name, actionData] : j["actions"].items()) {
      ActionBinding action;
      action.name = name;
      action.requiresModifiers = actionData.value("requiresModifiers", false);
      action.requiredMods = actionData.value("requiredMods", 0);

      if (actionData.contains("bindings")) {
        for (const auto& bindingData : actionData["bindings"]) {
          InputBinding binding;
          binding.source =
              static_cast<InputSource>(bindingData.value("source", 0));
          binding.code = bindingData.value("code", GLFW_KEY_UNKNOWN);
          binding.gamepadIndex = bindingData.value("gamepadIndex", 0);
          action.bindings.push_back(binding);
          m_InputStates[binding] = ButtonState{};
        }
      }

      m_ActionBindings[action.name] = action;
    }
  }

  // Load axes
  if (j.contains("axes")) {
    for (auto& [name, axisData] : j["axes"].items()) {
      AxisBinding axis;
      axis.name = name;
      axis.negativeBinding.source =
          static_cast<InputSource>(axisData.value("negativeSource", 0));
      axis.negativeBinding.code =
          axisData.value("negativeCode", GLFW_KEY_UNKNOWN);
      axis.positiveBinding.source =
          static_cast<InputSource>(axisData.value("positiveSource", 0));
      axis.positiveBinding.code =
          axisData.value("positiveCode", GLFW_KEY_UNKNOWN);
      axis.scale = axisData.value("scale", 1.0f);
      axis.deadzone = axisData.value("deadzone", 0.15f);
      axis.isMouseAxis = axisData.value("isMouseAxis", false);
      axis.isGamepadAxis = axisData.value("isGamepadAxis", false);

      if (axis.isGamepadAxis) {
        axis.gamepadAxis =
            static_cast<GamepadAxis>(axisData.value("gamepadAxis", 0));
        axis.gamepadIndex = axisData.value("gamepadIndex", 0);
      }

      m_AxisBindings[axis.name] = axis;
      m_InputStates[axis.negativeBinding] = ButtonState{};
      m_InputStates[axis.positiveBinding] = ButtonState{};
    }
  }

  return true;
}

void InputManager::resetFirstPressFlags() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (auto& [binding, state] : m_InputStates) {
    state.consumed = false;
  }
}

size_t InputManager::getQueuedEventCount() const {
  std::lock_guard<std::mutex> lock(m_QueueMutex);
  return m_EventQueue.size();
}

void InputManager::clearEventQueue() {
  std::lock_guard<std::mutex> lock(m_QueueMutex);
  m_EventQueue.clear();
}

// --- Callback Handlers ---

void InputManager::onKey(int key, int scancode, int action, int mods) {
  InputBinding binding = KeyboardBinding(key);

  bool isDown = (action == GLFW_PRESS || action == GLFW_REPEAT);

  // Update state
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    updateButtonState(binding, isDown);
  }

  // Trigger action events
  if (action == GLFW_PRESS) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& [actionName, actionBinding] : m_ActionBindings) {
      for (const auto& b : actionBinding.bindings) {
        if (b == binding) {
          triggerActionPressed(actionName);
          break;
        }
      }
    }
  } else if (action == GLFW_RELEASE) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& [actionName, actionBinding] : m_ActionBindings) {
      for (const auto& b : actionBinding.bindings) {
        if (b == binding) {
          triggerActionReleased(actionName);
          break;
        }
      }
    }
  }
}

void InputManager::onMouseButton(int button, int action, int mods) {
  InputBinding binding = MouseBinding(button);

  bool isDown = (action == GLFW_PRESS);

  // Update state
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    updateButtonState(binding, isDown);
  }

  // Trigger action events
  if (action == GLFW_PRESS) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& [actionName, actionBinding] : m_ActionBindings) {
      for (const auto& b : actionBinding.bindings) {
        if (b == binding) {
          triggerActionPressed(actionName);
          break;
        }
      }
    }
  } else if (action == GLFW_RELEASE) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& [actionName, actionBinding] : m_ActionBindings) {
      for (const auto& b : actionBinding.bindings) {
        if (b == binding) {
          triggerActionReleased(actionName);
          break;
        }
      }
    }
  }
}
// this could be called multiple times per frame ,and the delta is accumulated
// which could cause a jittery input in the right scenario
void InputManager::onCursorPos(double x, double y) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  glm::vec2 newPos(static_cast<float>(x), static_cast<float>(y));
  m_MouseDelta = (newPos - m_MousePos) * m_MouseSensitivity;
  m_MousePos = newPos;
}

void InputManager::onScroll(double xoffset, double yoffset) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_WheelDelta += yoffset;
}

void InputManager::onCharInput(unsigned int codepoint) {
  if (m_CallbackMode == CallbackMode::Immediate) {
    if (m_TextInputCallback) {
      m_TextInputCallback(codepoint);
    }
  } else {
    pushEvent(InputEvent::TextInput(codepoint, getTime()));
  }
}