#pragma once
// ─────────────────────────────────────────────────────────────
// Wave HUD (top centre): "WAVE 04 / 15" and the time left in the wave,
// a track with one node per wave, the next elite's tag and the debuff
// this wave applied. Line glyphs only; text keeps the renderer halo so
// it reads on a bright wallpaper without a plate.
//   ○ plain wave   ◇ elite wave   ◈ final wave
// A fifth wave counts as elite only when its scheduled enemy is an elite
// kind (IsEliteMob); EASY's Regulus/Antares waves stay plain.
// ─────────────────────────────────────────────────────────────
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "Augment.h"
#include "Codex.h"
#include "DrawPrim.h"
#include "EnemySpawner.h"
#include "EntityDraw.h"
#include "GameManager.h"
#include "Monster.h"
#include "SceneUI.h"
#include "Settings.h"
#include "TextRenderer.h"
#include "WaveSystem.h"

extern TextRenderer g_TextL;
extern TextRenderer g_TextS;

namespace wave_hud {

struct Rgb { float r, g, b; };
inline constexpr Rgb kPast   = { 0.47f, 0.87f, 1.00f };
inline constexpr Rgb kNext   = { 0.66f, 0.70f, 0.80f };
inline constexpr Rgb kTrack  = { 0.36f, 0.41f, 0.53f };
inline constexpr Rgb kRing   = { 0.24f, 0.31f, 0.43f };
inline constexpr Rgb kElite  = { 1.00f, 0.57f, 0.25f };
inline constexpr Rgb kFinal  = { 1.00f, 0.81f, 0.38f };
inline constexpr Rgb kDanger = { 1.00f, 0.36f, 0.38f };
inline constexpr Rgb kText   = { 0.86f, 0.91f, 1.00f };
inline constexpr float kKeyA = 0.38f;   // dark keyline under every line glyph
inline constexpr float kPi = 3.14159265f;

inline void Segment(float x0, float y0, float x1, float y1, float w,
                    Rgb c, float a) {
    const float dx = x1 - x0, dy = y1 - y0;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) return;
    const float nx = -dy / len * w * 0.5f, ny = dx / len * w * 0.5f;
    BatchTri(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, c.r, c.g, c.b, a);
    BatchTri(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, c.r, c.g, c.b, a);
}

// Arc from angle a0 sweeping `sweep` radians (screen space, clockwise).
inline void Arc(float cx, float cy, float radius, float thick, float a0,
                float sweep, Rgb c, float a) {
    const int segments = std::max(2, (int)(std::fabs(sweep) / (2.0f * kPi) * 40.0f));
    const float inner = radius - thick * 0.5f, outer = radius + thick * 0.5f;
    for (int i = 0; i < segments; ++i) {
        const float t0 = a0 + sweep * i / segments;
        const float t1 = a0 + sweep * (i + 1) / segments;
        const float c0 = std::cos(t0), s0 = std::sin(t0);
        const float c1 = std::cos(t1), s1 = std::sin(t1);
        BatchTri(cx + c0 * outer, cy + s0 * outer, cx + c1 * outer, cy + s1 * outer,
                 cx + c1 * inner, cy + s1 * inner, c.r, c.g, c.b, a);
        BatchTri(cx + c0 * outer, cy + s0 * outer, cx + c1 * inner, cy + s1 * inner,
                 cx + c0 * inner, cy + s0 * inner, c.r, c.g, c.b, a);
    }
}

inline void Ring(float cx, float cy, float radius, float thick, Rgb c, float a,
                 float keyline) {
    if (keyline > 0.0f)
        Arc(cx, cy, radius, thick + keyline, 0.0f, 2.0f * kPi, {0, 0, 0}, kKeyA * a);
    Arc(cx, cy, radius, thick, 0.0f, 2.0f * kPi, c, a);
}

inline void DiamondOutline(float cx, float cy, float r, float thick, Rgb c,
                           float a, float keyline) {
    const float px[5] = { cx, cx + r, cx, cx - r, cx };
    const float py[5] = { cy - r, cy, cy + r, cy, cy - r };
    // Extend each edge by half the stroke so the corners stay sharp.
    for (int pass = keyline > 0.0f ? 0 : 1; pass < 2; ++pass) {
        const float w = pass == 0 ? thick + keyline : thick;
        const Rgb col = pass == 0 ? Rgb{0, 0, 0} : c;
        const float alpha = pass == 0 ? kKeyA * a : a;
        for (int i = 0; i < 4; ++i) {
            const float dx = px[i + 1] - px[i], dy = py[i + 1] - py[i];
            const float len = std::sqrt(dx * dx + dy * dy);
            const float ex = dx / len * w * 0.5f, ey = dy / len * w * 0.5f;
            Segment(px[i] - ex, py[i] - ey, px[i + 1] + ex, py[i + 1] + ey, w, col, alpha);
        }
    }
}

inline void Dot(float cx, float cy, float r, Rgb c, float a, float keyline) {
    if (keyline > 0.0f) drawCircle(cx, cy, r + keyline * 0.5f, 0, 0, 0, kKeyA * a);
    drawCircle(cx, cy, r, c.r, c.g, c.b, a);
}

inline bool IsEliteWave(int wave, Difficulty difficulty) {
    const ScheduledEliteInfo info = GetScheduledElite(wave, difficulty);
    return info.scheduled && IsEliteMob(info.kind);
}

// Name of a scheduled elite as the Astral Log would show it: the name once
// one has been defeated, ??? when it has only been seen, ? when never met.
inline const wchar_t* EliteName(int codexId) {
    if (CodexMobNameUnlocked(codexId)) return MobName(codexId);
    return CodexMobSeen(codexId) ? L"???" : L"?";
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
    const float waveSeconds = gameTime - (wave - 1) * WAVE_DURATION_SECONDS;
    const bool lastSeconds = remain > 0.0f && remain <= 10.0f;
    const float keyline = 2.0f * hs;
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
    const float gap = finalWave > 1
        ? std::min(26.0f * hs, sw * 0.34f / (float)(finalWave - 1)) : 0.0f;
    const float trackW = gap * (float)(finalWave - 1);
    const float x0 = cx - trackW * 0.5f;
    const float curX = x0 + gap * (float)(wave - 1);
    const float lineW = 1.5f * hs;
    Segment(x0, trackY, x0 + trackW, trackY, lineW + keyline, {0, 0, 0}, kKeyA);
    if (wave > 1) Segment(x0, trackY, curX, trackY, lineW, kPast, 0.85f);
    if (wave < finalWave) Segment(curX, trackY, x0 + trackW, trackY, lineW * 0.8f, kTrack, 0.80f);

    const float frac = std::clamp(remain / WAVE_DURATION_SECONDS, 0.0f, 1.0f);
    for (int w = 1; w <= finalWave; ++w) {
        const float nx = x0 + gap * (float)(w - 1);
        const bool isFinal = w == finalWave;
        const bool isElite = !isFinal && IsEliteWave(w, difficulty);
        if (w == wave) {
            // Current wave: the arc is the time left, draining clockwise.
            const Rgb arcCol = lastSeconds ? kDanger : kPast;
            if (isFinal || isElite) {
                const Rgb c = isFinal ? kFinal : kElite;
                DiamondOutline(nx, trackY, 13.0f * hs, 1.9f * hs, c, 1.0f, keyline);
                if (isFinal) DiamondOutline(nx, trackY, 6.0f * hs, 1.1f * hs, c, 0.9f, 0.0f);
            }
            const float r = (isFinal || isElite) ? 7.5f * hs : 10.0f * hs;
            Ring(nx, trackY, r, 2.0f * hs, kRing, 0.95f, keyline);
            Arc(nx, trackY, r, 2.2f * hs, -0.5f * kPi, 2.0f * kPi * frac, arcCol, 1.0f);
            if (gm.waveTransitionTimer > 0.0f) {
                const float t = 1.0f - gm.waveTransitionTimer / 2.0f;
                const float pulse = std::clamp(t / 0.6f, 0.0f, 1.0f);
                Ring(nx, trackY, r + 18.0f * hs * pulse, 1.4f * hs, kPast,
                     0.9f * (1.0f - pulse), 0.0f);
            }
        } else if (isFinal) {
            DiamondOutline(nx, trackY, 9.0f * hs, 1.7f * hs, kFinal, 1.0f, keyline);
            DiamondOutline(nx, trackY, 4.5f * hs, 1.1f * hs, kFinal, 0.9f, 0.0f);
        } else if (isElite) {
            DiamondOutline(nx, trackY, 7.0f * hs, 1.7f * hs,
                           w < wave ? kPast : kElite, w < wave ? 0.85f : 1.0f, keyline);
        } else if (w < wave) {
            Dot(nx, trackY, 3.0f * hs, kPast, 0.95f, keyline);
        } else {
            Ring(nx, trackY, 4.0f * hs, 1.3f * hs, kNext, 0.85f, keyline);
        }
    }
    BatchFlush();

    // ── next elite tag ──
    const float tagY = trackY + 30.0f * hs;
    const float tagSc = UiTextScale(g_TextS, UiTextLevel::Description, hs * 0.66f);
    int eliteWave = -1;
    ScheduledEliteInfo elite;
    for (int w = std::max(wave, ELITE_WAVE_INTERVAL); w <= finalWave; ++w) {
        const ScheduledEliteInfo info = GetScheduledElite(w, difficulty);
        if (!info.scheduled || !IsEliteMob(info.kind)) continue;
        if (w == wave && info.spawned && !info.alive) continue;   // already defeated
        eliteWave = w;
        elite = info;
        break;
    }
    if (eliteWave > 0) {
        static const wchar_t* kEliteLabel[3] = { L"정예", L"ELITE", L"エリート" };
        static const wchar_t* kActive[3] = { L"출현", L"ACTIVE", L"出現中" };
        static const wchar_t* kUntil[3] = { L"등장까지", L"IN", L"出現まで" };
        const int codexId = (int)CodexMobIdForKind(elite.kind);
        const wchar_t* name = EliteName(codexId);
        wchar_t tag[96];
        if (eliteWave > wave) {
            swprintf_s(tag, L"%02d · %ls %ls", eliteWave, kEliteLabel[li], name);
        } else if (elite.alive) {
            swprintf_s(tag, L"%ls %ls · %ls", kEliteLabel[li], name, kActive[li]);
        } else {
            const int s = std::max(0, (int)std::ceil(ELITE_SPAWN_WAVE_SECONDS - waveSeconds));
            swprintf_s(tag, L"%ls %ls · %ls %d:%02d", kEliteLabel[li], name, kUntil[li], s / 60, s % 60);
        }
        const float nodeX = x0 + gap * (float)(eliteWave - 1);
        const float iconR = 9.0f * hs;
        const float textW = g_TextS.Width(tag, tagSc);
        const float tagW = iconR * 2.0f + 8.0f * hs + textW;
        const float iconX = std::min(nodeX, x0 + trackW + 40.0f * hs - tagW + iconR);
        const float nodeBottom = trackY + (eliteWave == wave ? 14.0f : 9.0f) * hs;
        const float elbowY = tagY - iconR - 4.0f * hs;
        BindMainShader();
        Segment(nodeX, nodeBottom, nodeX, elbowY, 1.1f * hs, kElite, 0.75f);
        if (std::fabs(iconX - nodeX) > 0.5f)
            Segment(nodeX, elbowY, iconX, elbowY, 1.1f * hs, kElite, 0.75f);
        const bool known = CodexMobNameUnlocked(codexId);
        const bool seen = CodexMobSeen(codexId);
        if (!seen) {
            DiamondOutline(iconX, tagY, iconR * 0.8f, 1.4f * hs, kElite, 0.9f, keyline);
            BatchFlush();
        } else {
            BatchFlush();
            // The Astral Log silhouette; a seen-only elite stays faint.
            const float savedAlpha = g_BatchAlpha;
            g_BatchAlpha = known ? 1.0f : 0.40f;
            float previewScale = iconR / 18.0f * 1.6f;
            // Genesis and Quasar carry wide rings around a small body.
            if (elite.kind == MobKind::GENESIS) previewScale *= 0.38f;
            else if (elite.kind == MobKind::QUASAR) previewScale *= 0.75f;
            drawCodexMobPreview(codexId, iconX, tagY, previewScale);
            BatchFlush();
            g_BatchAlpha = savedAlpha;
            BindMainShader();
        }
        const float textBase = tagY + g_TextS.BaselineOffset(tagSc) * 0.36f;
        HudText(g_TextS, tag, iconX + iconR + 8.0f * hs, textBase, tagSc, kElite, 1.0f);
    }

    // ── this wave's debuff, kept until the wave ends ──
    if (wave >= 2 && gm.lastWaveDebuff >= 0 && gm.lastWaveDebuff < AUG_TOTAL) {
        static const wchar_t* kDebuff[3] = { L"디버프", L"DEBUFF", L"デバフ" };
        wchar_t line[160];
        swprintf_s(line, L"%ls · %ls", kDebuff[li], ALL_AUGS[gm.lastWaveDebuff].locName[li]);
        const float debuffY = tagY + (eliteWave > 0 ? 26.0f : 0.0f) * hs;
        const float lineSc = UiTextScale(g_TextS, UiTextLevel::Description, hs * 0.62f);
        const float textW = g_TextS.Width(line, lineSc);
        const float dash = 10.0f * hs;
        const float left = cx - (dash + 8.0f * hs + textW) * 0.5f;
        // Freshly applied debuffs flash for the old toast's four seconds.
        const float fresh = std::clamp(gm.waveDebuffToast / 4.0f, 0.0f, 1.0f);
        const float alpha = 0.82f + 0.18f * fresh;
        BindMainShader();
        Segment(left, debuffY, left + dash, debuffY, 2.0f * hs + keyline, {0, 0, 0}, kKeyA);
        Segment(left, debuffY, left + dash, debuffY, 2.0f * hs, kDanger, alpha);
        BatchFlush();
        HudText(g_TextS, line, left + dash + 8.0f * hs,
                 debuffY + g_TextS.BaselineOffset(lineSc) * 0.36f, lineSc, kDanger, alpha);
    }
}
