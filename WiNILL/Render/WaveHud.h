#pragma once
// ─────────────────────────────────────────────────────────────
// Wave HUD (top centre): "WAVE 04 / 15" and the time left in the wave,
// and a seven-node window of the wave track - three past waves, the current
// wave fixed in the centre, three upcoming waves (symbols only, no labels).
// The window's end nodes are faded to say the track goes on; the run's
// first and final waves are never faded because nothing lies beyond them.
// Line glyphs only; text keeps the renderer halo so
// it reads on a bright wallpaper without a plate.
//   ○ plain wave   ◇ elite wave   ◈ final wave
// A fifth wave counts as elite only when its scheduled enemy is an elite
// kind (IsEliteMob); EASY's Regulus/Antares waves stay plain.
// ─────────────────────────────────────────────────────────────
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "DrawPrim.h"
#include "HudLines.h"
#include "EnemySpawner.h"
#include "GameManager.h"
#include "Monster.h"
#include "SceneUI.h"
#include "Settings.h"
#include "TextRenderer.h"
#include "WaveSystem.h"

extern TextRenderer g_TextL;
extern TextRenderer g_TextS;

namespace wave_hud {

using namespace hud_lines;

inline constexpr Rgb kPast   = { 0.47f, 0.87f, 1.00f };
inline constexpr Rgb kNext   = { 0.66f, 0.70f, 0.80f };
inline constexpr Rgb kTrack  = { 0.36f, 0.41f, 0.53f };
inline constexpr Rgb kElite  = { 1.00f, 0.57f, 0.25f };
inline constexpr Rgb kFinal  = { 1.00f, 0.81f, 0.38f };
inline constexpr Rgb kDanger = { 1.00f, 0.36f, 0.38f };
inline constexpr Rgb kText   = { 0.86f, 0.91f, 1.00f };

inline bool IsEliteWave(int wave, Difficulty difficulty) {
    const ScheduledEliteInfo info = GetScheduledElite(wave, difficulty);
    return info.scheduled && IsEliteMob(info.kind);
}

inline void HudText(TextRenderer& font, const wchar_t* text, float x,
                     float baseline, float scale, Rgb c, float a) {
    font.Draw(text, x, baseline - font.BaselineOffset(scale), scale, c.r, c.g, c.b, a);
}

}  // namespace wave_hud

inline void DrawWaveHud(float sw, float hudScale, float hudTopY, float gameTime,
                        Difficulty difficulty, const GameManager& gm) {
    using namespace wave_hud;
    const float hs = hudScale;
    const int li = std::max(0, std::min(2, LangIndex()));
    const int finalWave = FinalWaveCount(difficulty);
    const int wave = CurrentWaveNumber(gameTime, difficulty);
    const float remain = CurrentWaveSecondsRemaining(gameTime, difficulty);
    const bool lastSeconds = remain > 0.0f && remain <= 10.0f;
    const float keyline = 3.2f * hs;
    const float cx = sw * 0.5f;

    // ── header: WAVE 04 / 15   02:10 ──
    static const wchar_t* kWaveLabel[3] = { L"WAVE", L"WAVE", L"WAVE" };
    wchar_t number[8], total[16], timer[16];
    swprintf_s(number, L"%02d", wave);
    swprintf_s(total, L"/ %d", finalWave);
    const int remainWhole = (int)std::ceil(remain);
    swprintf_s(timer, L"%02d:%02d", remainWhole / 60, remainWhole % 60);
    const float labelSc = UiTextScale(g_TextS, UiTextLevel::Supporting, hs * 0.80f);
    const float numberSc = UiTextScale(g_TextL, UiTextLevel::Subtitle, hs * 0.74f);
    const float totalSc = UiTextScale(g_TextS, UiTextLevel::Description, hs * 0.78f);
    const float timerSc = UiTextScale(g_TextL, UiTextLevel::Subtitle,
                                      hs * (lastSeconds ? 0.76f : 0.60f));
    const float labelW = g_TextS.Width(kWaveLabel[li], labelSc);
    const float numberW = g_TextL.Width(number, numberSc);
    const float totalW = g_TextS.Width(total, totalSc);
    const float timerW = g_TextL.Width(timer, timerSc);
    const float gapS = 6.0f * hs, gapL = 20.0f * hs;
    const float headerW = labelW + gapS + numberW + gapS + totalW + gapL + timerW;
    const float baseline = hudTopY + g_TextL.BaselineOffset(numberSc) + 2.0f * hs;
    float x = cx - headerW * 0.5f;
    HudText(g_TextS, kWaveLabel[li], x, baseline, labelSc, kNext, 0.92f);
    x += labelW + gapS;
    HudText(g_TextL, number, x, baseline, numberSc, kText, 1.0f);
    x += numberW + gapS;
    HudText(g_TextS, total, x, baseline, totalSc, kNext, 0.92f);
    x += totalW + gapL;
    HudText(g_TextL, timer, x, baseline, timerSc,
             lastSeconds ? kDanger : kPast, 1.0f);

    // ── track ──
    BindMainShader();
    const float trackY = baseline + 22.0f * hs;
    constexpr int kWindowSide = 3;
    const int first = std::max(1, wave - kWindowSide);
    const int last = std::min(finalWave, wave + kWindowSide);
    const float gap = 30.0f * hs;
    auto nodeX = [&](int w) { return cx + gap * (float)(w - wave); };
    // Window ends fade unless they are the run's real ends.
    auto fade = [&](int w) {
        const bool windowEnd = w == wave - kWindowSide || w == wave + kWindowSide;
        return windowEnd && w != 1 && w != finalWave ? 0.40f : 1.0f;
    };
    const float lineW = 1.5f * hs;
    // Half-width of each node along the track, so the links stop at the
    // node's edge instead of running through it.
    auto nodeHalf = [&](int w) {
        const bool isFinal = w == finalWave;
        const bool isElite = !isFinal && IsEliteWave(w, difficulty);
        if (w == wave) return (isFinal || isElite ? 13.0f : 10.0f) * hs + 1.2f * hs;
        if (isFinal) return 9.0f * hs + 0.9f * hs;
        if (isElite) return 7.0f * hs + 0.9f * hs;
        return (w < wave ? 3.0f : 4.0f) * hs + 0.7f * hs;
    };
    const float linkPad = 3.0f * hs;
    for (int w = first; w < last; ++w) {
        const float a = nodeX(w) + nodeHalf(w) + linkPad;
        const float b = nodeX(w + 1) - nodeHalf(w + 1) - linkPad;
        if (b <= a) continue;
        const bool passed = w + 1 <= wave;
        const float f = std::min(fade(w), fade(w + 1));
        Segment(a, trackY, b, trackY, lineW + keyline, KeyCol(passed ? kPast : kTrack), kKeyA * f);
        Segment(a, trackY, b, trackY, passed ? lineW : lineW * 0.8f,
                passed ? kPast : kTrack, (passed ? 0.85f : 0.80f) * f);
    }

    const float frac = std::clamp(remain / WAVE_DURATION_SECONDS, 0.0f, 1.0f);
    for (int w = first; w <= last; ++w) {
        const float nx = nodeX(w);
        const float f = fade(w);
        const bool isFinal = w == finalWave;
        const bool isElite = !isFinal && IsEliteWave(w, difficulty);
        if (w == wave) {
            // Current wave: a steady outline in the node's own colour, and
            // its interior filling clockwise from 12 o'clock as the wave
            // runs - a disc on plain waves, the diamond on elite/final ones.
            const bool special = isFinal || isElite;
            const Rgb own = isFinal ? kFinal : (isElite ? kElite : kPast);
            const float r = special ? 13.0f * hs : 10.0f * hs;
            const float stroke = 2.0f * hs;
            const float pulse = gm.waveTransitionTimer > 0.0f
                ? std::clamp((1.0f - gm.waveTransitionTimer / 2.0f) / 0.6f, 0.0f, 1.0f)
                : 1.0f;
            constexpr float kFillA = 0.85f;
            if (special) {
                DiamondOutline(nx, trackY, r, stroke, own, 1.0f, keyline);
                // The final wave keeps its double frame; the fill sits inside.
                float fillR = r - stroke * 1.4f - 1.5f * hs;
                if (isFinal) {
                    const float innerR = r - 5.0f * hs;
                    DiamondOutline(nx, trackY, innerR, 1.1f * hs, own, 0.9f, 0.0f);
                    fillR = innerR - 1.1f * hs * 1.4f - 1.5f * hs;
                }
                SectorFill(nx, trackY, fillR, FillShape::Diamond, 1.0f - frac, own, kFillA);
                if (pulse < 1.0f)
                    DiamondOutline(nx, trackY, r + 16.0f * hs * pulse, 1.4f * hs, own,
                                   0.9f * (1.0f - pulse), 0.0f);
            } else {
                Ring(nx, trackY, r, stroke, own, 1.0f, keyline);
                SectorFill(nx, trackY, r - stroke * 0.5f - 1.5f * hs, FillShape::Circle,
                           1.0f - frac, own, kFillA);
                if (pulse < 1.0f)
                    Ring(nx, trackY, r + 18.0f * hs * pulse, 1.4f * hs, kPast,
                         0.9f * (1.0f - pulse), 0.0f);
            }
        } else if (isFinal) {
            DiamondOutline(nx, trackY, 9.0f * hs, 1.7f * hs, kFinal, f, keyline);
            DiamondOutline(nx, trackY, 4.5f * hs, 1.1f * hs, kFinal, 0.9f * f, 0.0f);
        } else if (isElite) {
            DiamondOutline(nx, trackY, 7.0f * hs, 1.7f * hs,
                           w < wave ? kPast : kElite, (w < wave ? 0.85f : 1.0f) * f, keyline);
        } else if (w < wave) {
            Dot(nx, trackY, 3.0f * hs, kPast, 0.95f * f, keyline);
        } else {
            Ring(nx, trackY, 4.0f * hs, 1.3f * hs, kNext, 0.85f * f, keyline);
        }
    }
    BatchFlush();
}
