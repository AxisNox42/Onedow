#pragma once

struct GLFWwindow;
class TextRenderer;

// Shared directional feedback used by every panel-button renderer.
enum class PanelButtonSlideSide {
    None,
    Left,
    Right,
    Both
};

bool UIButton(float x, float y, float w, float h, const wchar_t* label,
              double mx, double my, bool lmb, bool lmbPrev,
              bool selected = false,
              PanelButtonSlideSide slideSide = PanelButtonSlideSide::Both);

float CenterTextX(float containerW, TextRenderer& tr, const wchar_t* text, float scale);
