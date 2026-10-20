#pragma once
// ─────────────────────────────────────────────────────────────
// Wave HUD (top centre): "04 / 15" and the time left in the wave,
// and a seven-node window of the wave track - three past waves, the current
// wave fixed in the centre, three upcoming waves (symbols only, no labels).
// The window's end nodes are faded to say the track goes on; the run's
// first and final waves are never faded because nothing lies beyond them.
// Line glyphs only; text keeps the renderer halo so
// it reads on a bright wallpaper without a plate.
//   ○ plain wave   ◇ elite wave   ◈ final wave
// Every fifth non-boss wave is an elite wave (Quasar, Gravis or Genesis,
// rolled at run start); the final wave is reserved for the boss encounter.
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

// The seven-node wave track around (cx, trackY): three past waves, the current
// wave fixed in the centre, three upcoming ones. remainFrac is how much of the
// current wave is left (its node fills clockwise as it runs); pulse runs 0..1
// for the arrival ring (1 = none). slide shifts the nodes right by that many
// gaps so the track can glide when the current wave changes, and alphaMul
// fades the whole track, and fillPassedSpecial fills finished elite waves in.
// With the defaults this is the in-game HUD.
inline void DrawWaveTrack(float cx, float trackY, float hs, int wave, int finalWave,
                          Difficulty difficulty, float remainFrac, float pulse,
                          float slide = 0.0f, float alphaMul = 1.0f,
                          bool fillPassedSpecial = false, bool failCurrent = false) {
    using namespace wave_hud;
    const float keyline = 3.2f * hs;
    const float am = alphaMul;
    BindMainShader();
    constexpr int kWindowSide = 3;
    const int first = std::max(1, wave - kWindowSide - 1);
    const int last = std::min(finalWave, wave + kWindowSide + 1);
    const float gap = 30.0f * hs;
    auto posOf = [&](int w) { return (float)(w - wave) + slide; };
    auto nodeX = [&](int w) { return cx + gap * posOf(w); };
    // Window ends fade unless they are the run's real ends. A node sliding in
    // or out past the seventh slot fades away within a short stretch, so at
    // most seven are ever visible.
    auto fade = [&](int w) {
        const float d = std::fabs(posOf(w));
        float f = d <= 2.0f ? 1.0f
                : d <= 3.0f ? 1.0f - 0.6f * (d - 2.0f)
                : std::max(0.0f, 0.4f * (1.0f - (d - 3.0f) / 0.4f));
        if (w == 1 || w == finalWave)
            f = std::max(f, d <= 3.0f ? 1.0f : std::max(0.0f, 1.0f - (d - 3.0f) / 0.4f));
        return f * am;
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
        if (f <= 0.001f) continue;
        Segment(a, trackY, b, trackY, lineW + keyline, KeyCol(passed ? kPast : kTrack), kKeyA * f);
        Segment(a, trackY, b, trackY, passed ? lineW : lineW * 0.8f,
                passed ? kPast : kTrack, (passed ? 0.85f : 0.80f) * f);
    }

    const float frac = std::clamp(remainFrac, 0.0f, 1.0f);
    for (int w = first; w <= last; ++w) {
        const float nx = nodeX(w);
        const float f = fade(w);
        if (f <= 0.001f) continue;
        const bool isFinal = w == finalWave;
        const bool isElite = !isFinal && IsEliteWave(w, difficulty);
        if (w == wave) {
            // Current wave: a steady outline in the node's own colour, and
            // its interior filling clockwise from 12 o'clock as the wave
            // runs - a disc on plain waves, the diamond on elite/final ones.
            const bool special = isFinal || isElite;
            const Rgb own = failCurrent ? kDanger : (isFinal ? kFinal : (isElite ? kElite : kPast));
            const float r = special ? 13.0f * hs : 10.0f * hs;
            const float stroke = 2.0f * hs;
            constexpr float kFillA = 0.85f;
            if (special) {
                DiamondOutline(nx, trackY, r, stroke, own, f, keyline);
                // The final wave keeps its double frame; the fill sits inside.
                float fillR = r - stroke * 1.4f - 1.5f * hs;
                if (isFinal) {
                    const float innerR = r - 5.0f * hs;
                    DiamondOutline(nx, trackY, innerR, 1.1f * hs, own, 0.9f * f, 0.0f);
                    fillR = innerR - 1.1f * hs * 1.4f - 1.5f * hs;
                }
                SectorFill(nx, trackY, fillR, FillShape::Diamond, 1.0f - frac, own, kFillA * f);
                if (pulse < 1.0f)
                    DiamondOutline(nx, trackY, r + 16.0f * hs * pulse, 1.4f * hs, own,
                                   0.9f * (1.0f - pulse) * f, 0.0f);
            } else {
                Ring(nx, trackY, r, stroke, own, f, keyline);
                SectorFill(nx, trackY, r - stroke * 0.5f - 1.5f * hs, FillShape::Circle,
                           1.0f - frac, own, kFillA * f);
                if (pulse < 1.0f)
                    Ring(nx, trackY, r + 18.0f * hs * pulse, 1.4f * hs, kPast,
                         0.9f * (1.0f - pulse) * f, 0.0f);
            }
        } else if (isFinal) {
            DiamondOutline(nx, trackY, 9.0f * hs, 1.7f * hs, kFinal, f, keyline);
            DiamondOutline(nx, trackY, 4.5f * hs, 1.1f * hs, kFinal, 0.9f * f, 0.0f);
        } else if (isElite) {
            DiamondOutline(nx, trackY, 7.0f * hs, 1.7f * hs,
                           w < wave ? kPast : kElite, (w < wave ? 0.85f : 1.0f) * f, keyline);
            // The report shows finished elite waves filled in, like the plain ones.
            if (fillPassedSpecial && w < wave)
                SectorFill(nx, trackY, 3.6f * hs, FillShape::Diamond, 1.0f, kPast, 0.85f * f);
        } else if (w < wave) {
            Dot(nx, trackY, 3.0f * hs, kPast, 0.95f * f, keyline);
        } else {
            Ring(nx, trackY, 4.0f * hs, 1.3f * hs, kNext, 0.85f * f, keyline);
        }
    }
    BatchFlush();
}

inline void DrawWaveHud(float sw, float hudScale, float hudTopY, float gameTime,
                        Difficulty difficulty, const GameManager& gm) {
    using namespace wave_hud;
    const float hs = hudScale;
    const int finalWave = FinalWaveCount(difficulty);
    const int wave = CurrentWaveNumber(gameTime, difficulty);
    const float remain = CurrentWaveSecondsRemaining(gameTime, difficulty);
    const bool lastSeconds = remain > 0.0f && remain <= 10.0f;
    const float keyline = 3.2f * hs;
    const float cx = sw * 0.5f;

    // Two equally sized text groups; the timer keeps its width and font
    // size when it turns red, so the header does not jump at ten seconds.
    // The current wave is full size; the total beside it is smaller.
    wchar_t current[8], total[16], timer[16];
    swprintf_s(current, L"%02d", wave);
    swprintf_s(total, L"/ %02d", finalWave);
    const int remainWhole = (int)std::ceil(remain);
    swprintf_s(timer, L"%02d:%02d", remainWhole / 60, remainWhole % 60);
    const float textSc = UiTextScale(g_TextL, UiTextLevel::Subtitle, hs * 0.95f);
    const float totalSc = textSc * 0.65f;
    const float totalGap = 8.0f * hs;
    const float progressW = std::max(g_TextL.Width(L"88", textSc), g_TextL.Width(current, textSc))
                          + totalGap
                          + std::max(g_TextL.Width(L"/ 88", totalSc), g_TextL.Width(total, totalSc));
    const float timerW = std::max(g_TextL.Width(L"88:88", textSc), g_TextL.Width(timer, textSc));
    const float headerW = progressW + 24.0f * hs + timerW;
    const float baseline = hudTopY + g_TextL.BaselineOffset(textSc) + 4.0f * hs;
    const float x = cx - headerW * 0.5f;
    HudText(g_TextL, current, x, baseline, textSc, kText, 1.0f);
    HudText(g_TextL, total, x + g_TextL.Width(current, textSc) + totalGap, baseline,
            totalSc, kNext, 1.0f);
    HudText(g_TextL, timer, x + progressW + 24.0f * hs, baseline, textSc,
            lastSeconds ? kDanger : kPast, 1.0f);

    // ── track ──
    const float trackY = baseline + 26.0f * hs;
    const float pulse = gm.waveTransitionTimer > 0.0f
        ? std::clamp((1.0f - gm.waveTransitionTimer / 2.0f) / 0.6f, 0.0f, 1.0f)
        : 1.0f;
    DrawWaveTrack(cx, trackY, hs, wave, finalWave, difficulty,
                  std::clamp(remain / WaveDurationSeconds(wave, difficulty),
                             0.0f, 1.0f), pulse);
}
