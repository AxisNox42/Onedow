#pragma once

struct GLFWwindow;

extern bool  keys[1024];
extern float g_ScrollAccum;

void InputRegisterCallbacks(GLFWwindow* window);
