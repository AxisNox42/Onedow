#include "SceneUI.h"
#include "DrawPrim.h"
#include "TextRenderer.h"
#include "UiLayout.h"

extern TextRenderer g_TextL;

float CenterTextX(float containerW, TextRenderer& tr, const wchar_t* text, float scale) {
    return CenterX(containerW, tr.Width(text, scale));
}

bool UIButton(float x, float y, float w, float h, const wchar_t* label,
              double mx, double my, bool lmb, bool lmbPrev, bool selected) {
    BindMainShader();

    bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);

    float br = selected ? 0.32f : (hover ? 0.22f : 0.10f);
    float bg = selected ? 0.32f : (hover ? 0.22f : 0.10f);
    float bb = selected ? 0.40f : (hover ? 0.28f : 0.14f);
    drawRect(x, y, w, h, br, bg, bb, 0.92f);

    float er = hover ? 0.95f : (selected ? 0.75f : 0.45f);
    float eg = hover ? 0.95f : (selected ? 0.75f : 0.45f);
    float eb = hover ? 1.00f : (selected ? 0.95f : 0.55f);
    drawRect(x,         y,         w, 2.0f, er, eg, eb, 1.0f);
    drawRect(x,         y + h - 2, w, 2.0f, er, eg, eb, 1.0f);
    drawRect(x,         y,    2.0f, h, er, eg, eb, 1.0f);
    drawRect(x + w - 2, y,    2.0f, h, er, eg, eb, 1.0f);

    float sc = 0.9f;
    while (sc > 0.55f && g_TextL.Width(label, sc) > w - 16.0f) sc -= 0.05f;
    float lw = g_TextL.Width(label, sc);
    float lh = g_TextL.Height(label, sc);
    g_TextL.Draw(label,
                 x + (w - lw) * 0.5f,
                 y + (h - lh) * 0.5f,
                 sc, 1.0f, 1.0f, 1.0f, 0.98f);

    return hover && lmbPrev && !lmb;
}
