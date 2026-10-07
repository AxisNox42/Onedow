#include "Scenes.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameManager.h"
#include "GameContext.h"
#include "SceneSkills.h"
#include "SceneUI.h"
#include "SceneInternal.h"
#include "UiLayout.h"
#include "WindowChrome.h"
#include "Settings.h"
#include "Translations.h"
#include "TutorialText.h"
#include "Meta.h"
#include "Achievements.h"
#include "Codex.h"
#include "Augment.h"
#include "PlayerStats.h"
#include "Weapons.h"
#include "ExpSystem.h"
#include "SaveSystem.h"
#include "DrawPrim.h"
#include "../../Render/UiColors.h"
#include "../../Render/PlanetShader.h"
#include "../../Render/NebulaGlowShader.h"
#include "MainShader.h"
#include "../../Render/BlurShader.h"
#include "../../System/Audio.h"
#include "TextRenderer.h"
#include "IconSystem.h"
#include "../System/SystemInfo.h"
#include "Camera.h"
#include "EntityDraw.h"
#include "Monster.h"
#include "AugmentSlots.h"
#include "../../Render/Juice.h"
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <functional>

#include "SceneInternal.h"

struct TarotCardPose {
    float cx = 0.0f;
    float cy = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float angle = 0.0f;
};

static float TarotClamp01(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

static float TarotEaseOut(float t) {
    const float inv = 1.0f - TarotClamp01(t);
    return 1.0f - inv * inv * inv;
}

static void TarotSegment(float x1, float y1, float x2, float y2, float width,
                         float r, float g, float b, float a) {
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) return;
    const float nx = -dy / len * width * 0.5f;
    const float ny =  dx / len * width * 0.5f;
    BatchTri(x1 + nx, y1 + ny, x1 - nx, y1 - ny,
             x2 - nx, y2 - ny, r, g, b, a);
    BatchTri(x1 + nx, y1 + ny, x2 - nx, y2 - ny,
             x2 + nx, y2 + ny, r, g, b, a);
}
static bool TarotHit(const TarotCardPose& p, double mx, double my, float pad) {
    const float dx = (float)mx - p.cx;
    const float dy = (float)my - p.cy;
    const float cs = cosf(p.angle);
    const float sn = sinf(p.angle);
    const float lx = dx * cs + dy * sn;
    const float ly = -dx * sn + dy * cs;
    return fabsf(lx) <= p.w * 0.5f + pad &&
           fabsf(ly) <= p.h * 0.5f + pad;
}
static void DrawAugmentOrbitRing(float cx, float cy, float rx, float ry,
                                 float phase, float r, float g, float b,
                                 float alpha, float uiS,
                                 float lineScale = 1.0f,
                                 float glowScale = 0.0f) {
    if (alpha <= 0.001f || rx <= 2.0f || ry <= 2.0f) return;
    constexpr int kSegments = 48;
    float prevX = cx + cosf(phase) * rx;
    float prevY = cy + sinf(phase) * ry;
    for (int i = 1; i <= kSegments; ++i) {
        const float a = phase + 2.0f * (float)M_PI *
                        (float)i / (float)kSegments;
            const float x = cx + cosf(a) * rx;
            const float y = cy + sinf(a) * ry;
            if ((i % 8) != 0) {
                if (glowScale > 0.001f) {
                    DrawVisibleConstellLine(
                        prevX, prevY, x, y,
                        2.40f * uiS * glowScale,
                        r, g, b, alpha * 0.16f);
                }
                DrawVisibleConstellLine(prevX, prevY, x, y,
                                        0.72f * uiS * lineScale,
                                        r, g, b, alpha);
        }
        prevX = x;
        prevY = y;
    }
    for (int i = 0; i < 4; ++i) {
        const float a = phase + (float)i * (float)M_PI * 0.5f;
        DrawVisibleConstellNode(cx + cosf(a) * rx,
                                cy + sinf(a) * ry,
                                2.8f * uiS * lineScale, r, g, b,
                                alpha * 0.76f, false, true);
    }
}

static void DrawAugmentShieldOrbit(float cx, float cy, float radius,
                                   float phase, float satellitePhase,
                                   float r, float g, float b,
                                   float alpha, float pulse, float uiS) {
    if (alpha <= 0.001f || radius <= 2.0f) return;

    constexpr int kSegments = 64;
    for (int ring = 0; ring < 2; ++ring) {
        const float rx = radius * (ring == 0 ? 1.58f : 1.26f);
        const float ry = radius * (ring == 0 ? 0.84f : 0.64f);
        const float ringPhase = phase + (ring == 0 ? 0.0f : 0.24f);
        const int gapEvery = ring == 0 ? 9 : 11;
        const float thickness = (0.82f + pulse * 0.66f) * uiS;
        const float ringAlpha = alpha * (ring == 0 ? 0.82f : 0.58f);

        float prevX = cx + cosf(ringPhase) * rx;
        float prevY = cy + sinf(ringPhase) * ry;
        for (int i = 1; i <= kSegments; ++i) {
            const float a = ringPhase + 2.0f * (float)M_PI *
                            (float)i / (float)kSegments;
            const float x = cx + cosf(a) * rx;
            const float y = cy + sinf(a) * ry;
            if ((i % gapEvery) != 0) {
                DrawVisibleConstellLine(prevX, prevY, x, y,
                                        thickness, r, g, b, ringAlpha);
            }
            prevX = x;
            prevY = y;
        }
    }

    const float satelliteRx = radius * 1.58f;
    const float satelliteRy = radius * 0.84f;
    const float satelliteX = cx + cosf(satellitePhase) * satelliteRx;
    const float satelliteY = cy + sinf(satellitePhase) * satelliteRy;
    const float satelliteSize = (3.8f + pulse * 2.2f) * uiS;
    DrawConstellationDisc(satelliteX, satelliteY,
                          satelliteSize * (3.0f + pulse * 1.6f),
                          r, g, b, alpha * (0.14f + pulse * 0.16f));
    DrawVisibleConstellNode(satelliteX, satelliteY, satelliteSize,
                            r, g, b, alpha * (0.72f + pulse * 0.24f),
                            true, true);
}
static void DrawTarotEmblem(const AugDef& def, float cx, float cy, float size,
                            float r, float g, float b, float a, float now) {
    const AugListGroup group = AugListGroupOf(def);
    const float pulse = 1.0f + sinf(now * 2.7f) * 0.035f;
    const float outer = size * 0.50f * pulse;
    const float inner = size * 0.34f;
    drawCircle(cx, cy, outer, r * 0.18f, g * 0.18f, b * 0.18f, a * 0.56f);
    TarotSegment(cx - outer, cy, cx + outer, cy, 1.0f, r, g, b, a * 0.34f);
    TarotSegment(cx, cy - outer, cx, cy + outer, 1.0f, r, g, b, a * 0.34f);

    if (def.rarity == AugRarity::COMBO) {
        drawDiamond(cx - size * 0.20f, cy, size * 0.38f, r, g, b, a * 0.82f);
        drawDiamond(cx + size * 0.20f, cy, size * 0.38f, r, g, b, a * 0.82f);
        TarotSegment(cx - size * 0.10f, cy, cx + size * 0.10f, cy,
                     2.5f, r, g, b, a);
    } else if (def.rarity == AugRarity::DEBUFF) {
        const float q = inner * 0.92f;
        BatchTri(cx, cy + q, cx - q, cy - q * 0.72f,
                 cx + q, cy - q * 0.72f, r, g, b, a * 0.78f);
        drawDiamond(cx, cy - size * 0.04f, size * 0.28f,
                    1.0f, 0.28f, 0.28f, a);
    } else {
        const int points = (group == AugListGroup::SKILL ||
                            group == AugListGroup::COMBO) ? 6 : 4;
        for (int i = 0; i < points; ++i) {
            const float ang = -M_PI * 0.5f + (float)i * 2.0f * M_PI / points;
            const float x = cx + cosf(ang) * inner;
            const float y = cy + sinf(ang) * inner;
            TarotSegment(cx, cy, x, y, 2.0f, r, g, b, a * 0.72f);
            drawDiamond(x, y, size * 0.14f, r, g, b, a * 0.90f);
        }
        if (group == AugListGroup::ORBIT || group == AugListGroup::MYTHIC)
            drawCircle(cx, cy, inner * 0.52f, r, g, b, a * 0.60f);
        drawDiamond(cx, cy, size * 0.30f, r, g, b, a);
    }
    BatchFlush();
}

static void DrawTarotCardSurface(const TarotCardPose& p, float r, float g,
                                 float b, float alpha, float hover,
                                 float flash, bool selected, float now) {
    // Historical augment cards were square to the screen: a dark body, a
    // rarity-colored title bar, full neon border, and a small constellation
    // corner frame. Keep the pose parameter so the animation code can still
    // own movement/scale, but render the card in this deliberately flat style.
    const float left = p.cx - p.w * 0.5f;
    const float top = p.cy - p.h * 0.5f;
    const float topBar = std::max(38.0f, p.w * 0.16f);
    const float inner = std::max(4.0f, p.w * 0.018f);
    const float shimmer = 0.15f + 0.10f * (sinf(now * 3.2f) * 0.5f + 0.5f);
    const float glow = 0.78f + hover * 0.26f + flash * 0.25f;

    BindMainShader();
    drawRect(left + 8.0f, top + 10.0f, p.w, p.h,
             0.0f, 0.0f, 0.02f, alpha * 0.44f);
    drawRect(left, top, p.w, p.h,
             0.018f, 0.024f, 0.052f, alpha * 0.98f);
    drawRect(left + inner, top + inner, p.w - inner * 2.0f,
             p.h - inner * 2.0f,
             0.035f + r * 0.06f, 0.045f + g * 0.04f,
             0.082f + b * 0.04f, alpha * 0.92f);
    drawRect(left, top, p.w, topBar,
             r * 0.38f, g * 0.38f, b * 0.38f,
             alpha * (0.46f + hover * 0.16f + flash * 0.12f));

    const float borderR = std::min(1.0f, r * (1.4f + shimmer) + flash * 0.55f);
    const float borderG = std::min(1.0f, g * (1.4f + shimmer) + flash * 0.55f);
    const float borderB = std::min(1.0f, b * (1.4f + shimmer) + flash * 0.55f);
    const float borderA = alpha * glow;
    drawRect(left, top, p.w, 1.8f, borderR, borderG, borderB, borderA);
    drawRect(left, top + p.h - 1.8f, p.w, 1.8f,
             borderR, borderG, borderB, borderA);
    drawRect(left, top, 1.8f, p.h, borderR, borderG, borderB, borderA);
    drawRect(left + p.w - 1.8f, top, 1.8f, p.h,
             borderR, borderG, borderB, borderA);
    drawConstellFrame(left + inner, top + inner, p.w - inner * 2.0f,
                      p.h - inner * 2.0f, r, g, b,
                      alpha * (0.26f + hover * 0.24f),
                      14.0f, 4.0f, alpha * 0.12f);

    // The old card reserves the upper half for the icon. This matches the
    // icon slot used by the content pass below, so the divider never cuts
    // through the glyph at smaller window scales.
    const float dividerY = top + topBar + p.w * 0.54f;
    drawRect(left + p.w * 0.10f, dividerY,
             p.w * 0.80f, 1.5f, r, g, b,
             alpha * (0.26f + hover * 0.18f));
    drawRect(left + p.w * 0.10f, top + p.h - p.w * 0.24f,
             p.w * 0.80f, 1.0f, r, g, b, alpha * 0.18f);

    drawDiamond(left + 10.0f, top + 10.0f, 6.0f + hover * 2.0f,
                borderR, borderG, borderB, alpha * 0.88f);
    drawDiamond(left + p.w - 10.0f, top + 10.0f,
                6.0f + hover * 2.0f, borderR, borderG, borderB,
                alpha * 0.88f);
    drawDiamond(left + 10.0f, top + p.h - 10.0f, 4.5f,
                r, g, b, alpha * 0.52f);
    drawDiamond(left + p.w - 10.0f, top + p.h - 10.0f, 4.5f,
                r, g, b, alpha * 0.52f);

    if (selected) {
        drawRect(left - 5.0f, top - 5.0f, p.w + 10.0f, 2.6f,
                 1.0f, 1.0f, 1.0f, alpha * 0.78f);
        drawRect(left - 5.0f, top + p.h + 2.4f, p.w + 10.0f, 2.6f,
                 1.0f, 1.0f, 1.0f, alpha * 0.78f);
        drawRect(left - 5.0f, top - 5.0f, 2.6f, p.h + 10.0f,
                 1.0f, 1.0f, 1.0f, alpha * 0.78f);
        drawRect(left + p.w + 2.4f, top - 5.0f, 2.6f, p.h + 10.0f,
                 1.0f, 1.0f, 1.0f, alpha * 0.78f);
    }
    BatchFlush();
}

std::vector<std::wstring> TarotWrap(const wchar_t* src, float scale,
                                            float maxWidth) {
    std::vector<std::wstring> lines;
    const wchar_t* text = src ? src : L"";
    auto wrapSegment = [&](std::wstring segment) {
        while (!segment.empty() && segment.front() == L' ') segment.erase(segment.begin());
        std::wstring line;
        size_t start = 0;
        while (start < segment.size()) {
            const size_t end = segment.find(L' ', start);
            const std::wstring word = segment.substr(start,
                end == std::wstring::npos ? std::wstring::npos : end - start);
            if (word.empty()) {
                if (end == std::wstring::npos) break;
                start = end + 1;
                continue;
            }
            const std::wstring candidate = line.empty() ? word : line + L" " + word;
            if (g_TextS.Width(word.c_str(), scale) > maxWidth) {
                if (!line.empty()) {
                    lines.push_back(line);
                    line.clear();
                }
                std::wstring part;
                for (wchar_t ch : word) {
                    const std::wstring next = part + ch;
                    if (!part.empty() &&
                        g_TextS.Width(next.c_str(), scale) > maxWidth) {
                        lines.push_back(part);
                        part.clear();
                    }
                    part += ch;
                }
                line = std::move(part);
            } else if (!line.empty() &&
                       g_TextS.Width(candidate.c_str(), scale) > maxWidth) {
                lines.push_back(line);
                line = word;
            } else {
                line = candidate;
            }
            if (end == std::wstring::npos) break;
            start = end + 1;
        }
        if (!line.empty()) lines.push_back(line);
    };
    std::wstring segment;
    for (const wchar_t* p = text; *p; ++p) {
        const bool spacedSlash = *p == L'/' && p > text && p[1] != L'\0'
                              && p[-1] == L' ' && p[1] == L' ';
        if (*p == L'\xB7' || spacedSlash) {
            wrapSegment(std::move(segment));
            segment.clear();
            continue;
        }
        segment.push_back(*p);
    }
    wrapSegment(std::move(segment));
    return lines;
}

static void DrawTarotDetailPanel(const AugDef& def, float x, float y, float w,
                                 float h, float alpha) {
    float r, g, b;
    GetRarityColor(def.rarity, r, g, b);
    BindMainShader();
    drawRect(x + 7.0f, y + 8.0f, w, h, 0.0f, 0.0f, 0.02f, alpha * 0.36f);
    drawRect(x, y, w, h, 0.018f, 0.024f, 0.052f, alpha * 0.94f);
    drawRect(x, y, w, 4.0f, r, g, b, alpha * 0.95f);
    drawConstellFrame(x, y, w, h, r, g, b, alpha * 0.52f,
                      16.0f, 5.0f, alpha * 0.20f);
    BatchFlush();

    g_TextS.Draw(GetAugBadge(def), x + 18.0f, y + 16.0f,
                 UiTextScale(g_TextS, UiTextLevel::Supporting, 1.0f),
                 std::min(1.0f, r * 1.35f + 0.20f),
                 std::min(1.0f, g * 1.35f + 0.20f),
                 std::min(1.0f, b * 1.35f + 0.20f), alpha * 0.95f);
    const int language = CurLangIdx();
    const wchar_t* name = def.locName[language];
    const wchar_t* nameVariants[] = {
        def.locName[0], def.locName[1], def.locName[2]
    };
    float nameFactor = 0.90f;
    float nameScale = UiTextScale(g_TextL, UiTextLevel::Title, nameFactor);
    while (nameFactor > 0.54f &&
           MaxLocalizedTextWidth(g_TextL, nameVariants, 3, nameScale) > w - 36.0f) {
        nameFactor -= 0.04f;
        nameScale = UiTextScale(g_TextL, UiTextLevel::Title, nameFactor);
    }
    const float nameY = y + 45.0f;
    const float nameH = g_TextL.Height(name, nameScale);
    g_TextL.Draw(name, x + 18.0f, nameY, nameScale,
                 1.0f, 1.0f, 1.0f, alpha * 0.98f);
    BatchFlush();

    const wchar_t* stat = AugStat(def);
    const float statScale = UiTextScale(g_TextS, UiTextLevel::Description, 1.0f);
    const float statY = nameY + nameH + 8.0f;
    g_TextS.Draw(stat, x + 18.0f, statY, statScale,
                 0.72f, 1.0f, 0.82f, alpha * 0.92f);
    BatchFlush();

    const float descScale = UiTextScale(g_TextS, UiTextLevel::Description, 1.0f);
    const float maxWidth = w - 36.0f;
    const std::vector<std::wstring> lines = TarotWrap(AugDesc(def), descScale, maxWidth);
    float dy = statY + g_TextS.Height(stat, statScale) + 10.0f;
    for (const std::wstring& line : lines) {
        if (dy > y + h - 22.0f) break;
        g_TextS.Draw(line.c_str(), x + 18.0f, dy, descScale,
                     0.80f, 0.88f, 0.98f, alpha * 0.84f);
        dy += g_TextS.Height(line.c_str(), descScale) + 3.0f;
    }
    BatchFlush();
}

enum class AugPreviewMetric {
    ATTACK,
    FIRE_RATE,
    BULLET_SPEED,
    MOVE_SPEED,
    MAX_HP,
    VISION,
    COUNT
};

enum class AugVisualFamily {
    STRIKE,
    WARD,
    MOTION,
    ORBITAL,
    UTILITY,
    FRACTURED,
    DUAL
};

struct AugSelectionLayout {
    float ui = 1.0f;
    float textUi = 1.0f;
    float orbitCX = 0.0f;
    float orbitCY = 0.0f;
    float orbitR = 0.0f;
    float baseR = 0.0f;
    float panelX = 0.0f;
    float panelY = 0.0f;
    float panelW = 0.0f;
    float panelH = 0.0f;
};

static PlayerStats MakeAugPreviewBaseStats() {
    PlayerStats base;
    ApplyMeta(base);
    base.windowSize *= g_Scale;

    if (g_CurrentWeapon >= 0 &&
        g_CurrentWeapon < (int)StartWeapon::_COUNT) {
        ApplyWeapon(base, (StartWeapon)g_CurrentWeapon);
    }
    return base;
}

static PlayerStats MakeAugPreviewTrialStats() {
    PlayerStats trial = MakeAugPreviewBaseStats();
    const float trialHp = TrialPlayerMaxHpMult();
    if (trialHp < 0.999f)
        trial.maxHP *= trialHp;
    return trial;
}

static PlayerStats MakeAugPreviewOwnedStats() {
    PlayerStats owned = MakeAugPreviewTrialStats();
    for (int idx : g_OwnedAugs) {
        if (idx < 0 || idx >= AUG_TOTAL) continue;
        owned.Apply(ALL_AUGS[idx].type);
        if (ALL_AUGS[idx].rarity == AugRarity::COMMON)
            owned.ApplyCommonMultBoost();
    }
    return owned;
}

static PlayerStats MakeAugPreviewCandidateStats(const PlayerStats& owned,
                                                const AugDef& candidate) {
    PlayerStats preview = owned;
    preview.Apply(candidate.type);
    if (candidate.rarity == AugRarity::COMMON)
        preview.ApplyCommonMultBoost();
    return preview;
}

static float AugPreviewMetricValue(const PlayerStats& stats,
                                   AugPreviewMetric metric) {
    switch (metric) {
    case AugPreviewMetric::ATTACK:
        return stats.GetBaseDamage() * stats.damageMultiplier;
    case AugPreviewMetric::FIRE_RATE:
        return stats.fireInterval > 0.0001f
            ? 1.0f / stats.fireInterval : 0.0f;
    case AugPreviewMetric::BULLET_SPEED:
        return stats.bulletSpeed;
    case AugPreviewMetric::MOVE_SPEED:
        return stats.moveSpeedMult * 100.0f;
    case AugPreviewMetric::MAX_HP:
        return stats.maxHP;
    case AugPreviewMetric::VISION:
        return stats.windowSize;
    default:
        return 0.0f;
    }
}

struct TarotBurstParticle {
    float x = 0.0f, y = 0.0f;
    float vx = 0.0f, vy = 0.0f;
    float life = 0.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f;
};

struct AugShapeVertex {
    float angleDeg;
    float radius;
};

struct AugShapeEdge {
    int a;
    int b;
};

struct AugImpactRow {
    AugPreviewMetric metric = AugPreviewMetric::ATTACK;
    float before = 0.0f;
    float after = 0.0f;
};

static float AugSelectionClamp(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

static float AugSelectionEase(float v) {
    const float t = AugSelectionClamp(v);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

static float AugSelectionSmooth(float speed, float delta) {
    return 1.0f - expf(-std::max(0.0f, speed) *
                       std::max(0.0f, delta));
}

static AugSelectionLayout MakeAugSelectionLayout(float sw, float sh) {
    AugSelectionLayout layout;
    layout.ui = std::max(0.72f, std::min(1.15f,
        std::min(sw / 1920.0f, sh / 1080.0f)));
    layout.textUi = std::max(0.86f, layout.ui);
    const float aspect = sh > 1.0f ? sw / sh : 1.777f;
    layout.orbitCX = sw * (aspect < 1.50f ? 0.34f : 0.36f);
    layout.orbitCY = sh * 0.50f;
    layout.panelX = sw * (aspect < 1.50f ? 0.66f : 0.69f);
    layout.panelY = sh * 0.19f;
    layout.panelW = std::max(230.0f * layout.ui,
                             sw - layout.panelX - 42.0f * layout.ui);
    layout.panelH = sh * 0.67f;
    // Keep the selector's large circular structure dominant while leaving a
    // deliberate breathing gap before the detail panel.
    layout.orbitR = std::min(sw * 0.21f, sh * 0.32f);
    layout.orbitR = std::min(layout.orbitR,
        std::max(140.0f * layout.ui,
                 layout.panelX - layout.orbitCX - 132.0f * layout.ui));
    const float baseR = std::max(42.0f * layout.ui,
        std::min(64.0f * layout.ui, std::min(sw, sh) * 0.060f));
    layout.baseR = baseR * 1.25f;
    return layout;
}

static bool AugIsWardVisual(AugType type) {
    switch (type) {
    case AugType::REGEN_UP:
    case AugType::LIFESTEAL:
    case AugType::VAMPIRE:
    case AugType::MK2:
    case AugType::HP_UP:
    case AugType::FIREWALL:
    case AugType::REGEN_2:
    case AugType::CB_BASTION:
    case AugType::CB_LIFEBUOY:
        return true;
    default:
        return false;
    }
}

static bool AugIsMotionVisual(AugType type) {
    switch (type) {
    case AugType::MOVE_UP:
    case AugType::VISION_UP:
    case AugType::LIGHT_AMMO:
    case AugType::LIGHT_STEP:
    case AugType::MINIATURIZE:
        return true;
    default:
        return false;
    }
}

static bool AugIsStrikeVisual(AugType type) {
    switch (type) {
    case AugType::DMG_UP:
    case AugType::RATE_UP:
    case AugType::SPD_UP:
    case AugType::CRIT:
    case AugType::OVERDRIVE:
    case AugType::CORE_OVERLOAD:
    case AugType::PIERCE:
    case AugType::PIERCE_2:
    case AugType::TWIN:
    case AugType::TWIN_2:
    case AugType::CHAIN:
    case AugType::CHAIN_2:
    case AugType::DEATH_BLAST:
    case AugType::DEATH_BLAST_2:
    case AugType::POWER_SURGE:
        return true;
    default:
        return false;
    }
}

static AugVisualFamily AugVisualFamilyOf(const AugDef& def) {
    if (def.rarity == AugRarity::DEBUFF) return AugVisualFamily::FRACTURED;
    if (def.rarity == AugRarity::COMBO) return AugVisualFamily::DUAL;
    const AugListGroup group = AugListGroupOf(def);
    if (group == AugListGroup::ORBIT) return AugVisualFamily::ORBITAL;
    if (group == AugListGroup::WEAPON) return AugVisualFamily::STRIKE;
    if (group == AugListGroup::SKILL || group == AugListGroup::SPECIAL)
        return AugVisualFamily::UTILITY;
    if (AugIsWardVisual(def.type)) return AugVisualFamily::WARD;
    if (AugIsMotionVisual(def.type)) return AugVisualFamily::MOTION;
    if (AugIsStrikeVisual(def.type)) return AugVisualFamily::STRIKE;
    return AugVisualFamily::UTILITY;
}

static const wchar_t* AugVisualFamilyLabel(AugVisualFamily family) {
    switch (family) {
    case AugVisualFamily::STRIKE:    return L"STRIKE";
    case AugVisualFamily::WARD:      return L"WARD";
    case AugVisualFamily::MOTION:    return L"MOTION";
    case AugVisualFamily::ORBITAL:   return L"ORBITAL";
    case AugVisualFamily::UTILITY:   return L"UTILITY";
    case AugVisualFamily::FRACTURED: return L"FRACTURED";
    case AugVisualFamily::DUAL:      return L"DUAL";
    default:                         return L"UNKNOWN";
    }
}

static void DrawAugSelectionConstellation(const AugDef& def,
                                          AugVisualFamily family,
                                          float cx, float cy, float radius,
                                          float lineR, float lineG, float lineB,
                                          float accentR, float accentG,
                                          float accentB, float alpha,
                                          float reveal, float rotation,
                                          float ui, bool focused,
                                          float now) {
    static const AugShapeVertex strikeV[] = {
        {-90,1.00f},{-54,0.42f},{-18,0.96f},{18,0.40f},{54,1.00f},
        {90,0.42f},{126,0.94f},{162,0.40f},{198,0.98f},{234,0.42f}
    };
    static const AugShapeEdge strikeE[] = {
        {0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,8},{8,9},{9,0},
        {1,5},{5,9},{9,3},{3,7},{7,1}
    };
    static const AugShapeVertex wardV[] = {
        {-90,1.00f},{-30,1.00f},{30,1.00f},{90,1.00f},{150,1.00f},{210,1.00f},
        {-60,0.50f},{60,0.50f},{180,0.50f},{0,0.00f}
    };
    static const AugShapeEdge wardE[] = {
        {0,1},{1,2},{2,3},{3,4},{4,5},{5,0},{6,7},{7,8},{8,6},
        {0,8},{1,6},{2,6},{3,7},{4,7},{5,8},{6,9},{7,9},{8,9}
    };
    static const AugShapeVertex motionV[] = {
        {-72,0.96f},{-24,1.00f},{25,0.82f},{-38,0.55f},{18,0.48f},
        {72,0.62f},{126,0.78f},{176,0.58f},{220,0.88f}
    };
    static const AugShapeEdge motionE[] = {
        {0,1},{1,2},{2,4},{4,3},{3,0},{0,4},{1,3},{4,5},{5,6},{6,7},{7,8}
    };
    static const AugShapeVertex orbitalV[] = {
        {0,0.00f},{-90,1.00f},{-45,0.88f},{0,1.00f},{45,0.88f},
        {90,1.00f},{135,0.88f},{180,1.00f},{225,0.88f}
    };
    static const AugShapeEdge orbitalE[] = {
        {1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,8},{8,1},
        {0,1},{0,3},{0,5},{0,7}
    };
    static const AugShapeVertex utilityV[] = {
        {-90,0.94f},{-45,0.58f},{0,0.84f},{45,0.54f},{90,0.92f},
        {150,0.62f},{205,0.98f},{232,0.52f},{268,0.82f}
    };
    static const AugShapeEdge utilityE[] = {
        {0,1},{1,2},{2,3},{3,4},{1,5},{5,6},{3,7},{7,8},{5,7},{2,7}
    };
    static const AugShapeVertex fracturedV[] = {
        {-92,1.00f},{-48,0.55f},{-8,0.92f},{38,0.48f},{78,1.00f},
        {126,0.56f},{168,0.90f},{210,0.44f},{252,0.86f}
    };
    static const AugShapeEdge fracturedE[] = {
        {0,1},{1,2},{3,4},{4,5},{5,6},{7,8},{8,0},{1,5},{2,7},{4,7}
    };

    if (family == AugVisualFamily::DUAL) {
        DrawAugSelectionConstellation(def, AugVisualFamily::WARD,
            cx - radius * 0.28f, cy, radius * 0.70f,
            lineR, lineG, lineB, accentR, accentG, accentB,
            alpha, reveal, rotation - 0.18f, ui, focused, now);
        DrawAugSelectionConstellation(def, AugVisualFamily::STRIKE,
            cx + radius * 0.28f, cy, radius * 0.70f,
            lineR, lineG, lineB, accentR, accentG, accentB,
            alpha, reveal, rotation + 0.18f, ui, focused, now);
        DrawVisibleConstellLine(cx - radius * 0.24f, cy,
                                cx + radius * 0.24f, cy,
                                (focused ? 2.4f : 1.7f) * ui,
                                accentR, accentG, accentB,
                                alpha * 0.62f * reveal);
        BatchFlush();
        return;
    }

    const AugShapeVertex* vertices = utilityV;
    const AugShapeEdge* edges = utilityE;
    int vertexCount = (int)(sizeof(utilityV) / sizeof(utilityV[0]));
    int edgeCount = (int)(sizeof(utilityE) / sizeof(utilityE[0]));
    switch (family) {
    case AugVisualFamily::STRIKE:
        vertices = strikeV; edges = strikeE;
        vertexCount = (int)(sizeof(strikeV) / sizeof(strikeV[0]));
        edgeCount = (int)(sizeof(strikeE) / sizeof(strikeE[0]));
        break;
    case AugVisualFamily::WARD:
        vertices = wardV; edges = wardE;
        vertexCount = (int)(sizeof(wardV) / sizeof(wardV[0]));
        edgeCount = (int)(sizeof(wardE) / sizeof(wardE[0]));
        break;
    case AugVisualFamily::MOTION:
        vertices = motionV; edges = motionE;
        vertexCount = (int)(sizeof(motionV) / sizeof(motionV[0]));
        edgeCount = (int)(sizeof(motionE) / sizeof(motionE[0]));
        break;
    case AugVisualFamily::ORBITAL:
        vertices = orbitalV; edges = orbitalE;
        vertexCount = (int)(sizeof(orbitalV) / sizeof(orbitalV[0]));
        edgeCount = (int)(sizeof(orbitalE) / sizeof(orbitalE[0]));
        break;
    case AugVisualFamily::FRACTURED:
        vertices = fracturedV; edges = fracturedE;
        vertexCount = (int)(sizeof(fracturedV) / sizeof(fracturedV[0]));
        edgeCount = (int)(sizeof(fracturedE) / sizeof(fracturedE[0]));
        break;
    default:
        break;
    }

    DrawConstellationDisc(cx, cy, radius * 1.48f,
                          0.0f, 0.0f, 0.008f, alpha * 0.38f);
    DrawNebulaGlow(cx, cy, radius * (focused ? 2.08f : 1.58f),
                   lineR, lineG, lineB,
                   alpha * (focused ? 0.54f : 0.22f), now,
                   def.rarity == AugRarity::SPECIAL);

    float px[16] = {};
    float py[16] = {};
    const unsigned seed = ((unsigned)((int)def.type + 1) * 2654435761u);
    const float baseJitter = (((seed >> 4) & 255u) / 255.0f - 0.5f) * 0.12f;
    for (int i = 0; i < vertexCount && i < 16; ++i) {
        const unsigned bits = (seed >> ((i * 3) & 15)) ^ (seed * (unsigned)(i + 3));
        const float angleJitter = (((bits >> 5) & 15u) / 15.0f - 0.5f) * 0.10f;
        const float radialJitter = 0.94f + ((bits >> 9) & 15u) / 15.0f * 0.12f;
        const float angle = vertices[i].angleDeg * (float)M_PI / 180.0f +
                            rotation + baseJitter + angleJitter;
        const float vr = radius * vertices[i].radius * radialJitter;
        px[i] = cx + cosf(angle) * vr;
        py[i] = cy + sinf(angle) * vr;
    }

    const float edgeProgress = AugSelectionClamp(reveal) * (float)edgeCount;
    for (int e = 0; e < edgeCount; ++e) {
        const float local = AugSelectionClamp(edgeProgress - (float)e);
        if (local <= 0.001f) continue;
        const int a = edges[e].a;
        const int b = edges[e].b;
        const float ex = px[a] + (px[b] - px[a]) * local;
        const float ey = py[a] + (py[b] - py[a]) * local;
        DrawVisibleConstellLine(px[a], py[a], ex, ey,
                                (focused ? 2.35f : 1.72f) * ui,
                                lineR, lineG, lineB,
                                alpha * (focused ? 0.90f : 0.68f));
    }

    const float nodeProgress = AugSelectionClamp(reveal * 1.20f) *
                               (float)vertexCount;
    for (int i = 0; i < vertexCount; ++i) {
        const float local = AugSelectionClamp(nodeProgress - (float)i);
        if (local <= 0.001f) continue;
        const bool centerNode = vertices[i].radius < 0.10f;
        const float size = (centerNode ? 5.8f : (focused ? 4.8f : 3.6f)) *
                           ui * (0.70f + local * 0.30f);
        DrawVisibleConstellNode(px[i], py[i], size,
                                centerNode ? accentR : lineR,
                                centerNode ? accentG : lineG,
                                centerNode ? accentB : lineB,
                                alpha * (0.72f + local * 0.24f),
                                focused || centerNode, true);
    }

    const float pulse = 0.5f + 0.5f * sinf(now * 3.2f + baseJitter * 8.0f);
    DrawVisibleConstellNode(cx, cy,
        (focused ? 10.5f + pulse * 2.8f : 6.5f + pulse * 1.0f) * ui,
        accentR, accentG, accentB,
        alpha * AugSelectionClamp(reveal * 1.35f), true, true);
    BatchFlush();
}

static int BuildAugImpactRows(const AugDef& def, AugImpactRow* rows,
                              int maxRows) {
    if (!rows || maxRows <= 0) return 0;
    const PlayerStats owned = MakeAugPreviewOwnedStats();
    const PlayerStats result = MakeAugPreviewCandidateStats(owned, def);
    int count = 0;
    for (int i = 0; i < (int)AugPreviewMetric::COUNT && count < maxRows; ++i) {
        const AugPreviewMetric metric = (AugPreviewMetric)i;
        const float before = AugPreviewMetricValue(owned, metric);
        const float after = AugPreviewMetricValue(result, metric);
        if (fabsf(after - before) <= 0.05f) continue;
        rows[count++] = { metric, before, after };
    }
    return count;
}

static const wchar_t* AugMetricLabel(AugPreviewMetric metric) {
    static const wchar_t* labels[3][(int)AugPreviewMetric::COUNT] = {
        { L"공격", L"연사", L"탄속", L"이동", L"최대 HP", L"시야" },
        { L"ATTACK", L"FIRE RATE", L"BULLET SPD", L"MOVE", L"MAX HP", L"VISION" },
        { L"攻撃", L"連射", L"弾速", L"移動", L"最大HP", L"視界" }
    };
    int lang = CurLangIdx();
    if (lang < 0 || lang > 2) lang = 0;
    int idx = (int)metric;
    if (idx < 0 || idx >= (int)AugPreviewMetric::COUNT) idx = 0;
    return labels[lang][idx];
}

static void FormatAugMetricValue(wchar_t* out, size_t outCount,
                                 AugPreviewMetric metric, float value,
                                 bool signedValue) {
    if (!out || outCount == 0) return;
    if (metric == AugPreviewMetric::FIRE_RATE) {
        swprintf_s(out, outCount, signedValue ? L"%+.1f/s" : L"%.1f/s", value);
    } else if (metric == AugPreviewMetric::MOVE_SPEED) {
        swprintf_s(out, outCount, signedValue ? L"%+.1f%%" : L"%.1f%%", value);
    } else {
        swprintf_s(out, outCount, signedValue ? L"%+.0f" : L"%.0f", value);
    }
}

static void Scene_AugSelectConstellationPolished(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, delta = c.delta;
    const bool isDebuff = g_GameManager.currentState == GameState::DEBUFF_SELECT;
    const int count = std::max(0, std::min(3, g_GameManager.augChoiceCount));
    const bool inExit = g_AugExitT >= 0.0f;
    const float now = (float)glfwGetTime();
    const AugSelectionLayout layout = MakeAugSelectionLayout(sw, sh);

    static int seenSerial = -1;
    static float enterT = 0.0f;
    static float orbitPhase = 0.0f;
    static float orbitSpeed = 0.0f;
    static float ringPhase = 0.0f;
    static float hoverMotionT = 0.0f;
    static float shapePhase[3] = {};
    static float shieldCycleT[3] = {};
    static float shieldPhase[3] = {};
    static float shieldT[3] = {};
    static int shieldFocus = -1;
    static float focusT[3] = {};
    static float dimT[3] = { 1.0f, 1.0f, 1.0f };
    static float flashT[3] = {};
    static float panelT = 0.0f;
    static int previousFocus = -1;
    static double previousMx = -1.0;
    static double previousMy = -1.0;
    struct Particle { float x,y,vx,vy,life,r,g,b; };
    static Particle particles[72];
    static bool particlesSpawned = false;

    if (seenSerial != g_GameManager.augmentSelectionSerial) {
        seenSerial = g_GameManager.augmentSelectionSerial;
        enterT = 0.0f;
        orbitSpeed = 0.0f;
        ringPhase = 0.0f;
        hoverMotionT = 0.0f;
        for (int i = 0; i < 3; ++i)
            shapePhase[i] = (float)i * 2.0944f;
        for (int i = 0; i < 3; ++i) {
            shieldCycleT[i] = 0.0f;
            shieldPhase[i] = 0.0f;
            shieldT[i] = 0.0f;
        }
        shieldFocus = -1;
        panelT = 0.0f;
        previousFocus = -1;
        previousMx = c.mx;
        previousMy = c.my;
        particlesSpawned = false;
        g_HoveredAug = -1;
        for (int i = 0; i < 3; ++i) {
            focusT[i] = 0.0f;
            dimT[i] = 1.0f;
            flashT[i] = 0.0f;
        }
        for (auto& p : particles) p.life = 0.0f;
    }

    if (!inExit) enterT = std::min(1.0f, enterT + delta / 1.20f);
    const float enterE = AugSelectionEase(enterT);
    const bool inputReady = enterT >= 0.66f && !inExit;
    const int focus = g_HoveredAug >= 0 && g_HoveredAug < count
        ? g_HoveredAug : -1;

    if (focus != previousFocus && focus >= 0) flashT[focus] = 1.0f;
    previousFocus = focus;
    for (int i = 0; i < 3; ++i) {
        const float focusTarget = inputReady && focus == i ? 1.0f : 0.0f;
        // Keep every candidate's own hue. Non-focused constellations only lose
        // light, so focus reads as depth instead of a palette/state change.
        const float dimTarget = focus >= 0 && focus != i ? 0.46f : 1.0f;
        focusT[i] += (focusTarget - focusT[i]) *
                     AugSelectionSmooth(8.5f, delta);
        dimT[i] += (dimTarget - dimT[i]) *
                   AugSelectionSmooth(9.0f, delta);
        flashT[i] = std::max(0.0f, flashT[i] - delta / 0.22f);
    }
    panelT += ((focus >= 0 && !inExit ? 1.0f : 0.0f) - panelT) *
              AugSelectionSmooth(10.0f, delta);

    const float decel = AugSelectionEase(
        AugSelectionClamp((enterT - 0.12f) / 0.78f));
    const float entrySpeed = 0.90f + (0.065f - 0.90f) * decel;
    // Keep one shared angular velocity for the whole orbit. Hovering ramps
    // that velocity up smoothly instead of assigning a different speed to
    // each candidate based on its position.
    const float hoverTarget = focus >= 0 ? 1.0f : 0.0f;
    hoverMotionT += (hoverTarget - hoverMotionT) *
                    AugSelectionSmooth(focus >= 0 ? 11.0f : 8.0f, delta);
    const float hoverOrbitSpeed = 0.065f +
                                  0.075f * AugSelectionEase(hoverMotionT);
    const float targetSpeed = focus >= 0 ? hoverOrbitSpeed : entrySpeed;
    orbitSpeed += (targetSpeed - orbitSpeed) *
                  AugSelectionSmooth(focus >= 0 ? 8.5f : 4.5f, delta);
    if (!inExit) {
        orbitPhase += orbitSpeed * delta;
        ringPhase += orbitSpeed * delta;
        if (ringPhase > 2.0f * (float)M_PI)
            ringPhase -= 2.0f * (float)M_PI;
    }
    for (int i = 0; i < count; ++i) {
        // The constellation's local rotation is continuous across hover
        // changes. Focus may scale/pull it during confirmation, but never
        // re-seed its angle from a different time-based multiplier.
        // Keep the constellation's own spin independent from the shared
        // orbit. The previous value was too subtle to read during play.
        const float localSpeed = 0.24f;
        if (!inExit) shapePhase[i] += localSpeed * delta;
        if (shapePhase[i] > 2.0f * (float)M_PI)
            shapePhase[i] -= 2.0f * (float)M_PI;
    }

    float orbitX[3] = {};
    float orbitY[3] = {};
    float drawX[3] = {};
    float drawY[3] = {};
    float drawR[3] = {};
    float reveal[3] = {};
    for (int i = 0; i < count; ++i) {
        float angle = -(float)M_PI * 0.50f;
        if (count > 1)
            angle += orbitPhase + (float)i * (2.0f * (float)M_PI / (float)count);
        const float radiusScale = count == 1 ? 0.72f : 1.0f;
        orbitX[i] = layout.orbitCX + cosf(angle) * layout.orbitR * radiusScale;
        orbitY[i] = layout.orbitCY + sinf(angle) * layout.orbitR * radiusScale;
        reveal[i] = AugSelectionEase(
            AugSelectionClamp((enterT - (float)i * 0.07f) / 0.62f));
        const float grow = AugSelectionEase(focusT[i]);
        // Hover magnifies the constellation exactly where it lives on the
        // orbit. Only confirmation is allowed to pull it into the centre.
        drawX[i] = layout.orbitCX + (orbitX[i] - layout.orbitCX) * reveal[i];
        drawY[i] = layout.orbitCY + (orbitY[i] - layout.orbitCY) * reveal[i];
        drawR[i] = layout.baseR * (1.0f + grow * 0.64f + flashT[i] * 0.05f);
    }

    const bool pointerMoved = previousMx < 0.0 ||
        fabs(c.mx - previousMx) > 3.0 || fabs(c.my - previousMy) > 3.0;
    if (pointerMoved) g_GameManager.augmentKeyboardFocus = false;
    previousMx = c.mx;
    previousMy = c.my;
    auto hitCandidate = [&]() -> int {
        int best = -1;
        float bestD2 = 1e30f;
        for (int i = 0; i < count; ++i) {
            const float dx = (float)c.mx - drawX[i];
            const float dy = (float)c.my - drawY[i];
            const float d2 = dx * dx + dy * dy;
            const float hitR = std::max(layout.baseR * 1.55f,
                                        drawR[i] * 1.15f);
            if (d2 <= hitR * hitR && d2 < bestD2) {
                best = i;
                bestD2 = d2;
            }
        }
        return best;
    };
    if (inputReady && pointerMoved &&
        !g_GameManager.augmentKeyboardFocus) {
        const int hovered = hitCandidate();
        if (hovered >= 0) g_HoveredAug = hovered;
    }
    const bool lmbClick = c.lmb && !g_LmbPrev;
    if (inputReady && lmbClick) {
        const int clicked = hitCandidate();
        if (clicked >= 0) {
            g_HoveredAug = clicked;
            g_GameManager.augmentKeyboardFocus = false;
            if (g_AugExitT < 0.0f) {
                g_AugExitT = 0.0f;
                g_AugExitSlot = clicked;
            }
        }
    }

    const int activeFocus = g_HoveredAug >= 0 && g_HoveredAug < count
        ? g_HoveredAug : -1;
    for (int i = 0; i < 3; ++i) {
        const float shieldTarget = !inExit && activeFocus == i ? 1.0f : 0.0f;
        shieldT[i] += (shieldTarget - shieldT[i]) *
                      AugSelectionSmooth(13.0f, delta);
    }
    const float stateR = isDebuff ? 0.96f : 0.16f;
    const float stateG = isDebuff ? 0.14f : 0.84f;
    const float stateB = isDebuff ? 0.18f : 0.98f;
    const float collapseT = inExit
        ? AugSelectionEase(AugSelectionClamp(g_AugExitT / 0.55f)) : 0.0f;
    const float novaT = inExit
        ? AugSelectionEase(AugSelectionClamp((g_AugExitT - 0.55f) / 0.40f)) : 0.0f;
    const float sceneAlpha = inExit
        ? 1.0f - AugSelectionClamp((g_AugExitT - 1.12f) / 0.50f) : 1.0f;

    if (activeFocus != shieldFocus) {
        shieldFocus = activeFocus;
        if (activeFocus >= 0) {
            shieldCycleT[activeFocus] = 0.0f;
            shieldPhase[activeFocus] = 0.0f;
        }
    }

    float activeShieldPulse = 0.0f;
    if (!inExit && activeFocus >= 0) {
        float& cycle = shieldCycleT[activeFocus];
        cycle += delta;
        constexpr float kShieldAccel = 0.40f;
        constexpr float kShieldDecel = 0.70f;
        constexpr float kShieldRest = 0.45f;
        constexpr float kShieldCycle = kShieldAccel + kShieldDecel +
                                       kShieldRest;
        while (cycle >= kShieldCycle) cycle -= kShieldCycle;

        float speedScale = 0.72f;
        if (cycle < kShieldAccel) {
            activeShieldPulse = AugSelectionEase(cycle / kShieldAccel);
            speedScale = 0.72f + activeShieldPulse * 3.10f;
        } else if (cycle < kShieldAccel + kShieldDecel) {
            const float t = (cycle - kShieldAccel) / kShieldDecel;
            const float ease = AugSelectionEase(t);
            activeShieldPulse = 1.0f - ease * 0.78f;
            speedScale = 3.82f - ease * 3.10f;
        } else {
            activeShieldPulse = 0.10f;
        }

        shieldPhase[activeFocus] += 0.92f * speedScale * delta;
        if (shieldPhase[activeFocus] > 2.0f * (float)M_PI)
            shieldPhase[activeFocus] -= 2.0f * (float)M_PI;
    }

    BindMainShader();
    // Keep the gameplay scene readable underneath the selector. The former
    // near-black plate made the constellation field and surrounding context
    // disappear on darker displays.
    drawRect(0.0f, 0.0f, sw, sh,
             isDebuff ? 0.026f : 0.008f,
             isDebuff ? 0.008f : 0.018f,
             isDebuff ? 0.014f : 0.040f,
             enterE * (isDebuff ? 0.58f : 0.52f) * sceneAlpha);
    DrawNebulaGlow(layout.orbitCX, layout.orbitCY,
                   layout.orbitR * 1.02f,
                   stateR, stateG, stateB,
                   enterE * sceneAlpha * 0.34f,
                   now, false);
    drawRect(layout.panelX - 30.0f * layout.ui,
             layout.panelY - 28.0f * layout.ui,
             layout.panelW + 18.0f * layout.ui,
             layout.panelH + 38.0f * layout.ui,
             0.0f, 0.006f, 0.016f,
             enterE * sceneAlpha * 0.20f);
    DrawRadialGradientRect(
        layout.panelX - 80.0f * layout.ui,
        layout.panelY - 20.0f * layout.ui,
        layout.panelW + 160.0f * layout.ui,
        layout.panelH + 40.0f * layout.ui,
        stateR * 0.20f, stateG * 0.20f, stateB * 0.20f,
        enterE * sceneAlpha * 0.17f);
    BatchFlush();

    const float frameA = enterE * sceneAlpha;
    drawConstellFrame(18.0f * layout.ui, 18.0f * layout.ui,
                      sw - 36.0f * layout.ui,
                      sh - 36.0f * layout.ui,
                      stateR, stateG, stateB, frameA * 0.16f,
                      24.0f * layout.ui, 7.0f * layout.ui,
                      frameA * 0.10f, enterE);
    BatchFlush();

    if (!inExit && count > 0) {
        // The candidates share one circular orbit. It replaces the old
        // candidate-to-candidate triangle; the constellation shapes remain
        // independent and unchanged.
        const float orbitScale = count == 1 ? 0.72f : 1.0f;
        const float orbitReveal = reveal[count - 1];
        DrawAugmentOrbitRing(
            layout.orbitCX, layout.orbitCY,
            layout.orbitR * orbitScale * orbitReveal,
            layout.orbitR * orbitScale * orbitReveal,
            ringPhase - (float)M_PI * 0.5f,
            stateR, stateG, stateB,
            enterE * sceneAlpha * orbitReveal * 0.44f,
            layout.ui, 1.45f, 1.0f);
        // Inner field orbits give the empty center a layered, illuminated
        // structure without changing the candidates' path.
        DrawAugmentOrbitRing(
            layout.orbitCX, layout.orbitCY,
            layout.orbitR * orbitScale * orbitReveal * 0.84f,
            layout.orbitR * orbitScale * orbitReveal * 0.84f,
            ringPhase + 0.72f,
            stateR, stateG, stateB,
            enterE * sceneAlpha * orbitReveal * 0.22f,
            layout.ui, 1.16f, 0.42f);
        DrawAugmentOrbitRing(
            layout.orbitCX, layout.orbitCY,
            layout.orbitR * orbitScale * orbitReveal * 0.66f,
            layout.orbitR * orbitScale * orbitReveal * 0.66f,
            ringPhase + 1.46f,
            stateR, stateG, stateB,
            enterE * sceneAlpha * orbitReveal * 0.14f,
            layout.ui, 1.05f, 0.30f);
        DrawAugmentOrbitRing(
            layout.orbitCX, layout.orbitCY,
            layout.orbitR * orbitScale * orbitReveal * 0.48f,
            layout.orbitR * orbitScale * orbitReveal * 0.48f,
            ringPhase + 2.10f,
            stateR, stateG, stateB,
            enterE * sceneAlpha * orbitReveal * 0.10f,
            layout.ui, 0.96f, 0.22f);
        BatchFlush();

        const wchar_t* orbitLabel = isDebuff ? L"DEBUFF SELECT"
                                             : L"AUG SELECT";
        const float orbitLabelScale = UiTextScale(g_TextS, UiTextLevel::Title,
                                                   layout.textUi);
        const float orbitLabelW = g_TextS.Width(orbitLabel, orbitLabelScale);
        const float orbitLabelH = g_TextS.Height(orbitLabel, orbitLabelScale);
        g_TextS.Draw(orbitLabel,
                     layout.orbitCX - orbitLabelW * 0.5f,
                     layout.orbitCY - orbitLabelH * 0.5f,
                     orbitLabelScale,
                     0.95f, 0.98f, 1.0f,
                     enterE * sceneAlpha * 0.72f);
        BatchFlush();
    }

    for (int i = 0; i < count; ++i) {
        const int augIdx = g_GameManager.augChoices[i];
        if (augIdx < 0 || augIdx >= AUG_TOTAL) continue;
        const AugDef& def = ALL_AUGS[augIdx];
        const bool focused = activeFocus == i && !inExit;
        const bool selected = inExit && g_AugExitSlot == i;
        const bool other = inExit && g_AugExitSlot != i;
        float px = drawX[i];
        float py = drawY[i];
        float radius = drawR[i];
        float alpha = reveal[i] * dimT[i] * sceneAlpha;
        if (selected) {
            px += (layout.orbitCX - px) * collapseT;
            py += (layout.orbitCY - py) * collapseT;
            radius = layout.baseR * (1.0f + collapseT * 0.72f + novaT * 3.4f);
            alpha *= 1.0f - novaT * novaT;
        } else if (other) {
            int selectedSlot = std::max(0, std::min(count - 1, g_AugExitSlot));
            const float centerX = drawX[selectedSlot] +
                                  (layout.orbitCX - drawX[selectedSlot]) * collapseT;
            const float centerY = drawY[selectedSlot] +
                                  (layout.orbitCY - drawY[selectedSlot]) * collapseT;
            const float initialAngle = atan2f(drawY[i] - drawY[selectedSlot],
                                              drawX[i] - drawX[selectedSlot]);
            const float initialDist = sqrtf(
                (drawX[i] - drawX[selectedSlot]) * (drawX[i] - drawX[selectedSlot]) +
                (drawY[i] - drawY[selectedSlot]) * (drawY[i] - drawY[selectedSlot]));
            const float spiralR = initialDist * (1.0f - collapseT);
            const float spiralA = initialAngle + g_AugExitT * 10.5f;
            px = centerX + cosf(spiralA) * spiralR;
            py = centerY + sinf(spiralA) * spiralR;
            radius *= 1.0f - collapseT * 0.92f;
            alpha *= 1.0f - collapseT;
        }
        if (alpha <= 0.004f || radius <= 2.0f) continue;

        float rarityR, rarityG, rarityB;
        GetRarityColor(def.rarity, rarityR, rarityG, rarityB);
        // Rarity is the constellation's material identity. Keep a small state
        // tint for buff/debuff context, but let the tier color drive the whole
        // silhouette: lines, outer nodes, nebula glow, and hover shield.
        const float lineR = stateR * 0.22f + rarityR * 0.78f;
        const float lineG = stateG * 0.22f + rarityG * 0.78f;
        const float lineB = stateB * 0.22f + rarityB * 0.78f;
        const AugVisualFamily family = AugVisualFamilyOf(def);
        DrawAugSelectionConstellation(def, family, px, py, radius,
            selected && novaT > 0.0f ? 1.0f : lineR,
            selected && novaT > 0.0f ? 1.0f : lineG,
            selected && novaT > 0.0f ? 1.0f : lineB,
            rarityR, rarityG, rarityB,
            alpha, reveal[i], shapePhase[i], layout.ui,
            focused || selected, now);

        if (!inExit && shieldT[i] > 0.01f) {
            const bool shieldActive = i == activeFocus;
            const float shieldPulse = shieldActive ? activeShieldPulse : 0.10f;
            const float shieldAlpha = alpha * shieldT[i] *
                (shieldActive ? 0.36f + shieldPulse * 0.62f : 0.20f);
            DrawAugmentShieldOrbit(
                px, py, radius,
                shieldPhase[i], shieldPhase[i] + 0.38f,
                lineR, lineG, lineB,
                shieldAlpha, shieldPulse, layout.ui);
            BatchFlush();
        }

        if (!inExit && reveal[i] > 0.42f) {
            wchar_t keyLabel[16];
            swprintf_s(keyLabel, L"[%d]", i + 1);
            const float keyScale = UiTextScale(g_TextS, UiTextLevel::Subtitle,
                                                1.05f * layout.textUi);
            const float keyW = g_TextS.Width(keyLabel, keyScale);
            const float labelGap = 10.0f * layout.ui;
            float nameFactor = layout.textUi;
            float nameScale = UiTextScale(g_TextS, UiTextLevel::Title, nameFactor);
            const float maxGroupW = layout.orbitR * 1.12f;
            const wchar_t* nameVariants[] = {
                def.locName[0], def.locName[1], def.locName[2]
            };
            while (nameFactor > 0.55f &&
                   keyW + labelGap + MaxLocalizedTextWidth(
                       g_TextS, nameVariants, 3, nameScale) > maxGroupW) {
                nameFactor -= 0.04f;
                nameScale = UiTextScale(g_TextS, UiTextLevel::Title, nameFactor);
            }
            const float nameW = g_TextS.Width(AugName(def), nameScale);
            const float groupW = keyW + labelGap + nameW;
            const float labelY = py + radius + 12.0f * layout.ui;
            const float groupX = px - groupW * 0.5f;
            // Share one baseline: centering two line boxes of different
            // scale left "[1]" and the name visibly offset (QA #4).
            const float nameY = labelY + g_TextS.BaselineOffset(keyScale)
                              - g_TextS.BaselineOffset(nameScale);
            DrawShadowedText(g_TextS, keyLabel,
                groupX, labelY, keyScale,
                focused ? 1.0f : 0.88f,
                focused ? 1.0f : 0.93f,
                focused ? 1.0f : 0.98f,
                alpha * (focused ? 1.0f : 0.90f), 0.70f);
            DrawShadowedText(g_TextS, AugName(def),
                groupX + keyW + labelGap, nameY, nameScale,
                focused ? 1.0f : 0.82f,
                focused ? 1.0f : 0.90f,
                focused ? 1.0f : 0.98f,
                alpha * (focused ? 0.98f : 0.82f), 0.60f);
            BatchFlush();
        }
    }

    const float panelBaseA = enterE * sceneAlpha;
    DrawVisibleConstellLine(layout.panelX - 14.0f * layout.ui,
                            layout.panelY - 8.0f * layout.ui,
                            layout.panelX - 14.0f * layout.ui,
                            layout.panelY + layout.panelH,
                            1.35f * layout.ui,
                            stateR, stateG, stateB,
                            panelBaseA * (activeFocus >= 0 ? 0.50f : 0.20f));
    DrawVisibleConstellNode(layout.panelX - 14.0f * layout.ui,
                            layout.panelY - 8.0f * layout.ui,
                            3.2f * layout.ui,
                            stateR, stateG, stateB,
                            panelBaseA * 0.56f, true, true);
    BatchFlush();

    if (!inExit && activeFocus >= 0) {
        const int augIdx = g_GameManager.augChoices[activeFocus];
        if (augIdx >= 0 && augIdx < AUG_TOTAL) {
            const AugDef& def = ALL_AUGS[augIdx];
            const AugVisualFamily family = AugVisualFamilyOf(def);
            float rarityR, rarityG, rarityB;
            GetRarityColor(def.rarity, rarityR, rarityG, rarityB);
            const float a = panelT * panelBaseA;
            float y = layout.panelY;
            const wchar_t* panelKind = isDebuff ? L"TRIAL COST" : L"BUFF AUGMENT";
            g_TextS.Draw(panelKind, layout.panelX, y,
                         UiTextScale(g_TextS, UiTextLevel::Subtitle, layout.textUi),
                         stateR, stateG, stateB, a * 0.90f);
            wchar_t classText[128];
            swprintf_s(classText, L"%ls  //  %ls",
                       GetAugBadge(def), AugVisualFamilyLabel(family));
            const float classScale = UiTextScale(g_TextS, UiTextLevel::Supporting,
                                                  0.72f * layout.textUi);
            const float classW = g_TextS.Width(classText, classScale);
            g_TextS.Draw(classText,
                         layout.panelX + layout.panelW - classW, y,
                         classScale, rarityR, rarityG, rarityB, a * 0.84f);
            y += 56.0f * layout.ui;

            const wchar_t* name = AugName(def);
            float nameFactor = layout.textUi;
            float nameScale = UiTextScale(g_TextL, UiTextLevel::Title, nameFactor);
            while (nameFactor > 0.55f &&
                   g_TextL.Width(name, nameScale) > layout.panelW) {
                nameFactor -= 0.04f;
                nameScale = UiTextScale(g_TextL, UiTextLevel::Title, nameFactor);
            }
            DrawShadowedText(g_TextL, name, layout.panelX, y,
                             nameScale, 0.98f, 0.99f, 1.0f, a, 0.62f);
            y += 62.0f * layout.ui;

            const wchar_t* stat = AugStat(def);
            float statFactor = layout.textUi;
            float statScale = UiTextScale(g_TextS, UiTextLevel::Description, statFactor);
            while (statFactor > 0.55f &&
                   g_TextS.Width(stat, statScale) > layout.panelW) {
                statFactor -= 0.03f;
                statScale = UiTextScale(g_TextS, UiTextLevel::Description, statFactor);
            }
            g_TextS.Draw(stat, layout.panelX, y, statScale,
                         stateR, stateG, stateB, a * 0.95f);
            y += 54.0f * layout.ui;

            DrawVisibleConstellLine(layout.panelX, y,
                                    layout.panelX + layout.panelW, y,
                                    0.75f * layout.ui,
                                    stateR, stateG, stateB, a * 0.28f);
            BatchFlush();
            y += 18.0f * layout.ui;

            AugImpactRow rows[3];
            const int rowCount = BuildAugImpactRows(def, rows, 3);
            if (rowCount > 0) {
                g_TextS.Draw(L"STAT DELTA", layout.panelX, y,
                             UiTextScale(g_TextS, UiTextLevel::Subtitle,
                                         layout.textUi),
                             0.56f, 0.68f, 0.82f, a * 0.72f);
                y += 46.0f * layout.ui;
                for (int row = 0; row < rowCount; ++row) {
                    wchar_t before[32], after[32], deltaText[32];
                    FormatAugMetricValue(before, 32, rows[row].metric,
                                         rows[row].before, false);
                    FormatAugMetricValue(after, 32, rows[row].metric,
                                         rows[row].after, false);
                    const float diff = rows[row].after - rows[row].before;
                    FormatAugMetricValue(deltaText, 32, rows[row].metric,
                                         diff, true);
                    const wchar_t* label = AugMetricLabel(rows[row].metric);
                    const float metricScale = UiTextScale(g_TextS, UiTextLevel::Supporting,
                                                          0.82f * layout.textUi);
                    g_TextS.Draw(label, layout.panelX, y, metricScale,
                                 0.70f, 0.80f, 0.90f, a * 0.86f);
                    wchar_t values[96];
                    swprintf_s(values, L"%ls  →  %ls", before, after);
                    const float valuesW = g_TextS.Width(values, metricScale);
                    g_TextS.Draw(values,
                        layout.panelX + layout.panelW * 0.70f - valuesW,
                        y, metricScale,
                        0.88f, 0.94f, 1.0f, a * 0.92f);
                    const float deltaW = g_TextS.Width(deltaText, metricScale);
                    const bool positive = diff > 0.0f;
                    g_TextS.Draw(deltaText,
                        layout.panelX + layout.panelW - deltaW,
                        y, metricScale,
                        positive ? 0.14f : 0.98f,
                        positive ? 0.84f : 0.16f,
                        positive ? 0.98f : 0.20f,
                        a * 0.98f);
                    y += 48.0f * layout.ui;
                }
            } else {
                g_TextS.Draw(L"NO DIRECT STAT CHANGE", layout.panelX, y,
                             UiTextScale(g_TextS, UiTextLevel::Supporting,
                                         0.72f * layout.textUi),
                             0.70f, 0.78f, 0.88f, a * 0.72f);
                y += 48.0f * layout.ui;
            }

            // The lower block is deliberately separated from the numbers.
            // It is the explanatory layer: mechanics, conditions, and flavor.
            const float lowerSectionY = layout.panelY + layout.panelH * 0.53f;
            float descriptionY = std::max(y + 22.0f * layout.ui,
                                          lowerSectionY);
            const float descriptionFloor =
                layout.panelY + layout.panelH - 150.0f * layout.ui;
            if (descriptionY > descriptionFloor)
                descriptionY = y + 22.0f * layout.ui;

            DrawVisibleConstellLine(layout.panelX,
                                    descriptionY - 18.0f * layout.ui,
                                    layout.panelX + layout.panelW,
                                    descriptionY - 18.0f * layout.ui,
                                    0.85f * layout.ui,
                                    stateR, stateG, stateB, a * 0.34f);
            DrawVisibleConstellNode(layout.panelX,
                                    descriptionY - 18.0f * layout.ui,
                                    2.4f * layout.ui,
                                    stateR, stateG, stateB,
                                    a * 0.46f, true, true);
            BatchFlush();

            const wchar_t* descriptionHeader =
                isDebuff ? L"ENEMY MODIFIER" : L"MECHANIC";
            g_TextS.Draw(descriptionHeader, layout.panelX, descriptionY,
                         UiTextScale(g_TextS, UiTextLevel::Subtitle,
                                     layout.textUi),
                         stateR, stateG, stateB, a * 0.86f);
            descriptionY += 48.0f * layout.ui;

            const float descScale = UiTextScale(g_TextS, UiTextLevel::Supporting,
                                                layout.textUi);
            const std::vector<std::wstring> lines =
                TarotWrap(def.locDesc[CurLangIdx()], descScale, layout.panelW);
            int drawn = 0;
            for (const std::wstring& line : lines) {
                if (drawn >= 4 ||
                    descriptionY > layout.panelY + layout.panelH - 64.0f * layout.ui)
                    break;
                g_TextS.Draw(line.c_str(), layout.panelX, descriptionY, descScale,
                             0.76f, 0.84f, 0.94f, a * 0.82f);
                descriptionY += g_TextS.Height(line.c_str(), descScale)
                              + 3.0f * layout.ui;
                ++drawn;
            }
            BatchFlush();
        }
    } else if (!inExit) {
        const wchar_t* guide = CurLangIdx() == 0
            ? L"1 / 2 / 3 또는 별자리를 선택"
            : (CurLangIdx() == 2
                ? L"1 / 2 / 3 または星座を選択"
                : L"SELECT A CONSTELLATION OR PRESS 1 / 2 / 3");
        g_TextS.Draw(guide, layout.panelX, layout.panelY,
                     UiTextScale(g_TextS, UiTextLevel::Supporting,
                                 0.48f * layout.textUi),
                     0.54f, 0.66f, 0.80f, panelBaseA * 0.72f);
        BatchFlush();
    }

    if (!inExit) {
        const wchar_t* controls = L"1 / 2 / 3 FOCUS   CLICK / SPACE CONFIRM";
        g_TextS.Draw(controls, layout.panelX, sh * 0.925f,
                     UiTextScale(g_TextS, UiTextLevel::Supporting,
                                 layout.textUi),
                     0.54f, 0.66f, 0.82f, panelBaseA * 0.76f);
        BatchFlush();
    }

    if (inExit && g_AugExitT >= 0.55f && !particlesSpawned &&
        g_AugExitSlot >= 0 && g_AugExitSlot < count) {
        particlesSpawned = true;
        const AugDef& def = ALL_AUGS[g_GameManager.augChoices[g_AugExitSlot]];
        float r, g, b;
        GetRarityColor(def.rarity, r, g, b);
        for (int i = 0; i < 72; ++i) {
            const float angle = (float)i / 72.0f * 2.0f * (float)M_PI;
            const float speed = 120.0f + (float)(rand() % 300);
            particles[i] = { layout.orbitCX, layout.orbitCY,
                cosf(angle) * speed, sinf(angle) * speed,
                0.66f, r, g, b };
        }
        TriggerFlash(r, g, b, 0.78f);
    }
    if (particlesSpawned) {
        BindMainShader();
        for (auto& p : particles) {
            if (p.life <= 0.0f) continue;
            p.life -= delta;
            p.x += p.vx * delta;
            p.y += p.vy * delta;
            const float drag = std::max(0.0f, 1.0f - delta * 2.2f);
            p.vx *= drag;
            p.vy *= drag;
            const float a = AugSelectionClamp(p.life / 0.66f);
            drawDiamond(p.x, p.y, 4.2f * a + 0.8f,
                        p.r, p.g, p.b, a * 0.86f);
        }
        BatchFlush();
    }
}

void Scene_AugSelect(const SceneCtx& c) {
    Scene_AugSelectConstellationPolished(c);
}
static void Scene_AugReplaceCards(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, delta = c.delta;
    const int newIdx = g_GameManager.pendingAugIdx;
    if (newIdx < 0 || newIdx >= AUG_TOTAL) return;

    static int seenSerial = -1;
    static float enterT = 0.0f;
    static float hoverT[32] = {};
    static float flashT[32] = {};
    static double prevMx = -1.0, prevMy = -1.0;

    if (seenSerial != g_GameManager.augmentSelectionSerial) {
        seenSerial = g_GameManager.augmentSelectionSerial;
        enterT = 0.0f;
        prevMx = c.mx;
        prevMy = c.my;
        g_HoveredAug = -1;
        for (int i = 0; i < 32; ++i) { hoverT[i] = 0.0f; flashT[i] = 0.0f; }
    }
    enterT = std::min(1.0f, enterT + delta / 0.36f);
    const bool exiting = g_RepExitT >= 0.0f;
    const float exitT = exiting ? TarotEaseOut(TarotClamp01(g_RepExitT / 0.28f)) : 0.0f;
    const int n = std::max(0, std::min(32, g_GameManager.replaceChoiceCount));
    const int visible = std::min(n, 9);
    const bool pointerMoved = prevMx < 0.0 ||
        fabs(c.mx - prevMx) > 0.5 || fabs(c.my - prevMy) > 0.5;
    if (pointerMoved) g_GameManager.augmentKeyboardFocus = false;
    prevMx = c.mx;
    prevMy = c.my;

    const float ui = UiScale(sw, sh);
    const float panelX = sw * 0.705f;
    const float panelW = std::max(230.0f, sw - panelX - 24.0f);
    const float cardAreaW = std::max(420.0f, panelX - 34.0f);
    const int cols = std::max(1, std::min(3, visible));
    const int rows = std::max(1, (visible + cols - 1) / cols);
    const float gap = 16.0f * ui;
    const float cardW = std::min(225.0f * ui, (cardAreaW - gap * (cols - 1)) / cols);
    const float cardH = std::min(302.0f * ui, (sh * 0.64f - gap * (rows - 1)) / rows);
    const float gridW = cardW * cols + gap * (cols - 1);
    const float baseX = (cardAreaW - gridW) * 0.5f;
    const float baseY = sh * 0.255f;

    auto PoseFor = [&](int i, float yOffset = 0.0f) {
        const int col = i % cols;
        const int row = i / cols;
        TarotCardPose p;
        p.cx = baseX + col * (cardW + gap) + cardW * 0.5f;
        p.cy = baseY + row * (cardH + gap) + cardH * 0.5f + yOffset;
        p.w = cardW;
        p.h = cardH;
        p.angle = 0.0f;
        return p;
    };

    if (!exiting && enterT > 0.95f &&
        (!g_GameManager.augmentKeyboardFocus || pointerMoved)) {
        int hover = -1;
        for (int i = visible - 1; i >= 0; --i)
            if (TarotHit(PoseFor(i), c.mx, c.my, 10.0f)) { hover = i; break; }
        g_HoveredAug = hover;
    }
    if (g_HoveredAug >= 0 && g_HoveredAug < 32 &&
        g_HoveredAug < g_GameManager.replaceChoiceCount)
        flashT[g_HoveredAug] = std::max(flashT[g_HoveredAug], 0.18f);
    for (int i = 0; i < 32; ++i) {
        const float target = (!exiting && g_HoveredAug == i) ? 1.0f : 0.0f;
        hoverT[i] += (target - hoverT[i]) * std::min(1.0f, delta * 13.0f);
        flashT[i] = std::max(0.0f, flashT[i] - delta);
    }
    if (!exiting && c.lmb && !g_LmbPrev && g_HoveredAug >= 0 &&
        g_HoveredAug < visible) {
        g_RepExitT = 0.0f;
        g_RepExitSlot = g_HoveredAug;
    }

    const float enterE = TarotEaseOut(enterT);
    const float overlay = enterE * (1.0f - exitT);
    BindMainShader();
    drawRect(0, 0, sw, sh, 0.025f, 0.010f, 0.045f, overlay * 0.78f);
    drawConstellFrame(16.0f, 16.0f, sw - 32.0f, sh - 32.0f,
                      0.72f, 0.30f, 0.96f, overlay * 0.60f,
                      24.0f, 8.0f, overlay * 0.24f, enterT);
    BatchFlush();

    const wchar_t* title = L"IDENTITY SLOT // REPLACE";
    g_TextL.Draw(title, 24.0f * ui, sh * 0.075f,
                 UiTextScale(g_TextL, UiTextLevel::Title, ui),
                 1.0f, 0.96f, 1.0f, overlay * 0.96f);
    wchar_t newLine[160];
    swprintf_s(newLine, L"NEW  [%ls] %ls", GetAugBadge(ALL_AUGS[newIdx]),
               AugName(ALL_AUGS[newIdx]));
    g_TextS.Draw(newLine, 26.0f * ui, sh * 0.125f,
                 UiTextScale(g_TextS, UiTextLevel::Supporting, ui),
                 0.82f, 0.92f, 1.0f, overlay * 0.88f);
    BatchFlush();

    for (int i = 0; i < visible; ++i) {
        const int idx = g_GameManager.replaceChoices[i];
        if (idx < 0 || idx >= AUG_TOTAL) continue;
        const AugDef& def = ALL_AUGS[idx];
        float r, g, b;
        GetRarityColor(def.rarity, r, g, b);
        const float stagger = TarotEaseOut(TarotClamp01((enterT - i * 0.05f) / 0.72f));
        TarotCardPose p = PoseFor(i, (1.0f - stagger) * 54.0f);
        const float hT = TarotEaseOut(hoverT[i]);
        p.cy -= hT * 14.0f;
        p.w *= 1.0f + hT * 0.045f;
        p.h *= 1.0f + hT * 0.045f;
        const bool selected = exiting && g_RepExitSlot == i;
        const bool other = exiting && !selected;
        float alpha = stagger * (g_HoveredAug >= 0 && g_HoveredAug != i ? 0.42f : 1.0f);
        if (selected) {
            p.cx += (sw * 0.50f - p.cx) * exitT;
            p.cy += (sh * 0.49f - p.cy) * exitT;
            p.w *= 1.0f + exitT * 0.25f;
            p.h *= 1.0f + exitT * 0.25f;
            alpha *= 1.0f - exitT * 0.78f;
        } else if (other) {
            p.cy += (i & 1 ? -1.0f : 1.0f) * exitT * 30.0f;
            alpha *= 1.0f - exitT;
        }
        DrawTarotCardSurface(p, r, g, b, alpha, hT, flashT[i], selected,
                             (float)glfwGetTime());
        const float left = p.cx - p.w * 0.5f;
        const float top = p.cy - p.h * 0.5f;
        wchar_t key[16];
        swprintf_s(key, L"[%d]", i + 1);
        g_TextS.Draw(key, left + 14.0f * ui, top + 11.0f * ui,
                     UiTextScale(g_TextS, UiTextLevel::Supporting, ui),
                     0.80f, 0.86f, 1.0f, alpha * 0.86f);
        const wchar_t* name = AugName(def);
        const wchar_t* nameVariants[] = {
            def.locName[0], def.locName[1], def.locName[2]
        };
        float nameFactor = ui;
        float nameScale = UiTextScale(g_TextL, UiTextLevel::Title, nameFactor);
        while (nameFactor > 0.55f && MaxLocalizedTextWidth(
                   g_TextL, nameVariants, 3, nameScale) > p.w - 26.0f * ui) {
            nameFactor -= 0.04f;
            nameScale = UiTextScale(g_TextL, UiTextLevel::Title, nameFactor);
        }
        const float nameW = g_TextL.Width(name, nameScale);
        g_TextL.Draw(name, p.cx - nameW * 0.5f, top + 38.0f * ui,
                     nameScale, 1.0f, 1.0f, 1.0f, alpha * 0.96f);
        BatchFlush();
        DrawTarotEmblem(def, p.cx, p.cy - 8.0f, p.w * 0.40f,
                        r, g, b, alpha * 0.86f, (float)glfwGetTime());
        const wchar_t* stat = AugStat(def);
        const float statScale = UiTextScale(g_TextS, UiTextLevel::Supporting, ui);
        const float statW = g_TextS.Width(stat, statScale);
        g_TextS.Draw(stat, p.cx - statW * 0.5f, top + p.h - 46.0f * ui,
                     statScale, 0.72f, 1.0f, 0.82f, alpha * 0.84f);
        BatchFlush();
    }

    const float detailY = sh * 0.245f;
    DrawTarotDetailPanel(ALL_AUGS[newIdx], panelX, detailY, panelW,
                         std::min(sh * 0.48f, 390.0f), overlay * 0.96f);
    const wchar_t* hint = L"1-9 SELECT   SPACE REPLACE   ESC CANCEL";
    g_TextS.Draw(hint, 24.0f * ui, sh * 0.92f,
                 UiTextScale(g_TextS, UiTextLevel::Supporting, ui),
                 0.62f, 0.72f, 0.88f, overlay * 0.80f);
    BatchFlush();
}

void Scene_AugReplace(const SceneCtx& c) {
    Scene_AugReplaceCards(c);
}
void Scene_OwnedAugPanel(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                // 같은 인덱스 카운트 (스택)
                int counts[AUG_TOTAL] = {};
                for (int idx : g_OwnedAugs)
                    if (idx >= 0 && idx < AUG_TOTAL && !AugRemoved(ALL_AUGS[idx].type))
                        counts[idx]++;
                // 보유 증강을 티어(등급)순으로 정렬
                int ord[AUG_TOTAL], nord = 0;
                for (int i = 0; i < AUG_TOTAL; i++) if (counts[i] > 0) ord[nord++] = i;
                std::sort(ord, ord + nord, [](int a, int b) {
                    return AugTierIndexLess(a, b);
                });

                const float PX  = 16.0f;
                const float COLW = 320.0f;          // 리스트 클릭/호버 가로 범위
                const wchar_t* TITLE = T(StrId::OWNED_AUGS);
                const float listUiScale = UiScale(sw, sh);
                const float categoryScale = UiTextScale(
                    g_TextS, UiTextLevel::Supporting, listUiScale);
                const float augmentScale = UiTextScale(
                    g_TextS, UiTextLevel::Description, listUiScale);
                const float categoryH = g_TextS.Height(L"A", categoryScale);
                const float augmentH = g_TextS.Height(L"A", augmentScale);
                const float weaponH = categoryH;
                const float ROW_H = augmentH + 8.0f * listUiScale;
                const float HDR_H = categoryH + 6.0f * listUiScale;
                const float titleY = 60.0f;
                const float titleScale = UiTextScale(
                    g_TextS, UiTextLevel::Subtitle, listUiScale);
                const float weaponY = titleY + g_TextS.Height(TITLE, titleScale)
                                    + 8.0f * listUiScale;
                g_TextS.Draw(TITLE, PX, titleY, titleScale, 1, 1, 1, 0.95f);
                // 현재 무기 (항상 표시)
                {
                    const wchar_t* wPrefix = (g_Language==Language::EN)?L"Weapon:":
                                             (g_Language==Language::JP)?L"武器:":
                                             L"현재 무기:";
                    wchar_t wLine[160];
                    swprintf_s(wLine, L"%ls %ls", wPrefix, CurrentWeaponLabel());
                    float wFactor = listUiScale;
                    float weaponScale = UiTextScale(
                        g_TextS, UiTextLevel::Supporting, wFactor);
                    while (wFactor > 0.55f &&
                           g_TextS.Width(wLine, weaponScale) > COLW - 4.0f) {
                        wFactor -= 0.03f;
                        weaponScale = UiTextScale(
                            g_TextS, UiTextLevel::Supporting, wFactor);
                    }
                    g_TextS.Draw(wLine, PX + 2.0f, weaponY, weaponScale,
                                 0.55f, 0.85f, 1.0f, 0.92f);
                }

                // 리스트 뷰 영역 — 하단 스킬/HP HUD 바로 위까지 (넘치면 스크롤)
                const float listTop = weaponY + weaponH + 12.0f * listUiScale;
                float listBottom = sh - 175.0f;               // 스킬 슬롯/HP 패널 위까지
                if (listBottom < listTop + 4.0f * ROW_H) listBottom = listTop + 4.0f * ROW_H;
                const float viewH      = listBottom - listTop;
                int hdrCount = 0;
                AugRarity prevR = (AugRarity)-1;
                for (int oi = 0; oi < nord; oi++) {
                    AugRarity r = ALL_AUGS[ord[oi]].rarity;
                    if (r != prevR) { hdrCount++; prevR = r; }
                }
                const float contentH   = (float)nord * ROW_H + (float)hdrCount * HDR_H;

                // 마우스 휠 스크롤 (리스트 위에서만 소비)
                static float s_ownScroll = 0.0f;
                bool overList = (mx >= 0 && mx <= COLW && my >= listTop && my <= listBottom);
                if (overList && g_ScrollAccum != 0.0f)
                    s_ownScroll -= g_ScrollAccum * ROW_H * 1.5f;
                g_ScrollAccum = 0.0f;   // 매 프레임 소비 (다른 곳에서 안 쓰면 무시)
                float maxScroll = (contentH > viewH) ? (contentH - viewH) : 0.0f;
                if (s_ownScroll < 0.0f)        s_ownScroll = 0.0f;
                if (s_ownScroll > maxScroll)   s_ownScroll = maxScroll;

                // 리스트 (scissor 클립 + 스크롤)
                int   hoverAug = -1;
                float hoverRowY = 0.0f;
                BatchFlush(); glEnable(GL_SCISSOR_TEST);
                glScissor(0, (GLint)(sh - listBottom), (GLint)(COLW + 10.0f), (GLint)viewH);
                prevR = (AugRarity)-1;
                float ry = listTop - s_ownScroll;
                for (int oi = 0; oi < nord; oi++) {
                    int i = ord[oi];
                    AugRarity rar = ALL_AUGS[i].rarity;
                    if (rar != prevR) {
                        if (ry >= listTop - HDR_H && ry <= listBottom) {
                            wchar_t rh[48];
                            swprintf_s(rh, L"-- %ls --", GetRarityKR(rar));
                            g_TextS.Draw(rh, PX, ry, categoryScale,
                                         0.55f, 0.75f, 0.95f, 0.88f);
                        }
                        ry += HDR_H;
                        prevR = rar;
                    }
                    if (ry < listTop - ROW_H || ry > listBottom) { ry += ROW_H; continue; }
                    const AugDef& def = ALL_AUGS[i];
                    float cr, cg, cb;
                    GetRarityColor(def.rarity, cr, cg, cb);
                    cr = std::min(1.0f, cr * 1.3f + 0.25f);
                    cg = std::min(1.0f, cg * 1.3f + 0.25f);
                    cb = std::min(1.0f, cb * 1.3f + 0.25f);
                    // The highlight and its hit band pad the text line equally
                    // above and below so the bar sits centered on the label.
                    const float rowTop = ry - (ROW_H - augmentH) * 0.5f;
                    bool rowHover = (overList && my >= rowTop && my < rowTop + ROW_H);
                    if (rowHover) {
                        hoverAug = i; hoverRowY = ry;
                        BindMainShader();
                        drawRect(PX, rowTop, COLW - PX, ROW_H,
                                 0.15f, 0.16f, 0.26f, 0.6f);
                        drawRect(PX, rowTop, 3.0f, ROW_H, cr, cg, cb, 1.0f);
                    }
                    wchar_t line[128];
                    if (counts[i] > 1)
                        swprintf_s(line, L"· [%ls] %ls  ×%d", GetAugBadge(def), AugName(def), counts[i]);
                    else
                        swprintf_s(line, L"· [%ls] %ls", GetAugBadge(def), AugName(def));
                    g_TextS.Draw(line, PX + 8.0f * listUiScale, ry,
                                 augmentScale, cr, cg, cb, 0.9f);
                    ry += ROW_H;
                }
                BatchFlush(); glDisable(GL_SCISSOR_TEST);

                // 스크롤바 (내용이 넘칠 때만)
                if (maxScroll > 0.0f) {
                    BindMainShader();
                    float trackX = COLW + 2.0f;
                    drawRect(trackX, listTop, 4.0f, viewH, 0.12f, 0.12f, 0.16f, 0.6f);
                    float thumbH = viewH * (viewH / contentH);
                    float thumbY = listTop + (viewH - thumbH) * (s_ownScroll / maxScroll);
                    drawRect(trackX, thumbY, 4.0f, thumbH, 0.5f, 0.6f, 0.8f, 0.9f);
                }

                // 무기 줄 호버 (리스트 y=110 이전 — 증강 행과 분리)
                bool overWeapon = (mx >= 0 && mx <= COLW &&
                                   my >= weaponY - 4.0f &&
                                   my <= weaponY + weaponH + 4.0f * listUiScale);

                // 우측 상세 패널 — 증강 행: 증강 설명 / 무기 줄: 무기 설명
                auto wrapDescLines = [&](const wchar_t* src, float dsc, float dWmax,
                                         std::vector<std::wstring>& out) {
                    std::vector<std::wstring> dl; std::wstring cur2;
                    for (const wchar_t* p = src; *p; ++p) {
                        const bool spacedSlash = *p == L'/' && p > src && p[1] != L'\0'
                                              && p[-1] == L' ' && p[1] == L' ';
                        if (*p == L'\xB7' || spacedSlash) {
                            if (!cur2.empty()) dl.push_back(cur2);
                            cur2.clear();
                        } else cur2 += *p;
                    }
                    if (!cur2.empty()) dl.push_back(cur2);
                    out.clear();
                    for (auto& ln : dl) {
                        while (!ln.empty() && ln.front() == L' ') ln.erase(0, 1);
                        if (g_TextS.Width(ln.c_str(), dsc) <= dWmax) { out.push_back(ln); continue; }
                        std::wstring acc, word;
                        auto fw = [&]() {
                            if (word.empty()) return;
                            std::wstring tr = acc.empty() ? word : acc + L" " + word;
                            if (g_TextS.Width(tr.c_str(), dsc) > dWmax && !acc.empty()) {
                                out.push_back(acc); acc = word;
                            } else acc = tr;
                            word.clear();
                        };
                        for (wchar_t ch : ln) { if (ch == L' ') fw(); else word += ch; }
                        fw();
                        if (!acc.empty()) out.push_back(acc);
                    }
                };

                auto drawSidePanel = [&](float BY, float br, float bg, float bb,
                                         const wchar_t* title, const wchar_t* descSrc) {
                    const float BW = std::min(380.0f, sw - (COLW + 30.0f));
                    const float BH = 138.0f;
                    float BX = COLW + 18.0f;
                    if (BY + BH > sh - 20.0f) BY = sh - 20.0f - BH;
                    if (BY < 20.0f) BY = 20.0f;
                    BindMainShader();
                    drawRect(BX, BY, BW, BH, 0.03f, 0.03f, 0.06f, 0.95f);
                    drawRect(BX, BY, BW, 4.0f, br, bg, bb, 1.0f);
                    g_TextS.Draw(title, BX + 12.0f, BY + 12.0f, 0.90f,
                                 std::min(1.0f, br * 1.4f + 0.3f),
                                 std::min(1.0f, bg * 1.4f + 0.3f),
                                 std::min(1.0f, bb * 1.4f + 0.3f), 1.0f);
                    const float dsc = UiTextScale(g_TextS, UiTextLevel::Supporting, listUiScale);
                    const float dWmax = BW - 24.0f;
                    std::vector<std::wstring> wrapped;
                    wrapDescLines(descSrc, dsc, dWmax, wrapped);
                    float dy = BY + 40.0f;
                    for (auto& w : wrapped) {
                        if (dy > BY + BH - 14.0f) break;
                        g_TextS.Draw(w.c_str(), BX + 12.0f, dy, dsc, 1.0f, 1.0f, 0.95f, 0.90f);
                        dy += 22.0f;
                    }
                };

                if (hoverAug >= 0) {
                    const AugDef& sd = ALL_AUGS[hoverAug];
                    float hr, hg, hb;
                    GetRarityColor(sd.rarity, hr, hg, hb);
                    wchar_t hd[128];
                    swprintf_s(hd, L"[%ls] %ls", GetAugBadge(sd), AugName(sd));
                    drawSidePanel(hoverRowY - 6.0f, hr, hg, hb, hd, AugDesc(sd));
                } else if (overWeapon) {
                    drawSidePanel(weaponY - 4.0f, 0.35f, 0.75f, 1.0f,
                                  CurrentWeaponLabel(), CurrentWeaponDescText());
                }
}

