#include "SceneUI.h"
#include "DrawPrim.h"
#include "TextRenderer.h"
#include "UiLayout.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

extern TextRenderer g_TextL;

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

}

float CenterTextX(float containerW, TextRenderer& tr, const wchar_t* text, float scale) {
    return CenterX(containerW, tr.Width(text, scale));
}

bool UIButton(float x, float y, float w, float h, const wchar_t* label,
              double mx, double my, bool lmb, bool lmbPrev, bool selected,
              PanelButtonSlideSide slideSide) {
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

    float sc = 0.9f;
    const wchar_t* text = label ? label : L"";
    while (sc > 0.55f && g_TextL.Width(text, sc) > w - 16.0f) sc -= 0.05f;
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
