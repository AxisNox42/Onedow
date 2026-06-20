#pragma once

struct GLFWwindow;
class TextRenderer;

bool UIButton(float x, float y, float w, float h, const wchar_t* label,
              double mx, double my, bool lmb, bool lmbPrev,
              bool selected = false);

float CenterTextX(float containerW, TextRenderer& tr, const wchar_t* text, float scale);
