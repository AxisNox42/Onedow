#pragma once

struct GLFWwindow;
class TextRenderer;

enum class UiTextLevel { Title, Subtitle, Description, Supporting };
float UiTextScale(TextRenderer& renderer, UiTextLevel level,
                  float responsiveScale = 1.0f);
float MaxLocalizedTextWidth(TextRenderer& renderer,
                            const wchar_t* const* variants, int count,
                            float scale);

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
              PanelButtonSlideSide slideSide = PanelButtonSlideSide::Both,
              float responsiveScale = 1.0f,
              const wchar_t* const* labelVariants = nullptr,
              int labelVariantCount = 0);

void DrawShadowedText(TextRenderer& renderer, const wchar_t* text,
                      float x, float y, float scale,
                      float r, float g, float b, float alpha,
                      float shadowAlpha = 0.68f);
void BeginDeferredSceneText();
void EndDeferredSceneText();
float UpdateMenuCommandHover(float current, bool hovered, float dt);
void DrawPanelButton(const wchar_t* label, float x, float y, float w, float h,
                     float r, float g, float b, float alpha,
                     float hover, bool selected, float pulse, float now,
                     float responsiveScale = 1.0f,
                     bool keepTextReadable = false,
                     PanelButtonSlideSide slideSide = PanelButtonSlideSide::Both,
                     bool centerText = false,
                     const wchar_t* const* labelVariants = nullptr,
                     int labelVariantCount = 0);
void DrawUnifiedMenuCommand(const wchar_t* label, float x, float y, float w, float h,
                            float r, float g, float b, float alpha,
                            float hover, bool selected, float pulse, float now,
                            float responsiveScale = 1.0f,
                            bool keepTextReadable = false,
                            bool bothSides = true,
                            bool centerText = false,
                            const wchar_t* const* labelVariants = nullptr,
                            int labelVariantCount = 0);
bool PanelButtonHit(double mx, double my,
                    float x, float y, float w, float h,
                    float hitPadding = 0.0f);
bool UpdatePanelButtonHover(float& hover, bool enabled,
                            double mx, double my,
                            float x, float y, float w, float h,
                            float dt, float hitPadding = 0.0f);

float CenterTextX(float containerW, TextRenderer& tr, const wchar_t* text, float scale);
