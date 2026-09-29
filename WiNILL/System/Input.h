#pragma once

struct GLFWwindow;
enum class GameState;

struct InputFocusTransition {
    bool lost = false;
    bool gained = false;
};

extern bool  keys[1024];
extern float g_ScrollAccum;

void InputRegisterCallbacks(GLFWwindow* window);
// Clear callback state when the native window loses focus.  GLFW may not
// deliver release events while another application owns the keyboard.
void InputClearState();
InputFocusTransition InputUpdateWindowFocus(GLFWwindow* window);
void InputUpdateGameplayCursor(GLFWwindow* window, GameState state,
                               bool crosshairEnabled, bool debugVisible);
