#pragma once

#include <deque>
#include <functional>
#include <glm/vec2.hpp>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Window/Window.h"

// Forward declarations
class InputManager;

// Input source type
enum class InputSource { Keyboard, Mouse, Gamepad };

// Gamepad axis identifiers
enum class GamepadAxis {
  LeftX = 0,
  LeftY = 1,
  RightX = 2,
  RightY = 3,
  LeftTrigger = 4,
  RightTrigger = 5
};

// Gamepad button identifiers (matches GLFW_GAMEPAD_BUTTON_*)
enum class GamepadButton {
  A = 0,            // GLFW_GAMEPAD_BUTTON_A (Cross on PlayStation)
  B = 1,            // GLFW_GAMEPAD_BUTTON_B (Circle on PlayStation)
  X = 2,            // GLFW_GAMEPAD_BUTTON_X (Square on PlayStation)
  Y = 3,            // GLFW_GAMEPAD_BUTTON_Y (Triangle on PlayStation)
  LeftBumper = 4,   // GLFW_GAMEPAD_BUTTON_LEFT_BUMPER
  RightBumper = 5,  // GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER
  Back = 6,         // GLFW_GAMEPAD_BUTTON_BACK
  Start = 7,        // GLFW_GAMEPAD_BUTTON_START
  Guide = 8,        // GLFW_GAMEPAD_BUTTON_GUIDE
  LeftThumb = 9,    // GLFW_GAMEPAD_BUTTON_LEFT_THUMB
  RightThumb = 10,  // GLFW_GAMEPAD_BUTTON_RIGHT_THUMB
  DPadUp = 11,      // GLFW_GAMEPAD_BUTTON_DPAD_UP
  DPadRight = 12,   // GLFW_GAMEPAD_BUTTON_DPAD_RIGHT
  DPadDown = 13,    // GLFW_GAMEPAD_BUTTON_DPAD_DOWN
  DPadLeft = 14     // GLFW_GAMEPAD_BUTTON_DPAD_LEFT
};

// Input binding (can be key, mouse button, or gamepad button)
struct InputBinding {
  InputSource source = InputSource::Keyboard;
  int code = GLFW_KEY_UNKNOWN;  // Key code, mouse button, or gamepad button
  int gamepadIndex = 0;         // Which gamepad (0-3, for multi-player)

  bool operator==(const InputBinding& other) const {
    return source == other.source && code == other.code &&
           gamepadIndex == other.gamepadIndex;
  }

  bool operator<(const InputBinding& other) const {
    if (source != other.source) return source < other.source;
    if (code != other.code) return code < other.code;
    return gamepadIndex < other.gamepadIndex;
  }
};

// Hash function for InputBinding (for unordered_map)
namespace std {
template <>
struct hash<InputBinding> {
  size_t operator()(const InputBinding& b) const {
    return hash<int>()(static_cast<int>(b.source)) ^
           (hash<int>()(b.code) << 1) ^ (hash<int>()(b.gamepadIndex) << 2);
  }
};
}  // namespace std

// Action binding with multiple input sources
struct ActionBinding {
  std::string name;
  std::vector<InputBinding> bindings;  // Support multiple bindings per action
  bool requiresModifiers = false;
  int requiredMods = 0;  // GLFW_MOD_SHIFT, GLFW_MOD_CONTROL, etc.
};

// Axis binding (supports keyboard, mouse, and gamepad axes)
struct AxisBinding {
  std::string name;
  InputBinding negativeBinding;
  InputBinding positiveBinding;
  GamepadAxis gamepadAxis = GamepadAxis::LeftX;
  int gamepadIndex = 0;
  float scale = 1.0f;
  float deadzone = 0.15f;  // Deadzone for analog axes
  bool isMouseAxis = false;
  bool isGamepadAxis = false;
};

// Button/key state for edge detection
struct ButtonState {
  bool down = false;
  bool prevDown = false;
  double lastChangeTime = 0.0;
  bool consumed = false;  // For "first press" tracking
};

// Gamepad state
struct GamepadState {
  bool connected = false;
  std::string name;
  std::vector<float> axes;    // 6 axes (left stick, right stick, triggers)
  std::vector<bool> buttons;  // 15 buttons
  std::vector<bool> prevButtons;

  GamepadState() {
    axes.resize(6, 0.0f);
    buttons.resize(15, false);
    prevButtons.resize(15, false);
  }
};

// Event types for event queue
enum class InputEventType {
  ActionPressed,
  ActionReleased,
  AxisChanged,
  GamepadConnected,
  GamepadDisconnected,
  TextInput
};

// Input event structure
struct InputEvent {
  InputEventType type;
  std::string actionOrAxisName;
  float value = 0.0f;
  int gamepadIndex = -1;
  unsigned int codepoint = 0;
  double timestamp = 0.0;

  // Factory methods
  static InputEvent ActionPressed(const std::string& action, double time) {
    InputEvent e;
    e.type = InputEventType::ActionPressed;
    e.actionOrAxisName = action;
    e.timestamp = time;
    return e;
  }

  static InputEvent ActionReleased(const std::string& action, double time) {
    InputEvent e;
    e.type = InputEventType::ActionReleased;
    e.actionOrAxisName = action;
    e.timestamp = time;
    return e;
  }

  static InputEvent AxisChanged(const std::string& axis, float val,
                                double time) {
    InputEvent e;
    e.type = InputEventType::AxisChanged;
    e.actionOrAxisName = axis;
    e.value = val;
    e.timestamp = time;
    return e;
  }

  static InputEvent GamepadConnected(int index, double time) {
    InputEvent e;
    e.type = InputEventType::GamepadConnected;
    e.gamepadIndex = index;
    e.timestamp = time;
    return e;
  }

  static InputEvent GamepadDisconnected(int index, double time) {
    InputEvent e;
    e.type = InputEventType::GamepadDisconnected;
    e.gamepadIndex = index;
    e.timestamp = time;
    return e;
  }

  static InputEvent TextInput(unsigned int cp, double time) {
    InputEvent e;
    e.type = InputEventType::TextInput;
    e.codepoint = cp;
    e.timestamp = time;
    return e;
  }
};

/**
 * InputManager: Handles keyboard, mouse, and gamepad input
 * Uses event queue pattern to prevent deadlocks and improve safety
 */
class InputManager {
 public:
  enum class CallbackMode {
    Immediate,  // Fire callbacks immediately (fast but can deadlock if
                // InputManager is called from callback)
    Queued  // Queue events and process later (safe, 1-frame delay)
  };

  explicit InputManager(Window* window);
  ~InputManager();

  // Must be called once per frame (after Window::PollAllEvents())
  void Update();

  // Process queued events and fire callbacks (only needed in Queued mode)
  void processEvents();

  // --- Configuration ---
  void setCallbackMode(CallbackMode mode) { m_CallbackMode = mode; }
  CallbackMode getCallbackMode() const { return m_CallbackMode; }
  void setMaxQueueSize(size_t size) { m_MaxQueueSize = size; }

  // --- Action API ---
  void registerAction(const std::string& actionName,
                      const InputBinding& binding);
  void addActionBinding(const std::string& actionName,
                        const InputBinding& binding);

  bool isActionDown(const std::string& actionName) const;
  bool wasActionPressed(const std::string& actionName) const;
  bool wasActionReleased(const std::string& actionName) const;
  bool wasActionFirstPressed(const std::string& actionName);

  void remapAction(const std::string& actionName,
                   const InputBinding& oldBinding,
                   const InputBinding& newBinding);
  void clearActionBindings(const std::string& actionName);

  // --- Axis API ---
  void registerAxis(const std::string& axisName,
                    const InputBinding& negativeBinding,
                    const InputBinding& positiveBinding, float scale = 1.0f);
  void registerGamepadAxis(const std::string& axisName, GamepadAxis axis,
                           int gamepadIndex = 0, float scale = 1.0f,
                           float deadzone = 0.15f);

  float getAxis(const std::string& axisName) const;
  glm::vec2 getAxis2D(const std::string& axisNameX,
                      const std::string& axisNameY) const;

  // --- Mouse API ---
  glm::vec2 getMousePosition() const;
  glm::vec2 getMouseDelta() const;
  float getMouseWheel() const;
  void setMouseSensitivity(float sensitivity) {
    m_MouseSensitivity = sensitivity;
  }
  void setCursorMode(int mode);
  void resetMouseDelta();  // Call when changing cursor mode to prevent jumps

  // --- Gamepad API ---
  bool isGamepadConnected(int gamepadIndex = 0) const;
  std::string getGamepadName(int gamepadIndex = 0) const;
  int getConnectedGamepadCount() const;

  bool isGamepadButtonDown(GamepadButton button, int gamepadIndex = 0) const;
  bool wasGamepadButtonPressed(GamepadButton button,
                               int gamepadIndex = 0) const;
  bool wasGamepadButtonReleased(GamepadButton button,
                                int gamepadIndex = 0) const;

  float getGamepadAxis(GamepadAxis axis, int gamepadIndex = 0) const;
  glm::vec2 getGamepadLeftStick(int gamepadIndex = 0) const;
  glm::vec2 getGamepadRightStick(int gamepadIndex = 0) const;

  void setGamepadDeadzone(float deadzone) { m_DefaultDeadzone = deadzone; }
  void setGamepadVibration(int gamepadIndex, float leftMotor, float rightMotor);

  // --- Raw Input Query ---
  bool isKeyDown(int key) const;
  bool isMouseButtonDown(int button) const;

  // --- Persistence ---
  bool saveBindings(const std::string& filename) const;
  bool loadBindings(const std::string& filename);

  // --- Callbacks ---
  using ActionCallback = std::function<void(const std::string& actionName)>;
  using AxisCallback =
      std::function<void(const std::string& axisName, float value)>;
  using GamepadCallback = std::function<void(int gamepadIndex)>;
  using TextInputCallback = std::function<void(unsigned int codepoint)>;

  void setActionPressedCallback(ActionCallback callback) {
    m_ActionPressedCallback = std::move(callback);
  }
  void setActionReleasedCallback(ActionCallback callback) {
    m_ActionReleasedCallback = std::move(callback);
  }
  void setAxisChangedCallback(AxisCallback callback) {
    m_AxisChangedCallback = std::move(callback);
  }
  void setGamepadConnectedCallback(GamepadCallback callback) {
    m_GamepadConnectedCallback = std::move(callback);
  }
  void setGamepadDisconnectedCallback(GamepadCallback callback) {
    m_GamepadDisconnectedCallback = std::move(callback);
  }
  void setTextInputCallback(TextInputCallback callback) {
    m_TextInputCallback = std::move(callback);
  }

  // --- Utility ---
  double getTime() const { return glfwGetTime(); }
  void resetFirstPressFlags();

  // Debug
  size_t getQueuedEventCount() const;
  void clearEventQueue();

 private:
  Window* m_Window = nullptr;

  // Bindings
  std::map<std::string, ActionBinding> m_ActionBindings;
  std::map<std::string, AxisBinding> m_AxisBindings;

  // State tracking
  std::unordered_map<InputBinding, ButtonState> m_InputStates;
  std::vector<GamepadState> m_GamepadStates;

  // Mouse state
  glm::vec2 m_MousePos{0.0f};
  glm::vec2 m_PrevMousePos{0.0f};
  glm::vec2 m_MouseDelta{0.0f};
  float m_MouseSensitivity = 1.0f;
  double m_WheelDelta = 0.0;

  // Settings
  float m_DefaultDeadzone = 0.15f;
  CallbackMode m_CallbackMode = CallbackMode::Queued;
  size_t m_MaxQueueSize = 1000;

  // Event queue
  std::deque<InputEvent> m_EventQueue;
  mutable std::mutex m_QueueMutex;

  // Callbacks
  ActionCallback m_ActionPressedCallback;
  ActionCallback m_ActionReleasedCallback;
  AxisCallback m_AxisChangedCallback;
  GamepadCallback m_GamepadConnectedCallback;
  GamepadCallback m_GamepadDisconnectedCallback;
  TextInputCallback m_TextInputCallback;

  // Thread safety
  mutable std::mutex m_Mutex;

  // Internal helpers
  void updateGamepads();
  void updateButtonState(const InputBinding& binding, bool isDown);
  bool isBindingDown(const InputBinding& binding) const;
  bool wasBindingPressed(const InputBinding& binding) const;
  bool wasBindingReleased(const InputBinding& binding) const;
  float applyDeadzone(float value, float deadzone) const;

  // Event queue helpers
  void pushEvent(InputEvent event);
  void triggerActionPressed(const std::string& actionName);
  void triggerActionReleased(const std::string& actionName);

  // GLFW callback handlers
  void onKey(int key, int scancode, int action, int mods);
  void onMouseButton(int button, int action, int mods);
  void onCursorPos(double x, double y);
  void onScroll(double xoffset, double yoffset);
  void onCharInput(unsigned int codepoint);
};

// Helper functions for creating bindings
inline InputBinding KeyboardBinding(int key) {
  return InputBinding{InputSource::Keyboard, key, 0};
}

inline InputBinding MouseBinding(int button) {
  return InputBinding{InputSource::Mouse, button, 0};
}

inline InputBinding GamepadBinding(GamepadButton button, int gamepadIndex = 0) {
  return InputBinding{InputSource::Gamepad, static_cast<int>(button),
                      gamepadIndex};
}