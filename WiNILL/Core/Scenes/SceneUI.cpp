#include "SceneUI.h"
#include "DrawPrim.h"
#include "TextRenderer.h"
#include "UiLayout.h"
#include "Settings.h"
#include "Translations.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

extern TextRenderer g_TextL;
extern TextRenderer g_TextS;

namespace {

struct UIButtonMotion {
    bool used = false;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float hover = 0.0f;
    double lastTime = 0.0;
};

// UIButton is used by older panels that do not own a per-row hover array.
// Keep the motion state here so those panels still use the same eased hover
// treatment as the newer menu command rail.
UIButtonMotion& MotionForButton(float x, float y, float w, float h) {
    static UIButtonMotion slots[128];
    for (UIButtonMotion& slot : slots) {
        if (slot.used && std::fabs(slot.x - x) < 0.25f
                     && std::fabs(slot.y - y) < 0.25f
                     && std::fabs(slot.w - w) < 0.25f
                     && std::fabs(slot.h - h) < 0.25f) {
            return slot;
        }
    }
    for (UIButtonMotion& slot : slots) {
        if (!slot.used) {
            slot.used = true;
            slot.x = x;
            slot.y = y;
            slot.w = w;
            slot.h = h;
            return slot;
        }
    }
    // The game has fewer than 128 simultaneously visible legacy buttons. If
    // a future panel exceeds that, reuse a stable slot rather than allocating
    // per-frame state.
    static int recycle = 0;
    UIButtonMotion& slot = slots[recycle++ % 128];
    slot.used = true;
    slot.x = x;
    slot.y = y;
    slot.w = w;
    slot.h = h;
    slot.hover = 0.0f;
    slot.lastTime = 0.0;
    return slot;
}

void DrawUIButtonFeedback(float x, float y, float w, float h,
                          float hover, float r, float g, float b,
                          float alpha, PanelButtonSlideSide slideSide) {
    if (hover <= 0.01f || alpha <= 0.001f) return;
    BindMainShader();

    const float slide = 8.0f + 13.0f * hover;
    const float barH = std::min(h * 0.84f, 22.0f + 20.0f * hover);
    const float barY = y + (h - barH) * 0.5f;
    const float barA = (0.30f + 0.52f * hover) * alpha;
    const bool showLeft = slideSide == PanelButtonSlideSide::Left
                       || slideSide == PanelButtonSlideSide::Both;
    const bool showRight = slideSide == PanelButtonSlideSide::Right
                        || slideSide == PanelButtonSlideSide::Both;
    if (showLeft) {
        drawRect(x - slide, barY, 2.0f, barH, r, g, b, barA);
        drawDiamond(x - slide - 5.0f, y + h * 0.5f,
                    3.0f + 2.0f * hover, r, g, b,
                    (0.42f + 0.32f * hover) * alpha);
    }
    if (showRight) {
        drawRect(x + w + slide - 2.0f, barY, 2.0f, barH, r, g, b, barA);
        drawDiamond(x + w + slide + 3.0f, y + h * 0.5f,
                    3.0f + 2.0f * hover, r, g, b,
                    (0.42f + 0.32f * hover) * alpha);
    }
}

struct SceneTextCommand {
    TextRenderer* renderer = nullptr;
    std::wstring text;
    float x = 0.0f, y = 0.0f, scale = 1.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f, alpha = 1.0f;
    float shadowAlpha = 0.68f;
};

}

float UiTextScale(TextRenderer& renderer, UiTextLevel level,
                  float responsiveScale) {
    constexpr float kReferenceHeight = 46.0f;
    constexpr float kTextRatios[] = { 1.0f, 0.82f, 0.56f, 0.42f };
    const int levelIndex = static_cast<int>(level);
    const float pixels = kReferenceHeight * kTextRatios[levelIndex]
                       * std::max(0.0f, responsiveScale);
    const float lineHeight = renderer.LineHeightPixels();
    return lineHeight > 0.0f ? pixels / lineHeight : responsiveScale;
}

float MaxLocalizedTextWidth(TextRenderer& renderer,
                            const wchar_t* const* variants, int count,
                            float scale) {
    float width = 0.0f;
    if (!variants) return width;
    for (int i = 0; i < count; ++i)
        if (variants[i]) width = std::max(width, renderer.Width(variants[i], scale));
    return width;
}

namespace {

// SHOP has a large amount of late-drawn CircleTexture geometry.  Queueing its
// text lets the page finish all background/icon work first, then emits one
// final unlit text pass above that geometry.
static bool g_DeferSceneText = false;
static std::vector<SceneTextCommand> g_SceneTextCommands;

static void DrawShadowedTextImmediate(TextRenderer& renderer, const wchar_t* text,
                                      float x, float y, float scale,
                                      float r, float g, float b, float alpha,
                                      float shadowAlpha) {
    if (!text || alpha <= 0.001f) return;

    // Text is a UI signal, not part of the scene's light field.  The old
    // four-direction keyline drew almost a complete second glyph around every
    // label, which made small text look dirty and noticeably darker over the
    // page textures.  Keep only a restrained offset shadow for separation;
    // the actual label is still emitted last at its requested colour/alpha.
    const float shadowOffset = std::max(0.60f, std::min(1.00f, scale * 0.72f));
    const float readableShadowAlpha = std::min(0.16f,
                                               alpha * shadowAlpha * 0.24f);
    renderer.Draw(text, x + shadowOffset, y + shadowOffset, scale,
                  0.0f, 0.0f, 0.012f, readableShadowAlpha);
    renderer.Draw(text, x, y, scale, r, g, b, alpha);
}


static void DrawMenuCommandFeedback(float x, float y, float w, float h,
                                    float r, float g, float b, float alpha,
                                    float active, float pulse, float now,
                                    PanelButtonSlideSide slideSide =
                                        PanelButtonSlideSide::Both) {
    if (active <= 0.01f || alpha <= 0.001f) return;
    BindMainShader();
    const float lineH = 20.0f + 42.0f * active + 10.0f * pulse;
    const float barA = (0.24f + 0.55f * active + 0.12f * pulse) * alpha;
    const bool showLeft = slideSide == PanelButtonSlideSide::Left
                       || slideSide == PanelButtonSlideSide::Both;
    const bool showRight = slideSide == PanelButtonSlideSide::Right
                        || slideSide == PanelButtonSlideSide::Both;
    if (showLeft || showRight) {
        // Bottom commands use paired slidebars. Their offset grows with the
        // eased hover value, making the bars visibly travel outward instead
        // of appearing as static brackets.
        const float slide = 10.0f + 14.0f * active + 3.0f * pulse;
        if (showLeft) {
            drawRect(x - slide, y + h * 0.5f - lineH * 0.5f,
                     2.2f, lineH, r, g, b, barA);
            drawDiamond(x - slide - 6.0f, y + h * 0.5f,
                        3.0f + 2.6f * active + 1.8f * pulse,
                        r, g, b,
                        (0.40f + 0.34f * active + 0.16f * pulse) * alpha);
        }
        if (showRight) {
            drawRect(x + w + slide - 2.2f, y + h * 0.5f - lineH * 0.5f,
                     2.2f, lineH, r, g, b, barA);
            drawDiamond(x + w + slide + 4.0f, y + h * 0.5f,
                        3.0f + 2.6f * active + 1.8f * pulse,
                        r, g, b,
                        (0.40f + 0.34f * active + 0.16f * pulse) * alpha);
        }
    }
    if (slideSide != PanelButtonSlideSide::None) {
        const float scanY = y + h * (0.34f + 0.32f * fmodf(now * 0.55f, 1.0f));
        drawRect(x - 4.0f, scanY, w * (0.58f + 0.24f * active), 1.1f,
                 r, g, b, (0.050f + 0.090f * active) * alpha);
    }
}

static void DrawPanelButtonCore(const wchar_t* label,
                                float x, float y, float w, float h,
                                float r, float g, float b, float alpha,
                                float hover, bool selected, float pulse, float now,
                                float responsiveScale = 1.0f,
                                bool keepTextReadable = false,
                                PanelButtonSlideSide slideSide =
                                    PanelButtonSlideSide::Both,
                                bool centerText = false,
                                const wchar_t* const* labelVariants = nullptr,
                                int labelVariantCount = 0) {
    const float active = std::max(hover, selected ? 1.0f : 0.0f);
    DrawMenuCommandFeedback(x, y, w, h, r, g, b, alpha,
                            active, pulse, now, slideSide);

    const wchar_t* text = label ? label : L"";
    float textScale = UiTextScale(g_TextL, UiTextLevel::Title,
                                  responsiveScale);
    const float availableW = std::max(1.0f, w - 16.0f);
    const float availableH = std::max(1.0f, h - 8.0f);
    const float widestText = labelVariants
        ? MaxLocalizedTextWidth(g_TextL, labelVariants,
                                labelVariantCount, textScale)
        : g_TextL.Width(text, textScale);
    const float textHeight = g_TextL.Height(text, textScale);
    const float fit = std::min(1.0f,
        std::min(widestText > 0.0f ? availableW / widestText : 1.0f,
                 textHeight > 0.0f ? availableH / textHeight : 1.0f));
    textScale *= fit;
    const float textW = g_TextL.Width(text, textScale);
    const float textH = g_TextL.Height(text, textScale);
    const float textY = y + std::max(0.0f, (h - textH) * 0.5f);
    float tr = 1.0f + (r - 1.0f) * active;
    float tg = 1.0f + (g - 1.0f) * active;
    float tb = 1.0f + (b - 1.0f) * active;
    if (keepTextReadable && !selected) {
        // SHOP keeps every route legible at once. Its accent still survives,
        // but inactive tabs no longer read like disabled/transparent rows.
        constexpr float kInactiveTextLift = 0.72f;
        tr = r + (1.0f - r) * kInactiveTextLift;
        tg = g + (1.0f - g) * kInactiveTextLift;
        tb = b + (1.0f - b) * kInactiveTextLift;
    }
    if (selected) tr = tg = tb = 1.0f;

    const float textX = centerText
        ? x + (w - textW) * 0.5f : x + 8.0f;
    DrawShadowedText(g_TextL, text, textX, textY, textScale,
                     tr, tg, tb,
                     (keepTextReadable
                          ? (0.98f + 0.06f * active + 0.10f * pulse)
                          : (0.90f + 0.10f * active + 0.16f * pulse)) * alpha,
                     0.86f);
}


}

float UpdateMenuCommandHover(float current, bool hovered, float dt) {
    const float step = std::min(1.0f, dt * 10.0f);
    return current + ((hovered ? 1.0f : 0.0f) - current) * step;
}

void DrawShadowedText(TextRenderer& renderer, const wchar_t* text,
                             float x, float y, float scale,
                             float r, float g, float b, float alpha,
                             float shadowAlpha) {
    if (!text || alpha <= 0.001f) return;
    if (g_DeferSceneText) {
        SceneTextCommand cmd;
        cmd.renderer = &renderer;
        cmd.text = text;
        cmd.x = x; cmd.y = y; cmd.scale = scale;
        cmd.r = r; cmd.g = g; cmd.b = b;
        cmd.alpha = alpha;
        cmd.shadowAlpha = shadowAlpha;
        g_SceneTextCommands.emplace_back(std::move(cmd));
        return;
    }
    DrawShadowedTextImmediate(renderer, text, x, y, scale,
                              r, g, b, alpha, shadowAlpha);
}

void FlushSceneTextCommands() {
    if (g_SceneTextCommands.empty()) return;
    const bool wasDeferred = g_DeferSceneText;
    g_DeferSceneText = false;
    for (const SceneTextCommand& cmd : g_SceneTextCommands) {
        if (cmd.renderer && !cmd.text.empty()) {
            DrawShadowedTextImmediate(*cmd.renderer, cmd.text.c_str(),
                                      cmd.x, cmd.y, cmd.scale,
                                      cmd.r, cmd.g, cmd.b,
                                      cmd.alpha, cmd.shadowAlpha);
        }
    }
    g_SceneTextCommands.clear();
    g_DeferSceneText = wasDeferred;
}


void BeginDeferredSceneText() {
    g_SceneTextCommands.clear();
    g_DeferSceneText = true;
}

void EndDeferredSceneText() {
    FlushSceneTextCommands();
    g_DeferSceneText = false;
}

// Canonical panel-button surface. Scene code should use this entry point
// instead of assembling a frame, feedback bars, and text independently.
void DrawPanelButton(const wchar_t* label, float x, float y, float w, float h,
                            float r, float g, float b, float alpha,
                            float hover, bool selected, float pulse, float now,
                            float responsiveScale,
                            bool keepTextReadable,
                            PanelButtonSlideSide slideSide,
                            bool centerText,
                            const wchar_t* const* labelVariants,
                            int labelVariantCount) {
    DrawPanelButtonCore(label, x, y, w, h, r, g, b, alpha,
                        hover, selected, pulse, now,
                        responsiveScale, keepTextReadable,
                        slideSide, centerText,
                        labelVariants, labelVariantCount);
}

// Compatibility alias for older scene paths. All calls still flow through
// DrawPanelButton, so there is only one visual implementation to maintain.
void DrawUnifiedMenuCommand(const wchar_t* label,
                                   float x, float y, float w, float h,
                                   float r, float g, float b, float alpha,
                                   float hover, bool selected, float pulse, float now,
                                   float responsiveScale,
                                   bool keepTextReadable,
                                   bool bothSides,
                                   bool centerText,
                                   const wchar_t* const* labelVariants,
                                   int labelVariantCount) {
    const PanelButtonSlideSide slideSide = bothSides
        ? PanelButtonSlideSide::Both : PanelButtonSlideSide::Left;
    DrawPanelButton(label, x, y, w, h, r, g, b, alpha,
                    hover, selected, pulse, now,
                    responsiveScale, keepTextReadable,
                    slideSide, centerText,
                    labelVariants, labelVariantCount);
}

bool PanelButtonHit(double mx, double my,
                           float x, float y, float w, float h,
                           float hitPadding) {
    return mx >= x - hitPadding && mx <= x + w + hitPadding
        && my >= y - hitPadding && my <= y + h + hitPadding;
}

bool UpdatePanelButtonHover(float& hover, bool enabled,
                                   double mx, double my,
                                   float x, float y, float w, float h,
                                   float dt, float hitPadding) {
    const bool hovered = enabled && PanelButtonHit(mx, my, x, y, w, h,
                                                    hitPadding);
    hover = UpdateMenuCommandHover(hover, hovered, dt);
    return hovered;
}

bool DrawBottomLeftBackButton(float& hover, float sw, float sh, float uiScale,
                              double mx, double my, bool lmb, bool lmbPrev,
                              float alpha, float now, float dt) {
    (void)sw;
    const float w = 286.0f * uiScale, h = 64.0f * uiScale;
    const float x = BottomLeftActionX(uiScale);
    const float y = BottomLeftActionY(sh, h, uiScale);
    const bool hov = UpdatePanelButtonHover(hover, true, mx, my, x, y, w, h, dt, 0.0f);
    DrawUnifiedMenuCommand(T(StrId::BTN_BACK), x, y, w, h,
                           0.34f, 0.72f, 1.0f, alpha,
                           hover, hov, 0.0f, now,
                           uiScale, true, true, true,
                           kStrings[(int)StrId::BTN_BACK], LANG_COUNT);
    return hov && lmb && !lmbPrev;
}

float CenterTextX(float containerW, TextRenderer& tr, const wchar_t* text, float scale) {
    return CenterX(containerW, tr.Width(text, scale));
}

bool UIButton(float x, float y, float w, float h, const wchar_t* label,
              double mx, double my, bool lmb, bool lmbPrev, bool selected,
              PanelButtonSlideSide slideSide, float responsiveScale,
              const wchar_t* const* labelVariants, int labelVariantCount) {
    BindMainShader();

    bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
    UIButtonMotion& motion = MotionForButton(x, y, w, h);
    const double now = glfwGetTime();
    const float dt = motion.lastTime > 0.0
        ? std::min(0.05f, std::max(0.0f, (float)(now - motion.lastTime)))
        : (1.0f / 60.0f);
    motion.lastTime = now;
    const float step = std::min(1.0f, dt * 10.0f);
    motion.hover += ((hover ? 1.0f : 0.0f) - motion.hover) * step;
    const float active = std::max(motion.hover, selected ? 1.0f : 0.0f);

    const float washA = selected ? 0.10f : (0.035f * motion.hover);
    if (washA > 0.0f)
        drawRect(x, y, w, h, 0.10f, 0.26f, 0.42f, washA);
    DrawUIButtonFeedback(x, y, w, h, motion.hover,
                         0.38f, 0.82f, 1.0f, 1.0f, slideSide);

    const wchar_t* text = label ? label : L"";
    float sc = UiTextScale(g_TextL, UiTextLevel::Title, responsiveScale);
    const float widest = labelVariants
        ? MaxLocalizedTextWidth(g_TextL, labelVariants, labelVariantCount, sc)
        : g_TextL.Width(text, sc);
    const float labelHeight = g_TextL.Height(text, sc);
    const float widthFit = widest > 0.0f
        ? std::max(0.0f, w - 16.0f) / widest : 1.0f;
    const float heightFit = labelHeight > 0.0f
        ? std::max(0.0f, h - 8.0f) / labelHeight : 1.0f;
    sc *= std::min(1.0f, std::min(widthFit, heightFit));
    float lw = g_TextL.Width(text, sc);
    float lh = g_TextL.Height(text, sc);
    const float tr = selected ? 1.0f : 0.76f + 0.24f * active;
    const float tg = selected ? 1.0f : 0.82f + 0.18f * active;
    const float tb = selected ? 1.0f : 0.92f + 0.08f * active;
    const float ta = selected ? 1.0f : 0.82f + 0.16f * active;
    g_TextL.Draw(text,
                 x + (w - lw) * 0.5f,
                 y + (h - lh) * 0.5f,
                 sc, tr, tg, tb, ta);

    return hover && lmbPrev && !lmb;
}

const ConfirmText& ConfirmTextQuit() {
    static const ConfirmText t = {
        { L"게임을 종료할까요?", L"EXIT THE GAME?", L"ゲームを終了しますか？" },
        { L"종료하면 바탕화면으로 돌아갑니다.", L"YOU WILL RETURN TO THE DESKTOP.",
          L"デスクトップに戻ります。" } };
    return t;
}

const ConfirmText& ConfirmTextRestart() {
    static const ConfirmText t = {
        { L"다시 시작할까요?", L"RESTART THIS PLAY?", L"最初からやり直しますか？" },
        { L"새 플레이를 처음부터 시작합니다.", L"A NEW PLAY STARTS FROM THE BEGINNING.",
          L"新しいプレイを最初から始めます。" } };
    return t;
}

const ConfirmText& ConfirmTextAbandon() {
    static const ConfirmText t = {
        { L"플레이를 포기할까요?", L"ABANDON THIS PLAY?", L"プレイを放棄しますか？" },
        { L"지금까지의 진행이 종료됩니다.", L"YOUR CURRENT PROGRESS ENDS HERE.",
          L"ここまでの進行が終了します。" } };
    return t;
}

ConfirmChoice DrawConfirmDialog(bool open, const ConfirmText& text,
                                float sw, float sh, double mx, double my,
                                bool lmb, bool lmbPrev, float dt) {
    static float s_openT = 0.0f;
    static float s_hover[2] = {};
    if (!open) {
        s_openT = 0.0f;
        s_hover[0] = s_hover[1] = 0.0f;
        return ConfirmChoice::None;
    }
    s_openT += dt;
    const float fade = std::min(1.0f, s_openT / 0.16f);
    const float now = (float)glfwGetTime();
    const int li = std::max(0, std::min(2, LangIndex()));
    const float uiS = UiScale(sw, sh);

    BindMainShader();
    drawRect(0.0f, 0.0f, sw, sh, 0.01f, 0.02f, 0.04f, 0.62f * fade);

    const float cx = sw * 0.5f;
    const float cy = sh * 0.5f;
    const wchar_t* title = text.title[li];
    const wchar_t* prompt = text.prompt[li];
    const float titleScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
    const float promptScale = UiTextScale(g_TextL, UiTextLevel::Description, uiS);
    const float titleY = cy - 92.0f * uiS;
    DrawShadowedText(g_TextL, title,
                     cx - g_TextL.Width(title, titleScale) * 0.5f, titleY,
                     titleScale, 1.0f, 1.0f, 1.0f, 0.98f * fade, 0.72f);
    DrawShadowedText(g_TextL, prompt,
                     cx - g_TextL.Width(prompt, promptScale) * 0.5f,
                     titleY + g_TextL.Height(title, titleScale) + 10.0f * uiS,
                     promptScale, 0.60f, 0.72f, 0.86f, 0.82f * fade, 0.58f);

    static const wchar_t* kYes[3] = { L"네", L"YES", L"はい" };
    static const wchar_t* kNo[3] = { L"아니오", L"NO", L"いいえ" };
    const float bw = 210.0f * uiS, bh = 48.0f * uiS, gap = 56.0f * uiS;   // hover bars slide ~21px out of each side
    const float by = cy + 26.0f * uiS;
    const float bx[2] = { cx - bw - gap, cx + gap };
    ConfirmChoice result = ConfirmChoice::None;
    for (int i = 0; i < 2; ++i) {
        const bool hov = PanelButtonHit(mx, my, bx[i], by, bw, bh);
        s_hover[i] = UpdateMenuCommandHover(s_hover[i], hov, dt);
        const wchar_t* variants[3] = {
            i == 0 ? kYes[0] : kNo[0], i == 0 ? kYes[1] : kNo[1],
            i == 0 ? kYes[2] : kNo[2] };
        // YES is the destructive answer, so it carries the warning hue.
        const bool yes = i == 0;
        DrawUnifiedMenuCommand(variants[li], bx[i], by, bw, bh,
                               yes ? 1.0f : 0.34f, yes ? 0.32f : 0.82f,
                               yes ? 0.28f : 1.0f,
                               fade, s_hover[i], false, 0.0f,
                               now + 0.2f * (float)i, uiS, true, true, true,
                               variants, 3);
        // The click that opened the dialog must not answer it.
        if (hov && lmb && !lmbPrev && s_openT > 0.12f)
            result = yes ? ConfirmChoice::Yes : ConfirmChoice::No;
    }
    return result;
}
