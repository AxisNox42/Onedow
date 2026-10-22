#include "EntityDraw.h"
#include "../Entity/MonsterManager.h"
#include "../Entity/GravisField.h"
#include "Camera.h"
#include "DrawPrim.h"
#include "IconSystem.h"
#include "Codex.h"
#include "RangedMob.h"
#include "TextRenderer.h"
#include "UiColors.h"
#include "Settings.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <vector>

extern TextRenderer g_TextS;
extern float g_RfwW;

static void ApplyMobStyleTint(float& r, float& g, float& b) {
    if (g_MobVisualStyle != MobVisualStyle::SOFT) return;
    r = r * 0.72f + 0.14f;
    g = g * 0.72f + 0.16f;
    b = b * 0.72f + 0.20f;
}

static void ApplyMobHitFlash(float& r, float& g, float& b, float timer) {
    const float t = std::max(0.0f, std::min(1.0f,
        timer / MOB_HIT_FLASH_TIME));
    const float flash = t * t * (3.0f - 2.0f * t);
    r += (1.0f - r) * flash;
    g += (1.0f - g) * flash;
    b += (1.0f - b) * flash;
}

static void drawLineQuad(float x1, float y1, float x2, float y2, float width,
                         float r, float g, float b, float a) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;
    float px = -dy / len * width * 0.5f;
    float py =  dx / len * width * 0.5f;
    BatchTri(x1 + px, y1 + py, x2 + px, y2 + py, x2 - px, y2 - py, r, g, b, a);
    BatchTri(x1 + px, y1 + py, x2 - px, y2 - py, x1 - px, y1 - py, r, g, b, a);
}

void DrawApproachOrb(float x, float y, float visualTime) {
    const float pulse = 0.5f + 0.5f * sinf(visualTime * 4.0f);
    const float phase = visualTime * 0.65f;
    drawCircle(x, y, 22.0f + pulse * 3.0f,
               0.88f, 0.08f, 0.14f, 0.08f + pulse * 0.05f);
    drawCircle(x, y, 11.0f, 0.26f, 0.015f, 0.045f, 0.82f);
    float px[4], py[4];
    for (int i = 0; i < 4; ++i) {
        const float angle = phase + (float)i * 1.5707963f;
        px[i] = x + cosf(angle) * 17.0f;
        py[i] = y + sinf(angle) * 17.0f;
    }
    for (int i = 0; i < 4; ++i) {
        const int next = (i + 1) % 4;
        drawLineQuad(px[i], py[i], px[next], py[next],
                     1.4f, 1.0f, 0.20f, 0.25f, 0.78f);
        drawLineQuad(x, y, px[i], py[i],
                     0.9f, 1.0f, 0.28f, 0.32f, 0.50f);
        drawCircle(px[i], py[i], 2.6f, 1.0f, 0.34f, 0.36f, 0.92f);
    }
    drawCircle(x, y, 4.2f + pulse * 1.2f,
               1.0f, 0.62f, 0.48f, 0.96f);
}

void DrawAppWindow(float wx, float wy, float w, float h, const wchar_t* title, float tb) {
    // App bounds stay readable without an opaque window so the transparent
    // observatory background remains visible.
    drawConstellFrame(wx, wy, w, h, 0.48f, 0.88f, 1.0f, 0.36f,
                      std::max(14.0f, std::min(28.0f, std::min(w, h) * 0.12f)),
                      3.0f, 0.05f, 1.0f);
    drawRect(wx + 8.0f, wy + std::max(4.0f, tb * 0.5f),
             std::max(12.0f, w - 16.0f), 1.5f,
             0.48f, 0.88f, 1.0f, 0.28f);
    BatchFlush();
    if (title) {
        float tScale = (tb <= 16.0f) ? 0.38f : 0.5f;
        float ty = wy + (tb - 11.0f * tScale) * 0.5f;
        if (ty < wy + 1.0f) ty = wy + 1.0f;
        g_TextS.Draw(title, wx + 8.0f, ty, tScale, 0.82f, 0.94f, 1.0f, 0.90f);
    }
    BatchFlush();
}

bool inWin(float x, float y, float rx, float ry, float rw, float rh, float margin) {
    return x >= rx - margin && x <= rx + rw + margin &&
           y >= ry - margin && y <= ry + rh + margin;
}

static void DrawDiscWithTexture(float x, float y, float radius,
                                float r, float g, float b, float alpha,
                                GLuint texture);

void drawBullet(const Bullet& b) {
    float r = 4.8f * b.sizeScale;
    if (b.rainMissile && g_RainMissileTex) {
        float ang = std::atan2(b.dirX, -b.dirY);
        // Dark keyline disc so the missile icon separates from bright
        // wallpapers without a plate behind the battlefield.
        drawCircle(b.x, b.y, 9.0f,
                   b.color.r * 0.22f, b.color.g * 0.22f, b.color.b * 0.22f,
                   0.40f * CombatKeylineStrength());
        drawCircle(b.x - b.dirX * 10.0f, b.y - b.dirY * 10.0f, r * 1.3f,
                   1.0f, 0.6f, 0.2f, 0.5f);
        BatchFlush();
        // The main projection already applies camera zoom to world geometry.
        DrawIconRot(g_RainMissileTex, b.x, b.y, 12.0f, 15.5f, ang,
                    b.color.r, b.color.g, b.color.b, 1.0f);
        BindMainShader();
        return;
    }
    if (b.crescentBlade) {
        float spinAng   = atan2f(b.dirY, b.dirX) + 1.5708f;
        float grow      = 1.0f + b.traveled / 160.0f;
        if (grow > 3.8f) grow = 3.8f;
        float outerR  = r * 3.5f * grow;
        float innerR  = r * 1.5f * grow;
        float sweep   = 2.5f;
        int   segs    = 14;
        for (int i = 0; i < segs; i++) {
            float a0 = spinAng - sweep*0.5f + sweep*(float)i/segs;
            float a1 = spinAng - sweep*0.5f + sweep*(float)(i+1)/segs;
            BatchTri(b.x+cosf(a0)*innerR*0.4f, b.y+sinf(a0)*innerR*0.4f,
                     b.x+cosf(a0)*(outerR+12.f), b.y+sinf(a0)*(outerR+12.f),
                     b.x+cosf(a1)*(outerR+12.f), b.y+sinf(a1)*(outerR+12.f),
                     b.color.r, b.color.g, b.color.b, 0.07f);
            BatchTri(b.x+cosf(a0)*innerR*0.4f, b.y+sinf(a0)*innerR*0.4f,
                     b.x+cosf(a1)*(outerR+12.f), b.y+sinf(a1)*(outerR+12.f),
                     b.x+cosf(a1)*innerR*0.4f, b.y+sinf(a1)*innerR*0.4f,
                     b.color.r, b.color.g, b.color.b, 0.07f);
        }
        for (int i = 0; i < segs; i++) {
            float a0 = spinAng - sweep*0.5f + sweep*(float)i/segs;
            float a1 = spinAng - sweep*0.5f + sweep*(float)(i+1)/segs;
            BatchTri(b.x+cosf(a0)*innerR, b.y+sinf(a0)*innerR,
                     b.x+cosf(a0)*outerR, b.y+sinf(a0)*outerR,
                     b.x+cosf(a1)*outerR, b.y+sinf(a1)*outerR,
                     b.color.r, b.color.g, b.color.b, 0.90f);
            BatchTri(b.x+cosf(a0)*innerR, b.y+sinf(a0)*innerR,
                     b.x+cosf(a1)*outerR, b.y+sinf(a1)*outerR,
                     b.x+cosf(a1)*innerR, b.y+sinf(a1)*innerR,
                     b.color.r, b.color.g, b.color.b, 0.90f);
        }
        for (int i = 0; i < segs; i++) {
            float a0 = spinAng - sweep*0.5f + sweep*(float)i/segs;
            float a1 = spinAng - sweep*0.5f + sweep*(float)(i+1)/segs;
            BatchTri(b.x+cosf(a0)*outerR,        b.y+sinf(a0)*outerR,
                     b.x+cosf(a0)*(outerR+3.5f),  b.y+sinf(a0)*(outerR+3.5f),
                     b.x+cosf(a1)*(outerR+3.5f),  b.y+sinf(a1)*(outerR+3.5f),
                     1.0f, 1.0f, 1.0f, 0.62f);
            BatchTri(b.x+cosf(a0)*outerR,        b.y+sinf(a0)*outerR,
                     b.x+cosf(a1)*(outerR+3.5f),  b.y+sinf(a1)*(outerR+3.5f),
                     b.x+cosf(a1)*outerR,         b.y+sinf(a1)*outerR,
                     1.0f, 1.0f, 1.0f, 0.62f);
        }
        return;
    }
    if (!b.isEnemy) {
        float trailLen = b.speed * 0.020f;
        if (trailLen > 5.0f) {
            float tx = b.x - b.dirX * trailLen;
            float ty = b.y - b.dirY * trailLen;
            float px = -b.dirY * r, py = b.dirX * r;
            float v[6] = { b.x + px, b.y + py, b.x - px, b.y - py, tx, ty };
            BatchVerts(v, 3, b.color.r, b.color.g, b.color.b, 0.38f);
        }
    } else if (b.shootableEnemy) {
        float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * 18.0f + b.x * 0.02f);
        drawCircle(b.x, b.y, r * (1.75f + pulse * 0.20f), 0.34f, 1.0f, 0.82f, 0.14f + pulse * 0.10f);
        drawCircle(b.x, b.y, r * 1.28f, 0.86f, 1.0f, 0.96f, 0.20f);
    }
    // Thin rim tinted from the bullet's own colour: keeps it readable on
    // white or busy wallpapers without reading as a grey outline.
    const float keyline = CombatKeylineStrength();
    drawCircle(b.x, b.y, r + 1.2f * keyline,
               b.color.r * 0.22f, b.color.g * 0.22f, b.color.b * 0.22f,
               0.70f * keyline);
    drawCircle(b.x, b.y, r, b.color.r, b.color.g, b.color.b, 1.0f);
}

static void DrawEnemyWirePolygon(const float* vx, const float* vy, int count,
                                 float r, float g, float b, float alpha,
                                 float glowAlpha, float width = 1.1f) {
    for (int i = 0; i < count; ++i) {
        int next = (i + 1) % count;
        drawLineQuad(vx[i], vy[i], vx[next], vy[next], width + 1.0f,
                     r, g, b, glowAlpha);
        drawLineQuad(vx[i], vy[i], vx[next], vy[next], width,
                     r, g, b, alpha);
    }
}

static void DrawDiscWithTexture(float x, float y, float radius,
                                float r, float g, float b, float alpha,
                                GLuint texture) {
    if (texture && g_IconProg) {
        // Icon rendering changes the GL pass. Flush the pending geometry so
        // each textured halo remains behind the node that owns it.
        BatchFlush();
        // DrawIcon shares g_MainOrtho with world geometry, so keep this quad
        // in world space and let the camera apply zoom exactly once.
        const WorldDiscProjection projection = ProjectWorldDisc(x, y, radius);
        DrawIcon(texture, projection.worldX, projection.worldY,
                 projection.worldSize, projection.worldSize,
                 r, g, b, alpha);
    } else {
        drawCircle(x, y, radius, r, g, b, alpha);
    }
}

void DrawBulletHalos(const std::vector<Bullet>& bullets) {
    const GLuint texture = g_ConstellationCircleTex
        ? g_ConstellationCircleTex : g_InGameCircleTex;
    static std::vector<IconBatchQuad> outerQuads;
    static std::vector<IconBatchQuad> innerQuads;
    static std::vector<IconBatchQuad> quads;
    outerQuads.clear();
    innerQuads.clear();
    outerQuads.reserve(bullets.size());
    innerQuads.reserve(bullets.size());
    quads.clear();
    quads.reserve(bullets.size() * 2u);
    for (const auto& bullet : bullets) {
        if (!bullet.active) continue;
        const BulletHaloStyle style = GetBulletHaloStyle(bullet);
        const WorldDiscProjection outer = ProjectWorldDisc(
            bullet.x, bullet.y, style.outerRadius);
        if (outer.screenX + outer.screenRadius < 0.0f ||
            outer.screenX - outer.screenRadius > (float)screenWidth ||
            outer.screenY + outer.screenRadius < 0.0f ||
            outer.screenY - outer.screenRadius > (float)screenHeight)
            continue;

        if (texture && g_IconBatchProg) {
            const WorldDiscProjection inner = ProjectWorldDisc(
                bullet.x, bullet.y, style.innerRadius);
            outerQuads.push_back({outer.worldX, outer.worldY,
                                  outer.worldSize, outer.worldSize,
                                  bullet.color.r, bullet.color.g, bullet.color.b,
                                  style.outerAlpha});
            innerQuads.push_back({inner.worldX, inner.worldY,
                                  inner.worldSize, inner.worldSize,
                                  bullet.color.r, bullet.color.g, bullet.color.b,
                                  style.innerAlpha});
        } else {
            drawCircle(bullet.x, bullet.y, style.outerRadius,
                       bullet.color.r, bullet.color.g, bullet.color.b,
                       style.outerAlpha * 0.35f);
            drawCircle(bullet.x, bullet.y, style.innerRadius,
                       bullet.color.r, bullet.color.g, bullet.color.b,
                       style.innerAlpha * 0.35f);
        }
    }
    if (texture && g_IconBatchProg) {
        quads.clear();
        quads.insert(quads.end(), outerQuads.begin(), outerQuads.end());
        quads.insert(quads.end(), innerQuads.begin(), innerQuads.end());
        DrawIconBatch(texture, quads);
    }
}

struct EnemySightFrontMarker {
    float x, y, radius;
    float r, g, b, alpha;
};

static std::vector<EnemySightFrontMarker> g_EnemySightFrontMarkers;

void BeginEnemySightFrontBatch() {
    g_EnemySightFrontMarkers.clear();
}

static void QueueEnemySightFront(float x, float y, float radius,
                                 float r, float g, float b, float alpha) {
    if (radius <= 0.0f || radius != radius || alpha <= 0.0f) return;
    g_EnemySightFrontMarkers.push_back({x, y, radius, r, g, b, alpha});
}

void QueueMonsterSightFront(const Monster* m) {
    if (!m || !m->alive || m->sizeScale <= 0.0f) return;
    const float base = (m->summoned ? 28.0f : 18.0f) * m->sizeScale;
    float r = m->color.r, g = m->color.g, b = m->color.b;
    ApplyMobStyleTint(r, g, b);
    float alpha = 0.11f;
    float radiusScale = 2.55f;
    if (m->kind == MobKind::GENESIS) {
        const float openLift = m->hivePhase == 1
            ? 0.12f * m->hiveOpenFactor : 0.0f;
        const float pulseT = std::min(m->hivePulseTimer / 0.22f, 1.0f);
        const float pulseEase = pulseT * pulseT * (3.0f - 2.0f * pulseT);
        alpha = std::min(0.46f, 0.16f + openLift + 0.34f * pulseEase);
        radiusScale = 3.10f;   // tinted glow behind the whole shell ring
        if(m->GenesisRotorMode()) alpha=0.06f;
    } else if(m->genesisThrown && m->kind==MobKind::ROTOR) {
        alpha=0.04f;
        radiusScale=2.1f;
    } else if (m->genesisEgress) {
        const float egressT = std::min(m->genesisEgressTimer / 0.20f, 1.0f);
        alpha += 0.22f * (1.0f - egressT);
    } else if (m->kind == MobKind::GRAVIS) {
        // The tier-3 gravity controller needs a stronger local silhouette than
        // ordinary mobs; otherwise its field and large body swallow the disc.
        alpha = 0.24f;
        radiusScale = 3.35f;
    } else if (m->kind == MobKind::QUASAR) {
        alpha = 0.18f;
        radiusScale = 3.00f;   // tinted glow behind the halo and tilted orbit
    } else if (m->kind == MobKind::GIMBAL) {
        alpha = 0.19f;
        radiusScale = 2.35f;
    } else if (m->kind == MobKind::ANTARES) {
        alpha = 0.15f + 0.07f * m->AntaresSpeedRatio();
    } else if (m->kind == MobKind::MAGNETAR) {
        alpha = m->magnetarInterceptTimer > 0.0f ? 0.25f : 0.16f;
        radiusScale = 2.20f;
    }
    QueueEnemySightFront(m->worldX, m->worldY, base * radiusScale,
                         r, g, b, alpha);
}

void QueueRangedMobSightFront(const RangedMob* mob) {
    if (!mob || mob->deathScale <= 0.0f) return;
    const float base = RangedMob::VISUAL_BASE_PX * mob->deathScale;
    const float alpha = mob->lensState == RangedMob::State::BURST
        ? 0.17f : 0.11f;
    QueueEnemySightFront(mob->worldX, mob->worldY, base * 2.65f,
                         mob->color.r, mob->color.g, mob->color.b, alpha);
}

void FlushEnemySightFrontBatch() {
    if (g_EnemySightFrontMarkers.empty()) return;
    if (!(g_ConstellationCircleTex && g_IconBatchProg)) {
        for (const auto& marker : g_EnemySightFrontMarkers)
            drawCircle(marker.x, marker.y, marker.radius,
                       marker.r, marker.g, marker.b, marker.alpha);
        return;
    }

    static std::vector<IconBatchQuad> quads;
    quads.clear();
    quads.reserve(g_EnemySightFrontMarkers.size());
    for (const auto& marker : g_EnemySightFrontMarkers) {
        const WorldDiscProjection projection = ProjectWorldDisc(
            marker.x, marker.y, marker.radius);
        if (projection.screenX + projection.screenRadius < 0.0f ||
            projection.screenX - projection.screenRadius > (float)screenWidth ||
            projection.screenY + projection.screenRadius < 0.0f ||
            projection.screenY - projection.screenRadius > (float)screenHeight)
            continue;
        quads.push_back({projection.worldX, projection.worldY,
                         projection.worldSize, projection.worldSize,
                         marker.r, marker.g, marker.b, marker.alpha});
    }
    // Colored sight fields are light emitters. Additive blending keeps dense
    // packs luminous instead of repeatedly darkening the layers underneath.
    DrawIconBatch(g_ConstellationCircleTex, quads, true);
}

static constexpr float kSightRearScale = 1.55f;

// Render all black sight fields in a prepass. This keeps a mob's rear field
// behind every body instead of letting it occlude a neighboring mob.
static void DrawBlackSightRear(float x, float y, float foregroundRadius,
                               float alpha = 0.7f) {
    if (foregroundRadius <= 0.0f || foregroundRadius != foregroundRadius) return;
    const GLuint rearTexture = g_InGameCircleTex ? g_InGameCircleTex
                                                  : g_ConstellationCircleTex;
    DrawDiscWithTexture(x, y, foregroundRadius * kSightRearScale,
                        0.0f, 0.0f, 0.0f, alpha, rearTexture);
}

void DrawEnemySightRear(float x, float y, float foregroundRadius,
                        float alpha) {
    DrawBlackSightRear(x, y, foregroundRadius, alpha);
}

void DrawEnemySightRearBatch(const std::vector<EnemySightRearMarker>& markers,
                             float alpha) {
    if (markers.empty()) return;

    // Rear sight fields all use the same texture and tint. Batch them into a
    // single icon draw instead of flushing once per entity.
    const GLuint rearTexture = g_InGameCircleTex ? g_InGameCircleTex
                                                  : g_ConstellationCircleTex;
    if (!(rearTexture && g_IconBatchProg)) {
        for (const auto& marker : markers) {
            if (marker.foregroundRadius <= 0.0f ||
                marker.foregroundRadius != marker.foregroundRadius) continue;
            drawCircle(marker.x, marker.y,
                       marker.foregroundRadius * kSightRearScale,
                       0.0f, 0.0f, 0.0f, alpha);
        }
        return;
    }

    static std::vector<IconBatchQuad> quads;
    quads.clear();
    quads.reserve(markers.size());
    for (const auto& marker : markers) {
        if (marker.foregroundRadius <= 0.0f ||
            marker.foregroundRadius != marker.foregroundRadius) continue;

        const WorldDiscProjection projection = ProjectWorldDisc(
            marker.x, marker.y, marker.foregroundRadius * kSightRearScale);
        // Off-screen sight fields cannot affect the current frame. Avoid
        // uploading quads for entities still approaching from outside.
        if (projection.screenX + projection.screenRadius < 0.0f ||
            projection.screenX - projection.screenRadius > (float)screenWidth ||
            projection.screenY + projection.screenRadius < 0.0f ||
            projection.screenY - projection.screenRadius > (float)screenHeight)
            continue;

        quads.push_back({projection.worldX, projection.worldY,
                         projection.worldSize, projection.worldSize,
                         0.0f, 0.0f, 0.0f, alpha});
    }
    DrawIconBatch(rearTexture, quads);
}

// Rear (black) sight field radius per kind, in units of `base`.
static float MonsterSightRearScale(MobKind kind) {
    switch (kind) {
    case MobKind::GRAVIS:  return 3.35f;
    case MobKind::QUASAR:  return 3.30f;
    // Genesis' shells, plates and orbit frame reach 2.6 base; the field must
    // keep its dark core over all of them, not only the core.
    case MobKind::GENESIS: return 3.40f;
    default:               return 2.55f;
    }
}

void DrawCombatSightFields(const MonsterManager& monsters,
                           float playerX, float playerY,
                           float playerSize, bool renderRearFields) {
    BatchFlush();
    if (renderRearFields) {
        static std::vector<EnemySightRearMarker> rearMarkers;
        rearMarkers.clear();
        rearMarkers.reserve(1u + monsters.monsters.size() + monsters.rangedMobs.size());
        rearMarkers.push_back({playerX, playerY, playerSize * 2.45f});
        for (const auto* monster : monsters.monsters) {
            if (!monster || !monster->alive || monster->sizeScale <= 0.0f) continue;
            const float base = (monster->summoned ? 28.0f : 18.0f) * monster->sizeScale;
            rearMarkers.push_back({monster->worldX, monster->worldY,
                                   base * MonsterSightRearScale(monster->kind)});
            // Genesis is line-only over a wide area: two tighter dark
            // gradients deepen the contrast out to the shell ring so it
            // reads over bright or busy windows.
            if (monster->kind == MobKind::GENESIS) {
                rearMarkers.push_back({monster->worldX, monster->worldY, base * 2.80f});
                rearMarkers.push_back({monster->worldX, monster->worldY, base * 2.20f});
            }
        }
        for (const auto* ranged : monsters.rangedMobs) {
            if (!ranged || ranged->deathScale <= 0.0f) continue;
            const float base = RangedMob::VISUAL_BASE_PX * ranged->deathScale;
            rearMarkers.push_back({ranged->worldX, ranged->worldY, base * 2.65f});
        }
        DrawEnemySightRearBatch(rearMarkers);
    }
    BatchFlush();

    BeginEnemySightFrontBatch();
    for (const auto* monster : monsters.monsters)
        QueueMonsterSightFront(monster);
    for (const auto* ranged : monsters.rangedMobs)
        QueueRangedMobSightFront(ranged);
    FlushEnemySightFrontBatch();
    BatchFlush();
}

void DrawMonsterSightRear(const Monster* m) {
    if (!m || !m->alive || m->sizeScale <= 0.0f) return;
    const float base = (m->summoned ? 28.0f : 18.0f) * m->sizeScale;
    DrawBlackSightRear(m->worldX, m->worldY, base * MonsterSightRearScale(m->kind));
    if (m->kind == MobKind::GENESIS) {
        DrawBlackSightRear(m->worldX, m->worldY, base * 2.80f);
        DrawBlackSightRear(m->worldX, m->worldY, base * 2.20f);
    }
}

void DrawRangedMobSightRear(const RangedMob* r) {
    if (!r || r->deathScale <= 0.0f) return;
    const float base = RangedMob::VISUAL_BASE_PX * r->deathScale;
    DrawBlackSightRear(r->worldX, r->worldY, base * 2.65f);
}

struct EnemyNodeAnchor {
    float x;
    float y;
    float radius;
    float brightness;
};

static void DrawEnemyNode(const EnemyNodeAnchor& node,
                          float r, float g, float b, float alpha) {
    const float nodeAlpha = alpha * node.brightness;
    // The same anchor drives the halo, star center, and every connecting edge.
    drawCircle(node.x, node.y, node.radius * 1.55f,
               r, g, b, nodeAlpha * 0.20f);
    drawCircle(node.x, node.y, node.radius * 0.72f,
               r, g, b, nodeAlpha * 0.58f);
    drawCircle(node.x, node.y, node.radius * 0.20f,
               1.0f, 1.0f, 1.0f, nodeAlpha * 0.95f);
}

static void DrawEnemyCore(float x, float y, float size,
                          float r, float g, float b, float alpha,
                          bool warning = false) {
    const float coreR = warning ? 1.0f : r;
    const float coreG = warning ? 0.18f : g;
    const float coreB = warning ? 0.08f : b;
    drawCircle(x, y, size * 2.35f, coreR, coreG, coreB, alpha * 0.12f);
    drawCircle(x, y, size * 1.38f, coreR, coreG, coreB, alpha * 0.34f);
    drawCircle(x, y, size * 0.72f, coreR, coreG, coreB, alpha * 0.82f);
    drawCircle(x, y, size * 0.28f, 1.0f, 1.0f, 1.0f, alpha);
}

static void DrawFilledHexagon(float cx, float cy, float radius,
                              float r, float g, float b, float alpha,
                              float phase = -(float)M_PI * 0.5f) {
    if (radius <= 0.0f || radius != radius) return;
    const float step = 2.0f * (float)M_PI / 6.0f;
    float px = cx + cosf(phase) * radius;
    float py = cy + sinf(phase) * radius;
    for (int i = 1; i <= 6; ++i) {
        const float angle = phase + step * (float)i;
        const float nx = cx + cosf(angle) * radius;
        const float ny = cy + sinf(angle) * radius;
        BatchTri(cx, cy, px, py, nx, ny, r, g, b, alpha);
        px = nx;
        py = ny;
    }
}

static void DrawGenesisCore(float x, float y, float size,
                            float r, float g, float b, float alpha,
                            bool warning = false) {
    const float coreR = warning ? 1.0f : r;
    const float coreG = warning ? 0.18f : g;
    const float coreB = warning ? 0.08f : b;
    drawCircle(x, y, size * 2.35f, coreR, coreG, coreB, alpha * 0.12f);
    drawCircle(x, y, size * 1.38f, coreR, coreG, coreB, alpha * 0.34f);
    // GENESIS uses a solid station core so it reads as a dock/relay hub,
    // while the surrounding sight fields remain textured and soft.
    DrawFilledHexagon(x, y, size * 0.76f,
                      coreR, coreG, coreB, alpha * 0.88f);
    DrawFilledHexagon(x, y, size * 0.30f,
                      1.0f, 1.0f, 1.0f, alpha * 0.92f);
}

static void DrawEnemyArc(float cx, float cy, float radius,
                         float start, float sweep, int segments,
                         float r, float g, float b, float alpha,
                         float width, bool dashed);

static void DrawQuasarEllipseArc(float cx, float cy,
                                 float aimX, float aimY,
                                 float majorR, float minorR,
                                 float start, float sweep, int segments,
                                 float r, float g, float b,
                                 float alpha, float width) {
    const float sideX = -aimY, sideY = aimX;
    for (int i = 0; i < segments; ++i) {
        const float a0 = start + sweep * (float)i / (float)segments;
        const float a1 = start + sweep * (float)(i + 1) / (float)segments;
        const float x0 = cx + sideX * cosf(a0) * majorR + aimX * sinf(a0) * minorR;
        const float y0 = cy + sideY * cosf(a0) * majorR + aimY * sinf(a0) * minorR;
        const float x1 = cx + sideX * cosf(a1) * majorR + aimX * sinf(a1) * minorR;
        const float y1 = cy + sideY * cosf(a1) * majorR + aimY * sinf(a1) * minorR;
        drawLineQuad(x0, y0, x1, y1, width, r, g, b, alpha);
    }
}

static void DrawEnemyArc(float cx, float cy, float radius,
                         float start, float sweep, int segments,
                         float r, float g, float b, float alpha,
                         float width, bool dashed = false) {
    if (segments < 2) return;
    for (int i = 0; i < segments; ++i) {
        if (dashed && (i & 1)) continue;
        const float a0 = start + sweep * (float)i / (float)segments;
        const float a1 = start + sweep * (float)(i + 1) / (float)segments;
        drawLineQuad(cx + cosf(a0) * radius, cy + sinf(a0) * radius,
                     cx + cosf(a1) * radius, cy + sinf(a1) * radius,
                     width, r, g, b, alpha);
    }
}

// Elite frame: two thin solid hexagon outlines counter-rotating around the
// body. Shared by every IsEliteMob() kind as each one gets its second ability.
static void DrawEliteFrame(float x, float y, float base, float phase,
                           float r, float g, float b, float alpha) {
    DrawEnemyArc(x, y, base * 1.44f, -phase * 0.6f, 6.2831853f, 6,
                 r, g, b, alpha, 1.0f, false);
    DrawEnemyArc(x, y, base * 1.30f, phase * 0.9f + 0.5235988f, 6.2831853f, 6,
                 r, g, b, alpha * 0.7f, 0.8f, false);
}

// Landing-blast telegraph: a thin red ring at the damage radius over a
// faint disc. `timer` (0..1) fills an inner disc toward the ring, so a Swarm
// in flight shows how soon it lands; pass a negative timer to omit it.
static void DrawThrownBlastZone(float x, float y, float alpha, float timer,
                                float r=Monster::THROWN_SWARM_BLAST_RADIUS) {
    if (alpha <= 0.0f) return;
    drawCircle(x, y, r, 1.0f, 0.24f, 0.20f, 0.07f * alpha);
    if (timer >= 0.0f)
        drawCircle(x, y, r * std::min(1.0f, timer), 1.0f, 0.24f, 0.20f,
                   (0.08f + 0.14f * timer) * alpha);
    DrawEnemyArc(x, y, r, 0.0f, 6.2831853f, 48, 1.0f, 0.30f, 0.24f,
                 0.36f * alpha, 1.2f, false);
    drawCircle(x, y, 2.5f, 1.0f, 0.40f, 0.34f, 0.80f * alpha);
}

// Genesis (shell core): a small core inside three broad shells, one stack
// each. A shell is a faceted cradle holding a seated Swarm that points out
// of the ring, so it reads as ammunition rather than a shield.
//   charge  - loose empty shells; a charging shell is fed light pulses down
//             its spoke, slides in and snaps shut with a seam sweep. The ring
//             slows with each stack and stops gap-aligned at 3/3.
//   aim     - the ring settles with a small wobble, the shells clamp in and
//             the cut ends beside the muzzle gap heat up.
//   throw   - each Swarm rides its spoke into the core, the core flashes and
//             the Swarm leaves through the gap; only that shell kicks out.
//   regroup - emptied shells cool in the order they were emptied.
static void DrawGenesisLauncher(const Monster* m, float base, float t) {
    float cr = m->color.r, cg = m->color.g, cb = m->color.b;
    ApplyMobStyleTint(cr, cg, cb);
    ApplyMobHitFlash(cr, cg, cb, m->hitFlashTimer);
    const float pR = 1.0f, pG = 0.24f, pB = 0.20f;   // production red
    const float hR = 1.0f, hG = 0.62f, hB = 0.52f;   // heated edge
    const float liteR = cr + (1.0f - cr) * 0.35f;
    const float liteG = cg + (1.0f - cg) * 0.35f;
    const float liteB = cb + (1.0f - cb) * 0.35f;
    const bool rotorMode=m->GenesisRotorMode();
    const float pose=m->GenesisRotorPose();
    const glm::vec2 center=rotorMode ? m->GenesisRotorCenter() : glm::vec2(m->worldX,m->worldY);
    const float x=center.x,y=center.y;
    const float R = base * 2.05f;
    // Main contours thicken a little with size so the tier-4 body keeps weight.
    const float lw = std::clamp(base / 34.2f, 1.0f, 1.4f);
    const int phase = m->hivePhase;
    const bool aimLocked = phase == 1 || phase == 2;
    const float aim = aimLocked ? m->genesisAimAngle : m->hiveOrbitAngle;
    float ring = m->hiveOrbitAngle + (rotorMode ? 0.0f : m->genesisShellSpin);
    if (phase == 1) {                                          // settle wobble
        const float u = m->spawnTimer;
        ring += 0.08f * sinf(u * 26.0f) * expf(-u * 9.0f);
    }
    const float fill = rotorMode ? 0.0f : m->GenesisStackFill();
    const float charge = fill / (float)Monster::GENESIS_STACKS;
    const float recoil = m->genesisRecoil * m->genesisRecoil;
    const float clamp01 = phase == 1 ? m->hiveOpenFactor : (phase == 2 ? 1.0f : 0.0f);
    const float clampIn = clamp01 * clamp01 * (3.0f - 2.0f * clamp01);
    const float gapHeat = phase == 1 ? m->hiveOpenFactor : (phase == 2 ? 1.0f : 0.0f);

    // Aim lines and landing zones for the Swarms still seated.
    if (aimLocked) {
        const float muzX = x + cosf(aim) * R * 0.95f, muzY = y + sinf(aim) * R * 0.95f;
        const float lock = phase == 2 || m->hiveOpenFactor >= 0.999f ? 1.0f : 0.0f;
        const float a = phase == 1 ? m->hiveOpenFactor : 1.0f;
        for (int i = phase == 2 ? m->hiveSpawnCount : 0; i < Monster::GENESIS_STACKS; ++i) {
            const float ex = m->genesisShotX[i], ey = m->genesisShotY[i];
            const float len = std::sqrt((ex - muzX) * (ex - muzX) + (ey - muzY) * (ey - muzY));
            if (len < 1.0f) continue;
            drawLineQuad(muzX, muzY, ex, ey, 0.9f + 0.4f * a + 0.2f * lock,
                         pR, pG, pB, (0.12f + 0.30f * a + 0.11f * lock) * a);
            DrawThrownBlastZone(ex, ey, a, -1.0f);
        }
    }

    // Shells. kSlot is the charge order (rear shell 1 first); throws go in
    // reverse slot order, so the pair flanking the muzzle leaves first.
    static constexpr int kSlot[3] = { 1, 0, 2 };
    const float feedU = (phase == 2 && m->hiveSpawnCount < Monster::GENESIS_STACKS)
        ? std::clamp((m->spawnTimer - (Monster::GENESIS_THROW_GAP - Monster::GENESIS_FEED_TIME)) /
                     Monster::GENESIS_FEED_TIME, 0.0f, 1.0f)
        : 0.0f;
    for (int k = 0; k < 3; ++k) {
        const float mid = ring + 1.0471976f + 2.0943951f * (float)k;
        const bool front=k!=1; // k=1 is the single dedicated rear production cradle
        const float half=0.9076f-(front ? 0.40f*pose : 0.0f);
        const float ro=R*(1.0f+(front ? 0.22f*pose : 0.0f));
        const float ri=R*(0.74f+(front ? 0.16f*pose : 0.0f));
        const int slot = kSlot[k];
        const int shot = Monster::GENESIS_STACKS - 1 - slot;
        float f = std::clamp(fill - (float)slot, 0.0f, 1.0f);
        const bool feeding = phase == 2 && shot == m->hiveSpawnCount;
        if (feeding) f = 1.0f;
        const float age = m->GenesisThrowAge(shot);

        // Radial offset (units of R) and seam sweep for this shell.
        float push = 0.0f, sweep = -1.0f, heat = 0.0f;
        if (age >= 0.0f) {
            // Thrown: kick out, settle to the loose rest, cool down.
            const float in = std::min(1.0f, age / 0.05f);
            push = -0.07f + 0.17f * in + 0.12f * in * expf(-age * 5.0f);
            heat = expf(-age * 2.5f);
        } else if (phase == 1 || phase == 2) {
            push = -0.07f * clampIn;
        } else if (phase == 0) {
            const float lockAt = Monster::GENESIS_STACK_TIME * (float)(slot + 1);
            const float lockAge = m->spawnTimer - lockAt;
            if (lockAge < 0.0f) {
                const float s = f * f * (3.0f - 2.0f * f);
                push = 0.10f * (1.0f - s);
            } else if (lockAge < 0.18f) {
                const float u = lockAge / 0.18f;
                push = -0.05f * sinf(3.14159265f * u);
            }
            if (lockAge >= 0.0f && lockAge < 0.25f) sweep = lockAge / 0.25f;
        } else {
            push = 0.10f;
        }
        // The last shell locks on the phase change, so its snap plays in aim.
        if (phase == 1 && slot == Monster::GENESIS_STACKS - 1) {
            const float a = m->spawnTimer;
            if (a < 0.18f) push += -0.05f * sinf(3.14159265f * a / 0.18f);
            if (a < 0.25f) sweep = a / 0.25f;
        }
        if(rotorMode) push=front ? 0.10f*pose-0.08f*m->GenesisRotorKick() :
                                    0.14f*m->GenesisRotorWindup();
        if(m->GenesisRotorWaiting()) {
            const float lastAge=Monster::GENESIS_THROW_END+Monster::GENESIS_RECOVER_TIME+
                               (Monster::GENESIS_STACKS-1-shot)*Monster::GENESIS_THROW_GAP;
            push=(0.10f+0.12f*expf(-lastAge*5.0f))*
                 (1.0f-Monster::GenesisSmooth(m->spawnTimer/0.25f));
        }
        push *= R;
        const float sx = x + cosf(mid) * push, sy = y + sinf(mid) * push;

        const float lit = rotorMode ? (front ? 0.45f*pose : 0.7f*pose) :
                                     std::max(f * f * (3.0f - 2.0f * f), heat);
        // Empty shells stay readable over bright windows: they start a third
        // of the way to the lit tone and well above half alpha.
        const float tone = 0.35f + 0.65f * lit;
        float sr = cr + (liteR - cr) * tone, sg = cg + (liteG - cg) * tone,
              sb = cb + (liteB - cb) * tone;
        sr += (pR - sr) * heat * 0.6f; sg += (pG - sg) * heat * 0.6f; sb += (pB - sb) * heat * 0.6f;
        const float oa = 0.58f + 0.38f * lit, ow = (1.2f + 0.5f * lit) * lw;
        // Faceted outer edge (4 facets), faceted inner edge (3 facets) and
        // flat cut ends make one closed cradle outline.
        float px[9], py[9];
        for (int i = 0; i <= 4; ++i) {
            const float a = mid - half + half * 0.5f * (float)i;
            px[i] = sx + cosf(a) * ro;  py[i] = sy + sinf(a) * ro;
        }
        const float ih = half - 0.1047f;                // ends taper 6 deg
        for (int i = 0; i <= 3; ++i) {
            const float a = mid + ih - ih * (2.0f / 3.0f) * (float)i;
            px[5 + i] = sx + cosf(a) * ri;  py[5 + i] = sy + sinf(a) * ri;
        }
        for (int i = 0; i < 9; ++i) {
            const int n = (i + 1) % 9;
            drawLineQuad(px[i], py[i], px[n], py[n], ow, sr, sg, sb, oa);
        }
        // Scale plate (B): a narrower second plate rides just outside the
        // shell, staggered toward the spin, and clips onto the rim with two
        // tabs so the layers never cross. Liner plate: fixed to the core,
        // between the core ring and the shell.
        {
            const float pm = mid + 0.2443f;               // +14 deg stagger
            const float ph = 0.5585f;                     // 32 deg
            const float po = ro * 1.19f, pi = ro * 1.03f;
            float qx[7], qy[7];
            for (int i = 0; i <= 3; ++i) {
                const float a = pm - ph * 0.8f + ph * 0.6f * (float)i;
                qx[i] = sx + cosf(a) * po;  qy[i] = sy + sinf(a) * po;
            }
            for (int i = 0; i <= 2; ++i) {
                const float a = pm + ph - ph * (float)i;
                qx[4 + i] = sx + cosf(a) * pi;  qy[4 + i] = sy + sinf(a) * pi;
            }
            const float pa = (0.46f + 0.44f * lit), pw = (1.05f + 0.45f * lit) * lw;
            for (int i = 0; i < 7; ++i) {
                const int n = (i + 1) % 7;
                drawLineQuad(qx[i], qy[i], qx[n], qy[n], pw, sr, sg, sb, pa);
            }
            const float ps = po - R * 0.04f;
            for (int i = 0; i < 3; ++i) {
                const float a0 = pm - ph * 0.62f + ph * 0.48f * (float)i;
                const float a1 = a0 + ph * 0.48f;
                drawLineQuad(sx + cosf(a0) * ps, sy + sinf(a0) * ps,
                             sx + cosf(a1) * ps, sy + sinf(a1) * ps,
                             0.85f, sr, sg, sb, 0.22f + 0.20f * lit);
            }
            for (int e = -1; e <= 1; e += 2) {
                const float a = pm + ph * 0.55f * (float)e;
                drawLineQuad(sx + cosf(a) * pi, sy + sinf(a) * pi,
                             sx + cosf(a) * (ro + R * 0.005f), sy + sinf(a) * (ro + R * 0.005f),
                             1.2f, sr, sg, sb, pa);
                DrawEnemyNode({sx + cosf(pm + ph * 0.78f * (float)e) * (po + pi) * 0.5f,
                               sy + sinf(pm + ph * 0.78f * (float)e) * (po + pi) * 0.5f,
                               base * 0.045f, 0.6f + 0.4f * lit}, sr, sg, sb, 0.45f + 0.45f * lit);
            }
            const float lh = 0.6632f;                     // 38 deg
            float lx[13], ly[13];
            for (int i = 0; i <= 6; ++i) {
                const float a = mid - lh + lh * (2.0f / 6.0f) * (float)i;
                lx[i] = x + cosf(a) * R * 0.66f;  ly[i] = y + sinf(a) * R * 0.66f;
            }
            for (int i = 0; i <= 5; ++i) {
                const float a = mid + lh * 0.8f - lh * 1.6f / 5.0f * (float)i;
                lx[7 + i] = x + cosf(a) * R * 0.52f;  ly[7 + i] = y + sinf(a) * R * 0.52f;
            }
            for (int i = 0; i < 13; ++i) {
                const int n = (i + 1) % 13;
                drawLineQuad(lx[i], ly[i], lx[n], ly[n], 0.95f, cr, cg, cb, 0.34f + 0.26f * lit);
            }
        }
        // Cut ends beside the muzzle gap heat while aiming and throwing:
        // shell 0's leading end and shell 2's trailing end.
        if (gapHeat > 0.0f && (k == 0 || k == 2)) {
            const int e0 = k == 0 ? 0 : 4, e1 = k == 0 ? 8 : 5;
            drawLineQuad(px[e0], py[e0], px[e1], py[e1], 1.6f + 1.2f * gapHeat,
                         hR, hG, hB, 0.35f + 0.60f * gapHeat);
        }
        // Inset contour and two radial seams framing the cradle.
        const float rin = ro - R * 0.06f;
        const float span = half * 2.0f - 0.14f;
        for (int i = 0; i < 4; ++i) {
            const float a0 = mid - half + 0.07f + span * 0.25f * (float)i;
            const float a1 = a0 + span * 0.25f;
            drawLineQuad(sx + cosf(a0) * rin, sy + sinf(a0) * rin,
                         sx + cosf(a1) * rin, sy + sinf(a1) * rin,
                         0.85f, sr, sg, sb, 0.24f + 0.24f * lit);
        }
        for (int e = -1; e <= 1; e += 2) {
            const float a = mid + half * 0.42f * (float)e;
            drawLineQuad(sx + cosf(a) * (ri + R * 0.02f), sy + sinf(a) * (ri + R * 0.02f),
                         sx + cosf(a) * (rin - R * 0.01f), sy + sinf(a) * (rin - R * 0.01f),
                         0.85f, sr, sg, sb, 0.22f + 0.20f * lit);
        }
        // Lock-in seam sweep: a short white arc runs along the inset contour.
        if (sweep >= 0.0f) {
            const float c0 = mid - half + 0.07f + span * sweep;
            const float sa = 1.0f - sweep;
            for (int i = 0; i < 3; ++i) {
                const float a0 = c0 - 0.16f + 0.107f * (float)i, a1 = a0 + 0.107f;
                drawLineQuad(sx + cosf(a0) * rin, sy + sinf(a0) * rin,
                             sx + cosf(a1) * rin, sy + sinf(a1) * rin,
                             2.0f, 1.0f, 1.0f, 1.0f, 0.85f * sa);
            }
        }
        // Spoke from the core ring to the cradle; it becomes a hot rail while
        // its Swarm is fed into the core.
        const float s0x = x + cosf(mid) * R * 0.40f, s0y = y + sinf(mid) * R * 0.40f;
        const float s1x = sx + cosf(mid) * ri, s1y = sy + sinf(mid) * ri;
        if (feeding)
            drawLineQuad(s0x, s0y, s1x, s1y, 2.0f, hR, hG, hB, 0.55f + 0.40f * feedU);
        else
            drawLineQuad(s0x, s0y, s1x, s1y, 1.0f, sr, sg, sb, 0.30f + 0.32f * lit);
        // Supply line: the spoke of the shell now charging fills solid from
        // the core outward with the charge, led by a bright breathing head.
        if (phase == 0 && f > 0.0f && f < 1.0f) {
            const float hx = s0x + (s1x - s0x) * f, hy = s0y + (s1y - s0y) * f;
            const float breath = 0.5f + 0.5f * sinf(t * 9.0f);
            drawLineQuad(s0x, s0y, hx, hy, 1.8f, hR, hG, hB, 0.70f);
            drawCircle(hx, hy, base * (0.05f + 0.02f * breath), 1.0f, 0.85f, 0.80f,
                       0.70f + 0.25f * breath);
        }
        // Seated Swarm, pointing out of the ring; while fed it rides the
        // spoke into the core, turning to face the muzzle.
        if (f > 0.0f && age < 0.0f) {
            float cxs = sx + cosf(mid) * (ro + ri) * 0.5f;
            float cys = sy + sinf(mid) * (ro + ri) * 0.5f;
            float dir = mid, tr = R * 0.10f * (0.6f + 0.4f * lit);
            if (feeding) {
                const float u = feedU * feedU;
                cxs += (x - cxs) * u;  cys += (y - cys) * u;
                float turn = aim - mid;
                while (turn > 3.14159265f) turn -= 6.28318531f;
                while (turn < -3.14159265f) turn += 6.28318531f;
                dir = mid + turn * feedU;
                tr *= 1.0f + 0.3f * feedU;
            }
            const float flick = f >= 1.0f ? 0.88f + 0.12f * sinf(t * 21.0f + 2.1f * (float)k) : 1.0f;
            float tx[3], ty[3];
            for (int v = 0; v < 3; ++v) {
                const float a = dir + 2.0943951f * (float)v;
                tx[v] = cxs + cosf(a) * tr;  ty[v] = cys + sinf(a) * tr;
            }
            for (int v = 0; v < 3; ++v) {
                const int n = (v + 1) % 3;
                drawLineQuad(tx[v], ty[v], tx[n], ty[n], 1.3f, pR, pG, pB, 0.95f * lit * flick);
            }
            if (f >= 1.0f)
                drawCircle(cxs, cys, base * 0.055f, 1.0f, 1.0f, 1.0f, 0.85f * flick);
        }
    }

    // Orbit frame: thin arcs behind the shells, capped by a star-node pair
    // at every gap.
    for (int k = 0; k < (rotorMode ? 0 : 3); ++k) {
        const float mid = ring + 1.0471976f + 2.0943951f * (float)k;
        DrawEnemyArc(x, y, R * 1.27f, mid - 0.9425f, 1.885f, 18,
                     cr, cg, cb, 0.40f + 0.14f * charge, 1.0f * lw);
        const float ga = ring + 2.0943951f * (float)k;
        for (int e = -1; e <= 1; e += 2)
            DrawEnemyNode({x + cosf(ga + 0.1047f * (float)e) * R * 1.27f,
                           y + sinf(ga + 0.1047f * (float)e) * R * 1.27f,
                           base * 0.05f, 0.75f}, liteR, liteG, liteB, 0.70f);
    }

    // Core, sliding back from the aim and flashing on each throw.
    const float ccx = x - cosf(aim) * R * 0.08f * recoil;
    const float ccy = y - sinf(aim) * R * 0.08f * recoil;
    const float hr = cr + (pR - cr) * charge, hg = cg + (pG - cg) * charge,
                hb = cb + (pB - cb) * charge;
    drawCircle(ccx, ccy, R * (0.34f + 0.06f * recoil), pR, pG, pB,
               0.05f + 0.16f * charge + 0.30f * recoil);
    DrawEnemyArc(ccx, ccy, R * 0.40f, 0.0f, 6.2831853f, 28,
                 liteR + (1.0f - liteR) * recoil, liteG + (1.0f - liteG) * recoil,
                 liteB + (1.0f - liteB) * recoil, 0.90f, (1.5f + 0.8f * recoil) * lw);
    DrawEnemyArc(ccx, ccy, R * 0.32f, 0.0f, 6.2831853f, 24, hr, hg, hb, 0.40f + 0.30f * charge, 0.85f);
    if(rotorMode) {
        DrawGenesisCore(x,y,base*0.34f,0.95f,0.36f,0.30f,0.96f,false);
        const glm::vec2 axis(cosf(ring),sinf(ring)), side(-axis.y,axis.x);
        // Two short rails flank ONE central exit, no long beam or landing rings.
        for(float sign:{-1.0f,1.0f}) {
            const auto a=center+axis*(R*0.35f)+side*(R*0.48f*sign);
            const auto b=center+axis*(R*1.05f)+side*(R*0.48f*sign);
            drawLineQuad(a.x,a.y,b.x,b.y,1.2f,0.95f,0.30f,0.24f,0.8f*pose);
        }
        // The real forming Rotor is drawn in the entity pass. Do not draw a
        // separate full-size preview that gets replaced at the rear shell.
        return;
    }
    float Tx[3], Ty[3], tX[3], tY[3];
    for (int v = 0; v < 3; ++v) {
        const float a = ring + 2.0943951f * (float)v;   // vertices point at the gaps
        Tx[v] = ccx + cosf(a) * R * 0.24f;  Ty[v] = ccy + sinf(a) * R * 0.24f;
        tX[v] = ccx - cosf(a) * R * 0.12f;  tY[v] = ccy - sinf(a) * R * 0.12f;
    }
    for (int v = 0; v < 3; ++v) {
        const int n = (v + 1) % 3;
        drawLineQuad(Tx[v], Ty[v], Tx[n], Ty[n], 1.4f,
                     hr + (1.0f - hr) * 0.3f, hg + (1.0f - hg) * 0.3f, hb + (1.0f - hb) * 0.3f, 0.92f);
        drawLineQuad(tX[v], tY[v], tX[n], tY[n], 1.1f,
                     0.92f, 0.80f, 0.80f, 0.55f + 0.40f * charge);
    }
    DrawEnemyNode({Tx[0], Ty[0], base * 0.09f, 0.80f + 0.20f * charge}, 1.0f, 1.0f, 1.0f, 0.92f);
    drawCircle(ccx, ccy, base * (0.09f + 0.06f * recoil), 1.0f, 1.0f, 1.0f, 0.95f);
}

// Thrown Swarm afterimages, Antares style: older samples fade out, and the
// flight ghosts split into red/cyan channels like a top-speed Antares.
static void DrawThrownSwarmGhosts(const Monster* m, float base,
                                  float cr, float cg, float cb) {
    static constexpr float GHOST_ALPHA[Monster::ANTARES_TRAIL_COUNT] = {
        0.34f, 0.27f, 0.21f, 0.16f, 0.12f, 0.08f };
    const bool flight = m->thrownPhase == 1;
    const float radius = base * 0.92f * (flight ? 1.15f : 1.0f);
    for (int i = Monster::ANTARES_TRAIL_COUNT - 1; i >= 0; --i) {
        const glm::vec3& s = m->antaresTrail[i];
        // s.z is the body angle at that sample; the channel split stays
        // across the flight line.
        const float hx = cosf(m->thrownHeading), hy = sinf(m->thrownHeading);
        auto ghost = [&](float ox, float oy, float r, float g, float b, float a) {
            float vx[3], vy[3];
            for (int k = 0; k < 3; ++k) {
                const float ang = s.z + 2.0943951f * (float)k;
                vx[k] = s.x + ox + cosf(ang) * radius;
                vy[k] = s.y + oy + sinf(ang) * radius;
            }
            for (int k = 0; k < 3; ++k) {
                const int n = (k + 1) % 3;
                drawLineQuad(vx[k], vy[k], vx[n], vy[n], 1.2f, r, g, b, a);
            }
        };
        if (flight) {
            const float ox = -hy * 1.5f, oy = hx * 1.5f;
            ghost(ox, oy, 1.0f, 0.22f, 0.28f, GHOST_ALPHA[i] * 0.70f);
            ghost(-ox, -oy, 0.20f, 0.92f, 1.0f, GHOST_ALPHA[i] * 0.70f);
        } else {
            ghost(0.0f, 0.0f, cr, cg, cb, GHOST_ALPHA[i]);
        }
    }
}

// Regulus shield palette (ice blue-white).
static constexpr float kAegisR = 0.722f, kAegisG = 0.937f, kAegisB = 1.0f;
// Regulus plates and granted brackets turn together on one shared beat.
static constexpr float kAegisSpin = 0.9f;   // rad/s, about 7s per turn

// Shield granted by Regulus to another enemy: four corner brackets sized to
// the target, dimming as the shield wears down and flashing when struck.
// Brackets frame the ally instead of covering it with a second silhouette.
static void DrawAegisMark(float x, float y, float half, float ratio, float flash,
                          float angle, float grant) {
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    // Snap-in: a fresh shield appears wider and contracts onto the target.
    const float g01 = std::clamp(grant / SHIELD_GRANT_TIME, 0.0f, 1.0f);
    half *= 1.0f + 0.6f * g01 * g01;
    flash = std::max(flash, g01);
    const float a = (0.35f + 0.55f * ratio) + 0.35f * flash;
    const float r = kAegisR + (1.0f - kAegisR) * flash;
    const float g = kAegisG + (1.0f - kAegisG) * flash;
    const float b = kAegisB;
    const float ca = cosf(angle), sa = sinf(angle);
    const auto rot = [&](float lx, float ly, float& ox, float& oy) {
        ox = x + lx * ca - ly * sa;
        oy = y + lx * sa + ly * ca;
    };
    const float arm = half * 0.42f;
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sy = -1; sy <= 1; sy += 2) {
            float cx, cy, hx, hy, vx, vy;
            rot(half * (float)sx, half * (float)sy, cx, cy);
            rot((half - arm) * (float)sx, half * (float)sy, hx, hy);
            rot(half * (float)sx, (half - arm) * (float)sy, vx, vy);
            drawLineQuad(cx, cy, hx, hy, 2.0f, r, g, b, a);
            drawLineQuad(cx, cy, vx, vy, 2.0f, r, g, b, a);
        }
    }
}

// Half-size of the corner-bracket frame per enemy kind, in units of `base`,
// so the brackets sit just outside each silhouette.
static float AegisMarkExtent(MobKind kind) {
    // Tuned from shielded smoke captures (SHIELD step) so no node, plate or
    // pole cell touches the brackets at any rotation.
    switch (kind) {
    case MobKind::GENESIS:  return 2.75f;   // orbit frame + scale plates
    case MobKind::QUASAR:   return 2.40f;   // tilted outer orbit
    case MobKind::GRAVIS:   return 1.55f;
    case MobKind::ANTARES:  return 1.85f;   // pentagon vertices and afterimages
    case MobKind::MAGNETAR: return 2.35f;   // pole cells
    case MobKind::GIMBAL:   return 1.60f;
    case MobKind::SWARM:    return 1.30f;
    default:                return 1.55f;   // Rotor's rotating square corners
    }
}

// Regulus' own shield: four curved plates (not a hexagon). Solid and as
// bright as the remaining shield while it holds; dim and thin once broken,
// then thickening and brightening back over the regen delay.
static void DrawRegulusPlates(float x, float y, float base, float phase,
                              float ratio, float regen, float flash) {
    const float outerR = base * 1.75f, innerR = base * 1.62f;
    const bool broken = ratio <= 0.0f;
    for (int k = 0; k < 4; ++k) {
        const float a0 = phase + 1.5707963f * (float)k + 0.22f;
        const float sweep = 1.5707963f - 0.44f;
        if (broken) {
            DrawEnemyArc(x, y, outerR, a0, sweep, 12, kAegisR, kAegisG, kAegisB,
                         0.16f + 0.51f * regen, 0.8f + 1.6f * regen, false);
            continue;
        }
        const float a = std::min(1.0f, 0.45f + 0.50f * ratio + 0.35f * flash);
        DrawEnemyArc(x, y, outerR, a0, sweep, 12, kAegisR, kAegisG, kAegisB, a, 3.0f, false);
        DrawEnemyArc(x, y, innerR, a0 + 0.08f, sweep - 0.16f, 10,
                     0.92f, 0.99f, 1.0f, a * 0.5f, 1.0f, false);
    }
}

// Regulus body: a shield crest of four kite plates with flat (cut) tips
// around a square heart. Flat tips keep the cross from reading as spikes.
static void DrawRegulusCrest(float x, float y, float base, float phase,
                             float r, float g, float b, float visualTime) {
    const float tipR = base * 1.15f;
    const float pulse = 0.5f + 0.5f * sinf(visualTime * 3.0f);
    for (int k = 0; k < 4; ++k) {
        const float a = phase + 1.5707963f * (float)k;
        const float px[5] = {
            x + cosf(a) * tipR * 0.30f,
            x + cosf(a - 0.42f) * tipR * 0.56f,
            x + cosf(a - 0.13f) * tipR * 0.95f,
            x + cosf(a + 0.13f) * tipR * 0.95f,
            x + cosf(a + 0.42f) * tipR * 0.56f };
        const float py[5] = {
            y + sinf(a) * tipR * 0.30f,
            y + sinf(a - 0.42f) * tipR * 0.56f,
            y + sinf(a - 0.13f) * tipR * 0.95f,
            y + sinf(a + 0.13f) * tipR * 0.95f,
            y + sinf(a + 0.42f) * tipR * 0.56f };
        for (int i = 0; i < 5; ++i) {
            const int n = (i + 1) % 5;
            drawLineQuad(px[i], py[i], px[n], py[n], 1.6f, r, g, b, 0.92f);
        }
        // Spine down the plate, stopping short of the flat tip.
        drawLineQuad(px[0], py[0], x + cosf(a) * tipR * 0.80f, y + sinf(a) * tipR * 0.80f,
                     0.9f, r, g, b, 0.45f);
    }
    float sx[4], sy[4];
    for (int k = 0; k < 4; ++k) {
        const float a = phase + 0.7853982f + 1.5707963f * (float)k;
        sx[k] = x + cosf(a) * base * 0.30f;
        sy[k] = y + sinf(a) * base * 0.30f;
    }
    for (int k = 0; k < 4; ++k) {
        const int n = (k + 1) & 3;
        drawLineQuad(sx[k], sy[k], sx[n], sy[n], 1.3f, 0.92f, 0.99f, 1.0f, 0.90f);
    }
    DrawEnemyCore(x, y, base * 0.30f, r, g, b, 0.90f + 0.10f * pulse, false);
}

// Instant shield laser: a straight line from the Regulus to the target that
// flashes and fades over REGULUS_BEAM_TIME. No projectile is drawn.
void DrawRegulusBeams(const MonsterManager& manager, float /*visualTime*/) {
    for (const auto& beam : manager.regulusBeams) {
        const float t = std::clamp(beam.age / Monster::REGULUS_BEAM_TIME, 0.0f, 1.0f);
        const float fade = (1.0f - t) * (1.0f - t);
        drawLineQuad(beam.x0, beam.y0, beam.x1, beam.y1, 7.0f * (1.0f - 0.5f * t),
                     kAegisR, kAegisG, kAegisB, 0.22f * fade);
        drawLineQuad(beam.x0, beam.y0, beam.x1, beam.y1, 1.8f,
                     0.92f, 0.99f, 1.0f, 0.95f * fade);
        drawCircle(beam.x1, beam.y1, 10.0f * (1.0f + t), kAegisR, kAegisG, kAegisB,
                   0.30f * fade);
    }
}

// Antares' pentagon frame is split at every vertex. Ghosts reuse this recipe
// without the core, so the afterimage never blurs the hit-relevant center.
struct AntaresFrameStyle {
    float r, g, b, alpha;
    float pushX = 0.0f, pushY = 0.0f; // segments trail behind while accelerating
    bool nodes = true;
};

static void DrawAntaresFrame(float x, float y, float base, float angle,
                             const AntaresFrameStyle& style) {
    static constexpr int VERTICES = 5;
    static constexpr float STEP = 1.25663706f;
    static constexpr float GAP = 0.14f;
    const float radius = base * 1.40f;
    float vx[VERTICES], vy[VERTICES];
    for (int k = 0; k < VERTICES; ++k) {
        vx[k] = x + cosf(angle + (float)k * STEP) * radius;
        vy[k] = y + sinf(angle + (float)k * STEP) * radius;
    }
    for (int k = 0; k < VERTICES; ++k) {
        const int n = (k + 1) % VERTICES;
        const float ax = vx[k] + (vx[n] - vx[k]) * GAP + style.pushX;
        const float ay = vy[k] + (vy[n] - vy[k]) * GAP + style.pushY;
        const float bx = vx[n] + (vx[k] - vx[n]) * GAP + style.pushX;
        const float by = vy[n] + (vy[k] - vy[n]) * GAP + style.pushY;
        drawLineQuad(ax, ay, bx, by, 2.6f,
                     style.r, style.g, style.b, style.alpha * 0.12f);
        drawLineQuad(ax, ay, bx, by, 1.3f,
                     style.r, style.g, style.b, style.alpha * 0.85f);
    }
    if (!style.nodes) return;
    for (int k = 0; k < VERTICES; ++k)
        DrawEnemyNode({vx[k], vy[k], base * 0.16f, 0.90f},
                      style.r, style.g, style.b, style.alpha);
}

static void DrawAntares(const Monster* m, float base) {
    using State = Monster::AntaresState;
    float cr = m->color.r, cg = m->color.g, cb = m->color.b;
    ApplyMobStyleTint(cr, cg, cb);
    const float ghostR = cr, ghostG = cg, ghostB = cb;
    ApplyMobHitFlash(cr, cg, cb, m->hitFlashTimer);
    const float x = m->worldX, y = m->worldY;
    const bool skidding = m->antaresState == State::SKID;
    const bool topSpeed = m->AntaresStage() == 4 && !skidding;

    float headX = m->antaresHeadingX, headY = m->antaresHeadingY;
    if (headX == 0.0f && headY == 0.0f) {
        headX = cosf(m->antaresVisualAngle);
        headY = sinf(m->antaresVisualAngle);
    }
    // Segments lag up to 4px behind as speed builds.
    const float pushPx = 4.0f * m->AntaresSpeedRatio();

    // Afterimages: older samples fade out; only top-speed ghosts split into
    // red/cyan channels.
    static constexpr float GHOST_ALPHA[Monster::ANTARES_TRAIL_COUNT] = {
        0.34f, 0.27f, 0.21f, 0.16f, 0.12f, 0.08f };
    const int ghosts = std::min(m->AntaresGhostCount(),
                                Monster::ANTARES_TRAIL_COUNT);
    for (int i = ghosts - 1; i >= 0; --i) {
        const glm::vec3& sample = m->antaresTrail[i];
        AntaresFrameStyle ghost{ ghostR, ghostG, ghostB, GHOST_ALPHA[i] };
        ghost.nodes = false;
        if (topSpeed) {
            const float ox = -headY * 1.5f, oy = headX * 1.5f;
            ghost.alpha *= 0.70f;
            ghost.r = 1.0f; ghost.g = 0.22f; ghost.b = 0.28f;
            DrawAntaresFrame(sample.x + ox, sample.y + oy, base, sample.z, ghost);
            ghost.r = 0.20f; ghost.g = 0.92f; ghost.b = 1.0f;
            DrawAntaresFrame(sample.x - ox, sample.y - oy, base, sample.z, ghost);
        } else {
            DrawAntaresFrame(sample.x, sample.y, base, sample.z, ghost);
        }
    }

    // A ram stun rattles the frame (not the core) and settles over the stun.
    float shakeX = 0.0f, shakeY = 0.0f;
    if (m->antaresState == State::STUNNED) {
        const float left = 1.0f - std::min(1.0f,
            m->antaresStateTimer / Monster::ANTARES_STUN_TIME);
        const float amp = 3.5f * left;
        shakeX = sinf(m->antaresStateTimer * 71.0f) * amp;
        shakeY = cosf(m->antaresStateTimer * 53.0f) * amp;
    }
    AntaresFrameStyle body{ cr, cg, cb, 0.94f };
    body.pushX = -headX * pushPx + shakeX;
    body.pushY = -headY * pushPx + shakeY;
    DrawAntaresFrame(x, y, base, m->antaresVisualAngle, body);

    // The core dims through the skid and recovers over the rest; a stun
    // flickers it.
    float coreAlpha = 0.96f;
    if (skidding)
        coreAlpha = 0.96f - 0.38f * std::min(1.0f, m->antaresStateTimer / 0.45f);
    else if (m->antaresState == State::REST)
        coreAlpha = 0.58f + 0.38f * std::min(1.0f,
            m->antaresStateTimer / Monster::ANTARES_REST_TIME);
    else if (m->antaresState == State::STUNNED)
        coreAlpha = 0.62f + 0.30f * (0.5f + 0.5f * sinf(m->antaresStateTimer * 40.0f));
    DrawEnemyCore(x, y, base * 0.26f, cr, cg, cb, coreAlpha, topSpeed);
}

// Magnetar (lattice ward). The shield IS the body: a slowly turning
// icosahedron lattice at the real interception radius, back edges faint and
// front edges gold. Inside, a small crystal seed (octahedron) turns the other
// way and holds the lattice by four tethers. Each of the 20 faces is its own
// shield (Monster::MAGNETAR_FACE_HITS shots from its side); only broken faces
// change, the rest of the lattice stays gold.
//   hit       the face that stopped the shot flashes white
//   break     that face's edges flash white, flicker gold/orange and settle
//             orange; its vertices buckle inward one after another; the seed
//             flashes
//   broken    the orange face keeps crumpling: its vertices buckle in and
//             spring back on their own beats; the seed tints orange with the
//             share of broken faces; tethers to its vertices are cut
//   restore   MAGNETAR_FACE_RESTORE later the face snaps back to gold with a
//             white flash and a small overshoot
static void DrawMagnetar(const Monster* m, float base, float visualTime) {
    namespace ML = MagnetarLattice;
    const float x = m->worldX, y = m->worldY;
    float cr = m->color.r, cg = m->color.g, cb = m->color.b;
    ApplyMobStyleTint(cr, cg, cb);
    ApplyMobHitFlash(cr, cg, cb, m->hitFlashTimer);
    const float liteR = cr + (1.0f - cr) * 0.5f;
    const float liteG = cg + (1.0f - cg) * 0.5f;
    const float liteB = cb + (1.0f - cb) * 0.5f;
    const float hotR = 1.0f, hotG = 0.58f, hotB = 0.16f;   // broken face: orange
    const float pulse = 0.5f + 0.5f * sinf(visualTime * 3.1f);
    const float yaw = m->MagnetarYaw();
    const float pitch = m->MagnetarPitch();
    // In play the lattice sits on the real interception radius; codex
    // previews (g_SuppressMobSeen) scale it to the body instead.
    const float FR = g_SuppressMobSeen ? base * 2.6f : Monster::MAGNETAR_FIELD_RADIUS;
    static constexpr float kBreakTime = 0.40f;     // break look, then crumple
    static constexpr float kRestoreTime = 0.30f;   // snap-back look
    static constexpr float kPi = 3.1415927f;

    // Per vertex: age of the newest break among its faces (< 0: none broken)
    // and of the newest restore.
    float vBroken[12], vRestored[12];
    for (int i = 0; i < 12; ++i) { vBroken[i] = -1.0f; vRestored[i] = 99.0f; }
    int brokenCount = 0;
    float newestBreak = 99.0f;
    for (int f = 0; f < Monster::MAGNETAR_FACES; ++f) {
        const float bt = m->magnetarFaceBroken[f];
        const float rt = m->magnetarFaceRestored[f];
        if (bt >= 0.0f) { ++brokenCount; newestBreak = std::min(newestBreak, bt); }
        for (int k = 0; k < 3; ++k) {
            const int v = ML::kF[f][k];
            if (bt >= 0.0f && (vBroken[v] < 0.0f || bt < vBroken[v])) vBroken[v] = bt;
            vRestored[v] = std::min(vRestored[v], rt);
        }
    }

    // Vertex scale: buckle on break, keep crumpling while broken, overshoot
    // back to full on restore. Intact vertices stay on the ward radius.
    const float t = visualTime + (float)((size_t)m % 97u) * 0.37f;
    float vScale[12];
    for (int i = 0; i < 12; ++i) {
        float s = 1.0f;
        const float bt = vBroken[i];
        if (bt >= 0.0f) {
            // Recurring buckle, same shape as the break: a quick drop inward
            // with a small overshoot, then a slower spring back.
            const float period = 0.9f + 0.7f * fmodf((float)i * 0.381966f + 0.2f, 1.0f);
            const float c = fmodf(t / period + (float)i * 0.618034f, 1.0f);
            float buckle;
            if (c < 0.22f) {
                const float u = c / 0.22f;
                buckle = 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u) + 0.18f * sinf(kPi * u);
            } else {
                const float u = (c - 0.22f) / 0.78f;
                buckle = 1.0f - u * u * (3.0f - 2.0f * u);
            }
            const float tremble = 0.02f * sinf(t * 31.0f + (float)i * 2.3f);
            const float live = std::clamp(0.88f - 0.26f * buckle + tremble, 0.58f, 1.02f);
            if (bt < kBreakTime) {
                const float delay = 0.04f + 0.16f * fmodf((float)i * 0.618034f, 1.0f);
                const float u = std::clamp((bt - delay) / 0.18f, 0.0f, 1.0f);
                const float e = 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
                s = 1.0f - 0.36f * e - 0.08f * sinf(kPi * u);
            } else if (bt < kBreakTime + 0.35f) {                   // ease into the crumple
                const float k = (bt - kBreakTime) / 0.35f;
                s = 0.64f + (live - 0.64f) * k * k;
            } else {
                s = live;
            }
        } else if (vRestored[i] < kRestoreTime) {
            // Ease-out-back from a crumpled size to full.
            const float u = vRestored[i] / kRestoreTime - 1.0f;
            s = 0.75f + 0.25f * (1.0f + 2.70158f * u * u * u + 1.70158f * u * u);
        }
        vScale[i] = s;
    }

    // Project the lattice (unit vertices scaled so they sit on the circle).
    static const float kNorm = 1.0f / sqrtf(1.0f + ML::kPhi * ML::kPhi);
    ML::Proj pv[12];
    float vx[12], vy[12];
    for (int i = 0; i < 12; ++i) {
        pv[i] = ML::Project(ML::kV[i][0] * kNorm, ML::kV[i][1] * kNorm, ML::kV[i][2] * kNorm,
                            yaw, pitch);
        vx[i] = pv[i].x;
        vy[i] = pv[i].y;
    }
    for (int i = 0; i < 12; ++i) {
        const float r = sqrtf(vx[i] * vx[i] + vy[i] * vy[i]);
        const float ang = atan2f(vy[i], vx[i]);
        const float rr = r * FR * vScale[i];
        vx[i] = x + cosf(ang) * rr;
        vy[i] = y + sinf(ang) * rr;
    }

    // Lit faces: the face that just stopped a shot.
    auto faceLit = [&](int f) {
        return m->magnetarFaceHitFlash[f] / Monster::MAGNETAR_HIT_FLASH;
    };

    {
        // Edges: each of the 30 edges once. An edge is front when either of
        // its two faces faces the viewer, lit if either face is lit, broken
        // (with the newest break age) if either face is broken.
        bool edgeFront[12][12] = {};
        float edgeLit[12][12] = {};
        bool edgeSeen[12][12] = {};
        float edgeBroken[12][12];
        float edgeRestored[12][12];
        for (int a = 0; a < 12; ++a)
            for (int b = 0; b < 12; ++b) { edgeBroken[a][b] = -1.0f; edgeRestored[a][b] = 99.0f; }
        for (int f = 0; f < 20; ++f) {
            const int ids[3] = { ML::kF[f][0], ML::kF[f][1], ML::kF[f][2] };
            const bool front = pv[ids[0]].z + pv[ids[1]].z + pv[ids[2]].z > 0.0f;
            const float bt = m->magnetarFaceBroken[f];
            const float lit = bt >= 0.0f ? 0.0f : faceLit(f);
            for (int e = 0; e < 3; ++e) {
                const int a = std::min(ids[e], ids[(e + 1) % 3]);
                const int b = std::max(ids[e], ids[(e + 1) % 3]);
                edgeSeen[a][b] = true;
                edgeFront[a][b] = edgeFront[a][b] || front;
                edgeLit[a][b] = std::max(edgeLit[a][b], lit);
                if (bt >= 0.0f && (edgeBroken[a][b] < 0.0f || bt < edgeBroken[a][b]))
                    edgeBroken[a][b] = bt;
                edgeRestored[a][b] = std::min(edgeRestored[a][b], m->magnetarFaceRestored[f]);
            }
        }
        for (int a = 0; a < 12; ++a) {
            for (int b = a + 1; b < 12; ++b) {
                if (!edgeSeen[a][b]) continue;
                const float bt = edgeBroken[a][b];
                if (bt >= 0.0f) {
                    // Broken face: orange, flickering a little.
                    float er = hotR, eg = hotG, eb = hotB;
                    float flick = 0.85f + 0.15f * sinf(visualTime * 23.0f + (float)(a * 7 + b));
                    float width = edgeFront[a][b] ? 1.4f : 0.8f;
                    if (bt < kBreakTime) {
                        // Failing power: gold and orange alternate (per edge
                        // offset), settling into orange; white overload on top.
                        const float k = bt / kBreakTime;
                        const float flash = std::max(0.0f, 1.0f - bt / 0.07f);
                        const bool gold = sinf(bt * 75.0f + (float)(a * 3 + b)) > (2.0f * k - 1.0f);
                        if (gold) { er = cr; eg = cg; eb = cb; }
                        er += (1.0f - er) * flash;
                        eg += (1.0f - eg) * flash;
                        eb += (1.0f - eb) * flash;
                        flick = 1.0f + 0.4f * (1.0f - k);
                        width += 1.0f * (1.0f - k);
                    }
                    drawLineQuad(vx[a], vy[a], vx[b], vy[b], width, er, eg, eb,
                                 std::min(1.0f, (edgeFront[a][b] ? 0.75f : 0.25f) * flick));
                } else if (edgeLit[a][b] > 0.0f) {
                    drawLineQuad(vx[a], vy[a], vx[b], vy[b], 2.4f,
                                 1.0f, 1.0f, 1.0f, 0.95f * edgeLit[a][b]);
                } else {
                    const float snap = std::max(0.0f, 1.0f - edgeRestored[a][b] / kRestoreTime);
                    if (edgeFront[a][b] || snap > 0.0f) {
                        drawLineQuad(vx[a], vy[a], vx[b], vy[b], 1.6f + 0.8f * snap,
                                     cr + (1.0f - cr) * snap, cg + (1.0f - cg) * snap,
                                     cb + (1.0f - cb) * snap,
                                     (edgeFront[a][b] ? 0.80f : 0.22f) + 0.20f * snap);
                    } else {
                        drawLineQuad(vx[a], vy[a], vx[b], vy[b], 0.9f, cr, cg, cb, 0.22f);
                    }
                }
            }
        }
        for (int i = 0; i < 12; ++i)
            if (pv[i].z > 0.15f) {
                if (vBroken[i] >= 0.0f) drawCircle(vx[i], vy[i], 1.8f, 1.0f, 0.80f, 0.55f, 0.55f);
                else drawCircle(vx[i], vy[i], 2.0f, 1.0f, 0.97f, 0.88f, 0.70f);
            }
    }

    // Crystal seed: octahedron turning against the lattice. It tints orange
    // with the share of broken faces (fully orange from 5 broken).
    const float seedX = x, seedY = y;
    const float seedR = base * 0.42f;
    const float back = 1.0f - std::min(1.0f, (float)brokenCount / 5.0f);   // 0 orange .. 1 gold
    static const float kOct[6][3] = { {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1} };
    ML::Proj ov[6];
    float sx[6], sy[6];
    for (int i = 0; i < 6; ++i) {
        ov[i] = ML::Project(kOct[i][0], kOct[i][1] * 1.35f, kOct[i][2], -yaw * 1.6f, pitch);
        sx[i] = seedX + ov[i].x * seedR;
        sy[i] = seedY + ov[i].y * seedR;
    }
    const float sr = hotR + (1.0f - hotR) * back, sg = hotG + (0.97f - hotG) * back,
                sb = hotB + (0.85f - hotB) * back;
    drawCircle(seedX, seedY, seedR * 1.4f, hotR + (cr - hotR) * back, hotG + (cg - hotG) * back,
               hotB + (cb - hotB) * back, 0.10f + (brokenCount > 0 ? 0.15f * pulse : 0.06f));
    if (newestBreak < 0.30f) {      // seed flash at a face break
        const float w = 1.0f - newestBreak / 0.30f;
        drawCircle(seedX, seedY, seedR * (1.4f + 1.2f * (1.0f - w)), 1.0f, 0.85f, 0.6f, 0.45f * w * w);
    }
    static const int kOctE[12][2] = { {0, 2}, {0, 3}, {0, 4}, {0, 5}, {1, 2}, {1, 3},
                                      {1, 4}, {1, 5}, {2, 4}, {2, 5}, {3, 4}, {3, 5} };
    for (const auto& e : kOctE) {
        const bool front = ov[e[0]].z + ov[e[1]].z >= 0.0f;
        drawLineQuad(sx[e[0]], sy[e[0]], sx[e[1]], sy[e[1]], front ? 1.8f : 0.9f,
                     sr, sg, sb, front ? 0.95f : 0.35f);
    }
    drawCircle(seedX, seedY, base * 0.07f, 1.0f, 1.0f, 1.0f, 0.95f);

    // Tethers: four seed tips (equator) to the nearest front lattice vertices
    // that are not on a broken face.
    {
        static const int kTips[4] = { 0, 1, 4, 5 };
        bool used[12] = {};
        for (int k = 0; k < 4; ++k) {
            const int tip = kTips[k];
            int best = -1;
            float bestD = 1e9f;
            for (int i = 0; i < 12; ++i) {
                if (used[i] || pv[i].z < -0.1f || vBroken[i] >= 0.0f) continue;
                const float dx = vx[i] - sx[tip], dy = vy[i] - sy[tip];
                const float dd = dx * dx + dy * dy;
                if (dd < bestD) { bestD = dd; best = i; }
            }
            if (best < 0) continue;
            used[best] = true;
            drawLineQuad(sx[tip], sy[tip], vx[best], vy[best], 1.2f, liteR, liteG, liteB, 0.45f);
            drawCircle(vx[best], vy[best], 2.4f, 1.0f, 1.0f, 1.0f, 0.6f);
        }
    }
}

// Three incommensurate sines: a [-1, 1] signal with no audible "beat", used
// to keep the Quasar core from reading as a steady pulse.
static float QuasarNoise(float t) {
    return 0.5f * sinf(t * 7.3f) + 0.3f * sinf(t * 12.9f + 1.7f)
         + 0.2f * sinf(t * 23.1f + 0.4f);
}

static float QuasarHash(float n) {
    const float v = sinf(n * 127.1f) * 43758.5453f;
    return v - floorf(v);
}

// Ellipse outline: centre (cx, cy), first axis at angle `th` with half
// length `a`, second axis half length `b` (signed: flips which half is t in
// [0, pi]). Shared by the Quasar's gyro bands through all of their poses.
static void DrawEllipseLoop(float cx, float cy, float th, float a, float b,
                            float t0, float sweep, int segments,
                            float r, float g, float bl, float alpha, float width) {
    const float ux = cosf(th), uy = sinf(th), vx = -uy, vy = ux;
    float pxp = 0.0f, pyp = 0.0f;
    for (int i = 0; i <= segments; ++i) {
        const float t = t0 + sweep * (float)i / (float)segments;
        const float px = cx + ux * cosf(t) * a + vx * sinf(t) * b;
        const float py = cy + uy * cosf(t) * a + vy * sinf(t) * b;
        if (i > 0) drawLineQuad(pxp, pyp, px, py, width, r, g, bl, alpha);
        pxp = px; pyp = py;
    }
}

// Quasar (tier 4, Q2 gyro orb): a double-contour orb wrapped by two gyro
// bands that keep turning (faster with heat), girdled by an equatorial
// flange that spins on its own. Inside, the contained, unstable attack core
// (diamond frame, clamps, jets, leak sparks) holds the beam axis.
// Charge choreography:
//   wind-up 0-0.5s   the bands slow and fold flat into the flange plane
//   wind-up 0.5-1s   rings shrink into the orb, inner contour brightens
//   lock 0-0.7s      light streams in along the locked axis from both ends
//   lock 0.5-0.7s    the folded bands stand up as accelerator rings round
//                    the axis, the core swells
//   fire             the rings release (with the release ring) and spin free
static void DrawQuasarBody(const Monster* m, float base, float t,
                           float charge, float cr, float cg, float cb) {
    const float x = m->worldX, y = m->worldY;
    ApplyMobHitFlash(cr, cg, cb, m->hitFlashTimer);
    const float liteR = cr + (1.0f - cr) * 0.55f;
    const float liteG = cg + (1.0f - cg) * 0.55f;
    const float liteB = cb + (1.0f - cb) * 0.55f;
    const float heat = m->QuasarHeat();
    const float fallOpen = m->QuasarFallOpen();
    const float fallGather = m->QuasarFallGather();
    const float fallPulse = m->QuasarFallPulse();
    const float seed = (float)((size_t)m % 97u) * 0.37f;
    const float spin = m->quasarShellSpin;
    const float ax = cosf(m->quasarCoreAngle), ay = sinf(m->quasarCoreAngle);
    const float px = -ay, py = ax;
    const float orbR = base * (1.0f - 0.12f * fallGather + 0.10f * fallOpen);

    // Outer layers (tier 4). O3: a tilted orbit round the whole body that
    // precesses against the flange and carries two satellites; while the
    // aim is locked and firing it swings its long axis onto the beam axis and
    // flattens, then eases back. O1: a flat halo ring (double contour) with
    // three bracketed star nodes turning the other way; the wind-up rings
    // peel off it. Far halves are drawn here, near halves after the flange.
    auto easeO = [](float v) {
        return Monster::QuasarEase(v);
    };
    float orbitAlign = 0.0f;
    if (m->quasarState == 0) orbitAlign = 0.55f * easeO(m->QuasarPrecharge());
    else if (m->quasarState == 1) orbitAlign = 0.55f;
    else if (m->quasarState == 2)
        orbitAlign = 0.55f + 0.45f * easeO(m->quasarTimer / (Monster::QUASAR_LOCK_TIME * 0.85f));
    else if (m->quasarState == 3)
        orbitAlign = 1.0f;
    else if (m->quasarState == 4)
        orbitAlign = 1.0f - easeO(m->quasarTimer / 0.6f);
    const float axisAng = atan2f(m->quasarAimY, m->quasarAimX);
    float o3Ang = -m->quasarVisualAngle * 0.6f + seed * 2.0f;
    {
        float dA = axisAng - o3Ang;                       // ellipse repeats every pi
        dA -= 3.1415927f * floorf(dA / 3.1415927f + 0.5f);
        o3Ang += dA * orbitAlign;
    }
    const float o3A = base * 2.25f;
    const float o3B = base * (0.78f - 0.48f * orbitAlign);
    const float o3Heat = std::max(heat, orbitAlign);
    DrawEllipseLoop(x, y, o3Ang, o3A, o3B, 3.1415927f, 3.1415927f, 36,
                    cr, cg, cb, 0.36f + 0.20f * o3Heat, 1.2f);
    EnemyNodeAnchor sats[2];
    bool satFront[2];
    for (int i = 0; i < 2; ++i) {
        const float a = spin * 0.30f + 3.1415927f * (float)i + seed;
        const float ux = cosf(o3Ang), uy = sinf(o3Ang);
        sats[i] = { x + ux * cosf(a) * o3A - uy * sinf(a) * o3B,
                    y + uy * cosf(a) * o3A + ux * sinf(a) * o3B,
                    base * (i ? 0.06f : 0.085f), 0.80f + 0.20f * o3Heat };
        satFront[i] = sinf(a) >= 0.0f;
        if (!satFront[i]) DrawEnemyNode(sats[i], cr, cg, cb, 0.42f);
    }

    const float pre0 = m->QuasarPrecharge();
    const float beamGather = m->quasarState == 0 ? easeO(pre0) :
        ((m->quasarState == 1 || m->quasarState == 2) ? 1.0f :
         (m->quasarState == 3 ? 1.0f - easeO(m->quasarTimer / 0.3f) : 0.0f));
    const float haloR = base * (1.95f - 0.25f * beamGather - 0.22f * fallGather +
                                0.65f * fallOpen + 0.04f * fallPulse);
    const float haloGlow = std::max(heat, easeO(pre0 * 1.4f));
    DrawEnemyArc(x, y, haloR, 0.0f, 6.2831853f, 64, cr, cg, cb,
                 0.58f + 0.30f * haloGlow, 1.5f + 0.5f * haloGlow, false);
    DrawEnemyArc(x, y, haloR * 0.965f, 0.0f, 6.2831853f, 60, liteR, liteG, liteB,
                 0.24f + 0.20f * haloGlow, 0.9f, false);
    for (int k = 0; k < 3; ++k) {
        const float a = -spin * 0.22f + 2.0943951f * (float)k + seed;
        DrawEnemyArc(x, y, haloR * 1.05f, a - 0.13f, 0.26f, 6, liteR, liteG, liteB,
                     0.55f + 0.35f * haloGlow, 1.2f, false);
        DrawEnemyArc(x, y, haloR * 0.93f, a - 0.10f, 0.20f, 5, liteR, liteG, liteB,
                     0.40f + 0.30f * haloGlow, 1.0f, false);
        DrawEnemyNode({x + cosf(a) * haloR, y + sinf(a) * haloR, base * 0.07f,
                       0.80f + 0.20f * haloGlow}, liteR, liteG, liteB, 0.92f);
    }

    // Jet weight: grows through charge, holds while firing, then retracts.
    float jetT = 0.0f;
    if (m->quasarState == 2) jetT = charge;
    else if (m->quasarState == 3) jetT = 1.0f;
    else if (m->quasarState == 4) jetT = 1.0f - easeO(m->quasarTimer / 0.4f);

    // Equatorial flange: independent of the beam. Its tilt precesses slowly
    // (quasarVisualAngle) while the ring itself spins, shown by four lugs
    // travelling round it. DrawQuasarEllipseArc puts `major` along
    // (-aimY, aimX), so pass the perpendicular to lay it on (fx, fy).
    const float flA = m->QuasarFlangeAngle();
    const float fx = cosf(flA), fy = sinf(flA);
    const float flMajor = base * (1.55f - 0.18f * fallGather + 0.65f * fallOpen);
    const float flMinor = base * (0.40f - 0.10f * fallGather + 0.70f * fallOpen) * (1.0f - 0.30f * heat);
    // Back half (behind the orb), dim, drawn first.
    DrawQuasarEllipseArc(x, y, fy, -fx, flMajor, flMinor, 3.1415927f, 3.1415927f, 24,
                         cr, cg, cb, 0.30f + 0.12f * heat, 1.30f);
    EnemyNodeAnchor lugs[4];
    bool lugFront[4];
    for (int i = 0; i < 4; ++i) {
        const float a = spin * 0.45f + 1.5707963f * (float)i;
        const float ca = cosf(a), sa = sinf(a);
        lugs[i] = { x + fx * ca * flMajor + fy * sa * flMinor,
                    y + fy * ca * flMajor - fx * sa * flMinor,
                    base * (0.065f + 0.045f * fallOpen), 0.75f + 0.25f * heat };
        lugFront[i] = sa >= 0.0f;
        const float visible = m->QuasarPortVisibility(i);
        if (!lugFront[i]) DrawEnemyNode(lugs[i], cr, cg, cb, 0.40f * visible);
    }

    // Charge choreography weights.
    auto ease = [](float v) {
        return Monster::QuasarEase(v);
    };
    const float pre = m->QuasarPrecharge();
    const float lockT = m->quasarState == 2
        ? std::min(1.0f, m->quasarTimer / Monster::QUASAR_LOCK_TIME) : 0.0f;
    float fold = 0.0f, stand = 0.0f;
    if (m->quasarState == 0) {
        fold = ease(pre / 0.5f);
    } else if (m->quasarState == 1 || m->quasarState == 2) {
        fold = 1.0f;
        stand = ease((lockT - 0.714f) / 0.286f);          // last 0.2s of the lock
    } else if (m->quasarState == 3) {
        const float release = ease(m->quasarTimer / 0.30f);
        fold = 1.0f - release;
        stand = 1.0f - release;
    }

    // Gyro bands. Free pose: each turns about its own long axis (signed
    // minor) while the pair precesses. Folded: a smaller ring in the flange
    // plane. Standing: an accelerator ring round the beam axis, one per side.
    struct BandPose { float cx, cy; glm::vec2 u, v; float depth; };
    auto pose = [](float cx, float cy, float th, float a, float b) {
        return BandPose{cx, cy, {cosf(th) * a, sinf(th) * a},
                        {-sinf(th) * b, cosf(th) * b}, b};
    };
    auto mix = [](const BandPose& p, const BandPose& q, float k) {
        return BandPose{p.cx + (q.cx - p.cx) * k, p.cy + (q.cy - p.cy) * k,
                        p.u * (1-k) + q.u * k, p.v * (1-k) + q.v * k,
                        p.depth + (q.depth - p.depth) * k};
    };
    auto drawBand = [&](const BandPose& p, bool front, float r, float g, float b,
                        float alpha, float width) {
        const float start = front ? 0.0f : 3.1415927f;
        glm::vec2 previous = glm::vec2(p.cx,p.cy) + p.u * cosf(start) + p.v * sinf(start);
        for (int k=1;k<=20;++k) {
            const float a = start + 3.1415927f * k / 20.0f;
            const auto point = glm::vec2(p.cx,p.cy) + p.u * cosf(a) + p.v * sinf(a);
            drawLineQuad(previous.x,previous.y,point.x,point.y,width,r,g,b,alpha);
            previous=point;
        }
    };
    BandPose bands[2];
    const float axisA = atan2f(ay, ax);
    for (int i = 0; i < 2; ++i) {
        const float sgn = i ? -1.0f : 1.0f;
        const float tilt = -m->quasarVisualAngle * 0.35f + (i ? -1.0472f : 0.6109f) + spin * 0.08f;
        const float roll = spin * 0.55f + 1.5707963f * (float)i + seed;
        const float mn = sinf(roll); // signed width crosses zero without flipping
        const auto freeP = pose(x,y,tilt+1.5707963f,orbR*0.97f,orbR*0.97f*mn);
        const auto foldP = pose(x,y,flA,orbR*0.88f,orbR*0.88f*(flMinor/flMajor)*sgn);
        const auto openP = pose(x+fx*base*0.15f*sgn,y+fy*base*0.15f*sgn,
                                flA+sgn*0.8f,orbR*1.50f,orbR*0.85f*sgn);
        const auto standP = pose(x+ax*orbR*0.55f*sgn,y+ay*orbR*0.55f*sgn,
                                 axisA,orbR*0.16f,orbR*0.80f);
        // Blend the ellipse's two basis vectors, not wrapped angles: taking
        // a different shortest angle midway through a charge caused snapping.
        bands[i]=mix(mix(mix(freeP,openP,fallOpen),foldP,fold),standP,stand);
        drawBand(bands[i],false,cr,cg,cb,0.34f+0.10f*heat+0.40f*stand,0.95f+0.6f*stand);
    }
    // Band riders: one star node per band, dim while behind; they fade out
    // as the bands fold.
    EnemyNodeAnchor riders[2];
    bool riderFront[2];
    float riderOpacity[2];
    const float riderA = 1.0f - fold;
    for (int i = 0; i < 2; ++i) {
        const float a = spin * 0.9f + 3.1415927f * (float)i + seed;
        const BandPose& b = bands[i];
        const auto position=glm::vec2(b.cx,b.cy)+b.u*cosf(a)+b.v*sinf(a);
        riders[i] = {position.x,position.y,base*0.075f,0.75f+0.25f*heat};
        const float depth=sinf(a)*b.depth;
        riderFront[i]=depth>=0;
        riderOpacity[i]=0.685f+0.235f*std::clamp(depth/(base*0.08f),-1.0f,1.0f);
        if (!riderFront[i] && riderA > 0.0f)
            DrawEnemyNode(riders[i],cr,cg,cb,riderOpacity[i]*riderA);
    }

    // Wind-up 0.5-1s: rings shrink into the orb, a new one every third of
    // the cycle, fading in as the phase starts.
    if (pre > 0.5f) {
        const float u = (pre - 0.5f) / 0.5f;
        const float fadeIn = ease(u * 4.0f) * (1.0f - ease((u - 0.65f) / 0.35f));
        for (int j = 0; j < 3; ++j) {
            float ph = u * 2.0f + (float)j / 3.0f;
            ph -= floorf(ph);
            const float rr = base * (1.90f - 0.92f * ph);   // peel off the halo
            DrawEnemyArc(x, y, rr, 0.0f, 6.2831853f, 44, liteR, liteG, liteB,
                         0.45f * sinf(3.1415927f * ph) * fadeIn, 1.2f, false);
        }
    }

    // Lock: light streams in along the locked axis from both ends, thin and
    // dim far out, thick and white near its head.
    if (m->quasarState == 2) {
        const float head = base * (3.2f - 2.15f * ease(lockT * 1.15f));
        const float tail = base * 3.2f;
        for (int side = 0; side < 2; ++side) {
            const float sd = side ? -1.0f : 1.0f;
            static constexpr int kSeg = 10;
            for (int k = 0; k < kSeg; ++k) {
                const float r0 = tail + (head - tail) * (float)k / (float)kSeg;
                const float r1 = tail + (head - tail) * (float)(k + 1) / (float)kSeg;
                const float f = (float)(k + 1) / (float)kSeg;
                drawLineQuad(x + ax * r0 * sd, y + ay * r0 * sd,
                             x + ax * r1 * sd, y + ay * r1 * sd,
                             0.8f + 2.6f * f,
                             liteR + (1.0f - liteR) * f, liteG + (1.0f - liteG) * f,
                             liteB + (1.0f - liteB) * f, 0.25f + 0.65f * f);
            }
        }
    }

    // Orb: double contour; the inner contour warms with heat.
    DrawEnemyArc(x, y, orbR, 0.0f, 6.2831853f, 48, cr, cg, cb, 0.86f + 0.12f * heat, 1.9f, false);
    const float innerGlow = m->quasarState == 0 ? ease((pre - 0.5f) / 0.5f)
                          : (m->quasarState <= 3 ? 1.0f :
                             (m->quasarState == 4 ? 1.0f - ease(m->quasarTimer / 0.6f) : 0.0f));
    DrawEnemyArc(x, y, orbR * 0.90f, 0.0f, 6.2831853f, 44,
                 liteR, liteG, liteB, 0.34f + 0.30f * heat + 0.30f * innerGlow,
                 0.95f + 0.4f * innerGlow, false);

    // L3: contained core on the beam axis.
    const float amp = 0.25f + 0.75f * heat;
    const float noise = QuasarNoise(t + seed);
    const float halfL = base * (0.28f + 0.10f * jetT * jetT);
    const float halfW = base * 0.22f * (1.0f - 0.20f * jetT);
    const float dX[4] = { x + ax * halfL, x + px * halfW, x - ax * halfL, x - px * halfW };
    const float dY[4] = { y + ay * halfL, y + py * halfW, y - ay * halfL, y - py * halfW };
    for (int i = 0; i < 4; ++i) {
        const int n = (i + 1) & 3;
        drawLineQuad(dX[i], dY[i], dX[n], dY[n], 1.30f,
                     liteR, liteG, liteB, 0.80f + 0.20f * jetT);
        drawLineQuad(x + (dX[i] - x) * 0.55f, y + (dY[i] - y) * 0.55f,
                     x + (dX[n] - x) * 0.55f, y + (dY[n] - y) * 0.55f,
                     0.75f, cr, cg, cb, 0.35f);
    }

    // Containment clamps: four corner brackets on the diagonals that tremble
    // and get pushed outward as the charge peaks.
    const float clampR = base * (0.52f + 0.14f * jetT * jetT);
    for (int i = 0; i < 4; ++i) {
        const float c = atan2f(ay, ax) + 0.7853982f + 1.5707963f * (float)i;
        const float shake = amp * 1.2f * sinf(t * (29.0f + 3.0f * i) + seed + i);
        const float r = clampR + shake;
        DrawEnemyArc(x, y, r, c - 0.26f, 0.52f, 4,
                     liteR, liteG, liteB, 0.70f + 0.25f * heat, 1.25f, false);
        for (int e = -1; e <= 1; e += 2) {
            const float ea = c + 0.26f * (float)e;
            drawLineQuad(x + cosf(ea) * r, y + sinf(ea) * r,
                         x + cosf(ea) * (r - base * 0.07f),
                         y + sinf(ea) * (r - base * 0.07f),
                         1.10f, liteR, liteG, liteB, 0.70f + 0.25f * heat);
        }
    }

    // L4: jets from the diamond tips out through the emitter ports.
    for (int side = 0; side < 2; ++side) {
        const float s = side ? -1.0f : 1.0f;
        const float tipX = x + ax * halfL * s, tipY = y + ay * halfL * s;
        if (jetT > 0.0f) {
            const float len = (orbR + base * 0.55f - halfL) * jetT;
            const float ex = tipX + ax * s * len, ey = tipY + ay * s * len;
            drawLineQuad(tipX, tipY, ex, ey, 3.2f, cr, cg, cb, 0.22f + 0.25f * jetT);
            drawLineQuad(tipX, tipY, ex, ey, 1.0f, 1.0f, 1.0f, 1.0f, 0.55f + 0.40f * jetT);
        }
        DrawEnemyNode({tipX, tipY, base * 0.085f,
                       m->quasarState >= 1 ? 1.0f : 0.55f},
                      1.0f, 1.0f, 1.0f, 0.90f);
    }

    // Leak sparks: brief zigzags along the beam axis, more frequent with heat.
    const float rate = 2.0f + 10.0f * heat;
    const float slotF = (t + seed) * rate;
    const float slot = floorf(slotF);
    if (m->quasarState < Monster::QUASAR_FALL_STATE &&
        (slotF - slot) < 0.08f * rate && QuasarHash(slot + seed) < 0.65f) {
        const float h2 = QuasarHash(slot * 1.7f + 3.1f);
        const float s = QuasarHash(slot * 2.3f + 7.7f) < 0.5f ? -1.0f : 1.0f;
        const float dev = (h2 - 0.5f) * 0.5f;
        const float dirX = (ax * cosf(dev) - ay * sinf(dev)) * s;
        const float dirY = (ay * cosf(dev) + ax * sinf(dev)) * s;
        const float len = base * (0.35f + 0.40f * h2);
        float sx = x + dirX * base * 0.18f, sy = y + dirY * base * 0.18f;
        for (int k = 1; k <= 4; ++k) {
            const float f = (float)k / 4.0f;
            const float off = (k == 4) ? 0.0f : ((k & 1) ? 1.0f : -1.0f) * base * 0.05f;
            const float nx = x + dirX * (base * 0.18f + len * f) - dirY * off;
            const float ny = y + dirY * (base * 0.18f + len * f) + dirX * off;
            drawLineQuad(sx, sy, nx, ny, 1.2f, 1.0f, 1.0f, 1.0f, 0.85f);
            sx = nx; sy = ny;
        }
    }

    // Unstable core: off-beat flicker, a jittering white center inside a
    // fixed halo, and a rim that bulges one side at a time.
    const float flicker = 0.80f + 0.20f * amp * noise;
    const float coreS = base * 0.20f * (1.0f + 0.12f * heat + 0.35f * stand);
    drawCircle(x, y, coreS * 1.70f, cr, cg, cb, 0.12f * flicker);
    drawCircle(x, y, coreS * 1.15f, cr, cg, cb, 0.30f * flicker);
    drawCircle(x, y, coreS * 0.72f, liteR, liteG, liteB, 0.82f * flicker);
    float rimPX = 0.0f, rimPY = 0.0f;
    for (int k = 0; k <= 20; ++k) {
        const float a = 6.2831853f * (float)k / 20.0f;
        const float bulge = std::max(0.0f, sinf(a * 2.0f + t * 5.3f + seed)
                                         * sinf(t * 3.1f + a));
        const float r = coreS * 1.05f * (1.0f + 0.22f * amp * bulge);
        const float rx = x + cosf(a) * r, ry = y + sinf(a) * r;
        if (k > 0)
            drawLineQuad(rimPX, rimPY, rx, ry, 1.0f,
                         liteR, liteG, liteB, 0.55f * flicker);
        rimPX = rx; rimPY = ry;
    }
    const float jx = amp * 1.6f * sinf(t * 31.7f + seed);
    const float jy = amp * 1.6f * sinf(t * 27.3f + seed * 1.3f);
    drawCircle(x + jx, y + jy, coreS * 0.32f, 1.0f, 1.0f, 1.0f,
               std::min(1.0f, 0.85f * flicker + 0.15f));

    // Front halves of the bands, front riders, then the flange front half
    // pass over everything.
    for (int i = 0; i < 2; ++i)
        drawBand(bands[i],true,liteR+(1-liteR)*stand,liteG+(1-liteG)*stand,
                 liteB+(1-liteB)*stand,0.70f+0.25f*heat,1.45f+0.5f*stand);
    for (int i = 0; i < 2; ++i)
        if (riderFront[i] && riderA > 0.0f)
            DrawEnemyNode(riders[i],cr,cg,cb,riderOpacity[i]*riderA);
    DrawQuasarEllipseArc(x, y, fy, -fx, flMajor, flMinor, 0.0f, 3.1415927f, 24,
                         cr, cg, cb, 0.82f + 0.15f * heat, 2.2f);
    DrawQuasarEllipseArc(x, y, fy, -fx, flMajor * 0.92f, flMinor * 0.80f, 0.35f, 2.44f, 18,
                         liteR, liteG, liteB, 0.22f + 0.20f * heat, 0.80f);
    for (int i = 0; i < 4; ++i) {
        const float visible = m->QuasarPortVisibility(i);
        if (lugFront[i]) DrawEnemyNode(lugs[i], liteR, liteG, liteB, 0.92f * visible);
    }
    // Near half of the tilted outer orbit and its front satellites.
    DrawEllipseLoop(x, y, o3Ang, o3A, o3B, 0.0f, 3.1415927f, 36,
                    liteR, liteG, liteB, 0.55f + 0.35f * o3Heat, 1.5f + 0.6f * o3Heat);
    for (int i = 0; i < 2; ++i)
        if (satFront[i]) DrawEnemyNode(sats[i], liteR, liteG, liteB, 0.95f);
    if (fallPulse > 0.0f && m->quasarFallFired > 0) {
        const int last = m->quasarFallFired - 1;
        const glm::vec2 port = m->quasarFallOrigins[last];
        DrawEnemyArc(port.x, port.y, base * (0.12f + 0.25f * (1.0f - fallPulse)),
                     0.0f, 6.2831853f, 16, 1.0f, 1.0f, 1.0f, fallPulse * 0.9f, 1.8f, false);
    }

}

// Gravis elite telegraphs and effects: sling chevrons, the thrown mass and
// its return arc, the glide lane and destination circle, the arrival burst.
// Lines only; every danger shape is a thin solid outline.
static void DrawGravisElite(const Monster* m, float base, float cr, float cg, float cb) {
    const int st = m->gravisState;   // body machine: glide
    const int sl = m->gravisSling;   // sling machine: satellites
    if (st == Monster::GRAVIS_IDLE && sl == Monster::GRAVIS_IDLE) return;
    const float x = m->worldX, y = m->worldY;
    const float lr = cr + (1.0f - cr) * 0.45f;
    const float lg = cg + (1.0f - cg) * 0.45f;
    const float lb = cb + (1.0f - cb) * 0.45f;
    // Dark under-stroke + colored line + white core, readable on any window.
    auto edge = [&](glm::vec2 a, glm::vec2 b, float alpha, float width) {
        drawLineQuad(a.x, a.y, b.x, b.y, width + 3.5f, 0.0f, 0.0f, 0.0f, 0.65f * alpha);
        drawLineQuad(a.x, a.y, b.x, b.y, width, cr, cg, cb, alpha);
        drawLineQuad(a.x, a.y, b.x, b.y, width * 0.4f, 1.0f, 1.0f, 1.0f, 0.8f * alpha);
    };
    auto mass = [&](glm::vec2 p) {
        DrawEnemyArc(p.x, p.y, 13.0f, 0.0f, 6.2831853f, 28, 0.0f, 0.0f, 0.0f, 0.7f, 5.0f, false);
        DrawEnemyArc(p.x, p.y, 13.0f, 0.0f, 6.2831853f, 28, lr, lg, lb, 0.95f, 2.2f, false);
        drawCircle(p.x, p.y, 4.6f, 1.0f, 1.0f, 1.0f, 1.0f);
    };

    if (sl != Monster::GRAVIS_IDLE) {
        const float t = m->gravisSlingTimer;
        if (sl == Monster::GRAVIS_SLING_LOCK) {
            // Chevrons ahead of the wound-up orbit fix the throw lines.
            const float grow = std::min(1.0f, t / 0.12f);
            for (int i = 0; i < Monster::GRAVIS_SLING_COUNT; ++i) {
                const glm::vec2 d = m->GravisSatDir(i);
                const glm::vec2 dside(-d.y, d.x);
                for (int k = 0; k < 3; ++k) {
                    const glm::vec2 c = glm::vec2(x, y) + d * (base * 1.9f + 24.0f * k);
                    const glm::vec2 tip = c + d * 8.0f, a = c - d * 6.0f + dside * 11.0f,
                                    b = c - d * 6.0f - dside * 11.0f;
                    const float alpha = grow * (1.0f - 0.22f * k);
                    for (const auto& seg : {std::pair<glm::vec2, glm::vec2>{a, tip}, {b, tip}}) {
                        drawLineQuad(seg.first.x, seg.first.y, seg.second.x, seg.second.y,
                                     5.5f, 0.0f, 0.0f, 0.0f, 0.55f * alpha);
                        drawLineQuad(seg.first.x, seg.first.y, seg.second.x, seg.second.y,
                                     2.4f, 1.0f, 1.0f, 1.0f, alpha);
                    }
                }
            }
        }
        if (sl == Monster::GRAVIS_SLING_OUT) {
            const float u = std::min(1.0f, t / Monster::GRAVIS_SLING_OUT_TIME);
            for (int i = 0; i < Monster::GRAVIS_SLING_COUNT; ++i) {
                for (int k = 0; k < 8; ++k) {
                    const float u0 = u - 0.04f * k, u1 = u - 0.04f * (k + 1);
                    if (u1 < 0.0f) break;
                    const glm::vec2 head = m->GravisSatOut(u0, i), tail = m->GravisSatOut(u1, i);
                    const float fade = 1.0f - k / 8.0f;
                    drawLineQuad(tail.x, tail.y, head.x, head.y, 5.0f * fade, cr, cg, cb, 0.65f * fade);
                    drawLineQuad(tail.x, tail.y, head.x, head.y, 1.1f * fade, 1.0f, 1.0f, 1.0f, 0.85f * fade);
                }
                mass(m->gravisSatPos[i]);
            }
        }
        if (sl == Monster::GRAVIS_SLING_HARASS || sl == Monster::GRAVIS_SLING_BACK) {
            const bool harass = sl == Monster::GRAVIS_SLING_HARASS;
            for (int i = 0; i < Monster::GRAVIS_SLING_COUNT; ++i) {
                if (!harass && (m->gravisSatDocked & (1u << i))) continue;
                const glm::vec2 p = m->gravisSatPos[i];
                const glm::vec2 v = p - m->gravisSatPrev[i];
                const bool tele = harass && m->gravisDartState[i] == 1;
                const float phase = m->gravisDartTimer[i];
                const bool flying = !harass || m->GravisDartFlying(i);
                if (tele) {
                    // The mass hovers, so this arc is the real flight path: it bends
                    // through the aim point and on to where the dart will stop.
                    const glm::vec2 tg = m->gravisDartTarget[i];
                    const Monster::GravisDartPath path = m->GravisDartPlan(i);
                    const float k = std::clamp(phase / Monster::GRAVIS_DART_TELEGRAPH, 0.0f, 1.0f);
                    const int segs = 28;
                    glm::vec2 prev = path.from;
                    for (int q = 1; q <= segs; ++q) {
                        const glm::vec2 cur = Monster::GravisDartAt(path, path.length * (float)q / (float)segs);
                        edge(prev, cur, 0.30f + 0.55f * k, 1.4f);
                        prev = cur;
                    }
                    const glm::vec2 end = prev;
                    const glm::vec2 d = Monster::GravisDartTangent(path, path.length);
                    const glm::vec2 sd(-d.y, d.x);
                    edge(end + sd * 12.0f, end - sd * 12.0f, 0.5f + 0.4f * k, 1.4f);   // far cap
                    // Chevrons run along the arc toward the end.
                    for (int c = 0; c < 4; ++c) {
                        const float u = std::fmod(k * 3.0f + c * 0.25f, 1.0f);
                        const glm::vec2 qp = Monster::GravisDartAt(path, path.length * u);
                        const glm::vec2 qd = Monster::GravisDartTangent(path, path.length * u);
                        const glm::vec2 qs(-qd.y, qd.x);
                        const glm::vec2 a2 = qp - qd * 7.0f + qs * 7.0f, b2 = qp - qd * 7.0f - qs * 7.0f;
                        drawLineQuad(a2.x, a2.y, qp.x, qp.y, 1.8f, 1.0f, 1.0f, 1.0f, 0.55f * k);
                        drawLineQuad(b2.x, b2.y, qp.x, qp.y, 1.8f, 1.0f, 1.0f, 1.0f, 0.55f * k);
                    }
                    DrawEnemyArc(tg.x, tg.y, 14.0f, 0.0f, 6.2831853f, 24, lr, lg, lb, 0.8f, 1.4f, false);
                    DrawEnemyArc(tg.x, tg.y, 14.0f, -1.5707963f, k * 6.2831853f, 24,
                                 1.0f, 1.0f, 1.0f, 0.9f, 1.8f, false);
                    // Wind-up: the mass is drawn back from its anchor along a thin string
                    // while charge arcs close in on it and it flickers brighter.
                    const glm::vec2 an = m->gravisDartAnchor[i];
                    drawLineQuad(an.x, an.y, p.x, p.y, 1.2f, lr, lg, lb, 0.35f * k);
                    for (int c = 0; c < 3; ++c) {
                        const float rr = (26.0f - 14.0f * k) + 4.0f * c;
                        DrawEnemyArc(p.x, p.y, rr, m->gravisSlingTimer * 12.0f + c * 2.1f, 1.7f, 12,
                                     1.0f, 1.0f, 1.0f, (0.25f + 0.55f * k) * (1.0f - 0.2f * c), 1.5f, false);
                    }
                    DrawEnemyArc(p.x, p.y, 13.0f + 6.0f * k, 0.0f, 6.2831853f, 28, 1.0f, 1.0f, 1.0f, 0.55f * k, 1.6f, false);
                }
                // Trail along the real motion; longer while thrown.
                const int trail = flying ? 7 : 4;
                for (int k = 0; k < trail; ++k) {
                    const float fade = 1.0f - k / (float)trail;
                    const glm::vec2 a = p - v * (float)k * 2.0f, b = p - v * (float)(k + 1) * 2.0f;
                    drawLineQuad(b.x, b.y, a.x, a.y, (flying ? 5.0f : 3.0f) * fade, cr, cg, cb, 0.6f * fade);
                }
                mass(p);
            }
        }
    }

    if (m->GravisDestinationShown()) {
        const glm::vec2 aim = m->gravisGlideAim;
        const glm::vec2 side(-aim.y, aim.x);
        const glm::vec2 start = m->gravisGlideStart, dest = m->gravisGlideDest;
        const float rise = st == Monster::GRAVIS_GLIDE_LANE
            ? std::min(1.0f, m->gravisTimer / 0.15f) : 1.0f;
        const float burst = st == Monster::GRAVIS_GLIDE_BURST ? m->gravisTimer : -1.0f;
        const float fadeOut = burst >= 0.0f ? std::max(0.0f, 1.0f - burst / 0.12f) : 1.0f;
        const float laneAlpha = 0.75f * rise * fadeOut;
        const float half = Monster::GRAVIS_GLIDE_LANE_HALF;
        if (laneAlpha > 0.0f) {
            // Path outline: two rails and the far cap. Solid, nothing filled.
            edge(start + side * half, dest + side * half, laneAlpha, 1.6f);
            edge(start - side * half, dest - side * half, laneAlpha, 1.6f);
            edge(dest + side * half, dest - side * half, laneAlpha, 1.6f);
        }
        const float radius = Monster::GRAVIS_BLAST_RADIUS;
        if (burst < 0.0f) {
            const float t = st == Monster::GRAVIS_GLIDE_LANE ? m->gravisTimer
                          : Monster::GRAVIS_GLIDE_LANE_TIME + m->gravisTimer;
            const float progress = std::clamp(t / (Monster::GRAVIS_GLIDE_LANE_TIME +
                                                   Monster::GRAVIS_GLIDE_RUN_TIME), 0.0f, 1.0f);
            DrawEnemyArc(dest.x, dest.y, radius, 0.0f, 6.2831853f, 64, 0.0f, 0.0f, 0.0f, 0.75f * rise, 5.0f, false);
            DrawEnemyArc(dest.x, dest.y, radius, 0.0f, 6.2831853f, 64, cr, cg, cb, 0.80f * rise, 1.8f, false);
            // Arcs close toward the center while the burst approaches.
            for (int k = 0; k < 3; ++k) {
                const float r = radius * (0.40f + 0.50f * std::fmod(1.0f - progress * 2.0f + k / 3.0f + 3.0f, 1.0f));
                DrawEnemyArc(dest.x, dest.y, r, k * 2.1f + progress * 5.0f, 2.2f, 24,
                             lr, lg, lb, 0.40f * rise, 1.2f, false);
            }
            DrawEnemyArc(dest.x, dest.y, radius, -1.5707963f, progress * 6.2831853f, 64,
                         1.0f, 1.0f, 1.0f, 0.85f * rise, 2.4f, false);
            for (int k = 0; k < 4; ++k) {
                const float a = k * 1.5707963f + 0.7853982f;
                const glm::vec2 d(cosf(a), sinf(a));
                const glm::vec2 tail = dest + d * (radius + 14.0f), tip = dest + d * (radius - 8.0f);
                drawLineQuad(tail.x, tail.y, tip.x, tip.y, 2.0f, cr, cg, cb, 0.80f * rise);
                for (int sgn : {-1, 1}) {
                    const glm::vec2 perp(-d.y, d.x);
                    const glm::vec2 wing = tip + d * 7.0f + perp * (float)sgn * 5.0f;
                    drawLineQuad(wing.x, wing.y, tip.x, tip.y, 1.8f, 1.0f, 1.0f, 1.0f, 0.75f * rise);
                }
            }
            drawLineQuad(dest.x - 4.0f, dest.y, dest.x + 4.0f, dest.y, 1.0f, cr, cg, cb, 0.55f * rise);
            drawLineQuad(dest.x, dest.y - 4.0f, dest.x, dest.y + 4.0f, 1.0f, cr, cg, cb, 0.55f * rise);
        }
        if (st == Monster::GRAVIS_GLIDE_RUN) {
            // Streaks behind the gliding body.
            for (int k = 0; k < 4; ++k) {
                const float lag = 26.0f + 22.0f * k;
                const glm::vec2 a = glm::vec2(x, y) - aim * lag + side * (float)(k - 1.5f) * 9.0f;
                const glm::vec2 b = a - aim * (30.0f + 8.0f * k);
                drawLineQuad(b.x, b.y, a.x, a.y, 2.2f, lr, lg, lb, 0.55f - 0.10f * k);
            }
        }
        if (burst >= 0.0f) {
            const float fr = std::clamp(burst / Monster::GRAVIS_GLIDE_BURST_TIME, 0.0f, 1.0f);
            // The burst ring and its outward ticks.
            DrawEnemyArc(dest.x, dest.y, radius * (0.55f + 0.45f * std::min(1.0f, burst / 0.10f)),
                         0.0f, 6.2831853f, 64, 1.0f, 1.0f, 1.0f, 1.0f - fr, 3.2f, false);
            for (int k = 0; k < 12; ++k) {
                const float a = k * 0.5235988f + 0.2f;
                const glm::vec2 d(cosf(a), sinf(a));
                const glm::vec2 p0 = dest + d * (radius * 0.35f + 90.0f * fr);
                const glm::vec2 p1 = dest + d * (radius * 0.70f + 130.0f * fr);
                drawLineQuad(p0.x, p0.y, p1.x, p1.y, 2.0f, lr, lg, lb, (1.0f - fr) * 0.85f);
            }
        }
    }
}

static void DrawQuasarFall(const Monster* m, float cr, float cg, float cb) {
    if (!m->QuasarFalling()) return;
    const float age = m->quasarTimer;
    const float radius = Monster::QUASAR_FALL_RADIUS;
    auto diamond = [&](glm::vec2 p, float size, float alpha, float angle = 0.0f) {
        // A faceted chunk with a white spine, rather than a small round node.
        const glm::vec2 shape[6] = {{0,-1.5f}, {0.65f,-0.3f}, {0.48f,0.6f},
                                    {-0.1f,1.4f}, {-0.65f,0.25f}, {-0.35f,-0.5f}};
        glm::vec2 v[6];
        for (int k = 0; k < 6; ++k) {
            const glm::vec2 q = shape[k] * size;
            v[k] = p + glm::vec2(q.x * cosf(angle) - q.y * sinf(angle),
                                 q.x * sinf(angle) + q.y * cosf(angle));
        }
        for (int k = 0; k < 6; ++k) {
            const auto a = v[k], b = v[(k + 1) % 6];
            drawLineQuad(a.x, a.y, b.x, b.y, 2.0f, cr, cg, cb, alpha);
        }
        drawLineQuad(v[0].x, v[0].y, v[3].x, v[3].y, 1.8f, 1.0f, 1.0f, 1.0f, alpha);
        drawLineQuad(v[1].x, v[1].y, v[4].x, v[4].y, 1.0f, cr, cg, cb, alpha);
        drawCircle(p.x, p.y, size * 0.24f, 1.0f, 1.0f, 1.0f, alpha);
    };
    for (int i = 0; i < m->quasarFallFired; ++i) {
        if (!(m->quasarFallTargetsMask & (1u << i))) continue;
        const float impactAt = m->QuasarFallImpactTime(i);
        const float after = age - impactAt;
        const glm::vec2 target = m->quasarFallTargets[i];
        if (after < 0.0f) {
            const float progress = std::clamp((age - m->quasarFallLaunchTimes[i]) / Monster::QUASAR_FALL_WARNING, 0.0f, 1.0f);
            // Dark under-stroke keeps the unfilled danger disc legible on white windows.
            DrawEnemyArc(target.x, target.y, radius, 0.0f, 6.2831853f, 64,
                         0.0f, 0.0f, 0.0f, 0.75f, 5.0f, false);
            DrawEnemyArc(target.x, target.y, radius, 0.0f, 6.2831853f, 64,
                         cr, cg, cb, 0.65f, 1.5f, false);
            if (progress > 0.0f)
                DrawEnemyArc(target.x, target.y, radius, -1.5707963f,
                             progress * 6.2831853f, 64,
                             1.0f, 1.0f, 1.0f, 0.85f, 2.2f, false);
            for (int k = 0; k < 4; ++k) {
                const float a = k * 1.5707963f;
                const glm::vec2 direction(cosf(a), sinf(a));
                const glm::vec2 side(-direction.y, direction.x);
                const glm::vec2 tip = target + direction * (radius - 19.0f);
                const glm::vec2 tail = target + direction * (radius - 7.0f);
                drawLineQuad(tail.x, tail.y, tip.x, tip.y,
                             2.0f, cr, cg, cb, 0.80f);
                for (int s : {-1, 1}) {
                    const glm::vec2 wing = tip + direction * 6.0f + side * (float)s * 4.0f;
                    drawLineQuad(wing.x, wing.y, tip.x, tip.y,
                                 1.8f, 1.0f, 1.0f, 1.0f, 0.65f + 0.30f * progress);
                }
            }
            // A small cross pins the ground point; it cannot look like another falling node.
            drawLineQuad(target.x - 3.0f, target.y, target.x + 3.0f, target.y,
                         1.0f, cr, cg, cb, 0.55f);
            drawLineQuad(target.x, target.y - 3.0f, target.x, target.y + 3.0f,
                         1.0f, cr, cg, cb, 0.55f);
        }
        if (after >= 0.0f) {
            if (after >= Monster::QUASAR_FALL_IMPACT_FX) continue;
            if (after < 0.095f)
                DrawEnemyArc(target.x, target.y, radius, 0.0f, 6.2831853f, 64,
                             1.0f, 1.0f, 1.0f, 1.0f - after / 0.095f, 3.0f, false);
            for (int k = 0; k < 6; ++k) {
                const float a = k * 1.0471976f + 0.4f + i * 0.37f;
                const float d = 7.0f + 175.0f * after;
                if (after < 0.10f)
                    drawLineQuad(target.x + cosf(a) * 5.0f, target.y + sinf(a) * 5.0f,
                                 target.x + cosf(a + 0.12f) * 43.0f, target.y + sinf(a + 0.12f) * 43.0f,
                                 1.8f, 1.0f, 1.0f, 1.0f, (1.0f - after / 0.10f) * 0.9f);
                diamond(target + glm::vec2(cosf(a), sinf(a)) * d,
                        5.5f * (1.0f - after / Monster::QUASAR_FALL_IMPACT_FX),
                        1.0f - after / Monster::QUASAR_FALL_IMPACT_FX);
            }
            continue;
        }
        const glm::vec2 p = m->QuasarFallPosition(i, age);
        const float flight = age - m->quasarFallLaunchTimes[i];
        const glm::vec2 previous = m->QuasarFallPosition(i, age - 0.01f);
        const glm::vec2 velocity = (p - previous) / 0.01f;
        const float history = std::min(0.12f, 65.0f / std::max(1.0f, glm::length(velocity)));
        // A sampled curved tail follows the path; one straight line hid all of its turning.
        for (int k = 0; k < 6; ++k) {
            const auto head = m->QuasarFallPosition(i, age - history * k / 6.0f);
            const auto tail = m->QuasarFallPosition(i, age - history * (k + 1) / 6.0f);
            const float fade = 1.0f - k / 6.0f;
            drawLineQuad(tail.x, tail.y, head.x, head.y, 4.8f * fade,
                         cr, cg, cb, 0.65f * fade);
            drawLineQuad(tail.x, tail.y, head.x, head.y, 1.0f * fade,
                         1.0f, 1.0f, 1.0f, 0.85f * fade);
        }
        const float angle = atan2f(velocity.y, velocity.x) + 1.5707963f + flight * 8.0f;
        const float size = 10.5f + 2.5f * Monster::QuasarEase(flight / Monster::QUASAR_FALL_WARNING);
        diamond(p, size, 0.98f, angle);
        const glm::vec2 side(cosf(angle), sinf(angle));
        diamond(p - side * 14.0f, 2.8f, 0.65f, angle);
        diamond(p + side * 12.0f, 2.2f, 0.65f, angle);

    }
}

// NORMA placeholder body (design pending): a square outline and centre point
// in ice blue. Phase 2 adds a turning inner square; entering it flashes white.
static void DrawNormaPlaceholder(const Monster* m, float base, float visualTime) {
    const float x = m->worldX, y = m->worldY;
    const float half = base * 1.3f;
    const float flash = std::clamp(m->normaPhaseFlash / norma::kPhaseFlashSeconds, 0.0f, 1.0f);
    const float hit = std::clamp(m->hitFlashTimer / MOB_HIT_FLASH_TIME, 0.0f, 1.0f);
    const float lift = std::max(flash, hit * 0.6f);
    const float r = m->color.r + (1.0f - m->color.r) * lift;
    const float g = m->color.g + (1.0f - m->color.g) * lift;
    const float b = m->color.b;
    const float pulse = 0.85f + 0.15f * sinf(visualTime * 2.0f);
    const float w = 2.6f + 2.0f * flash;
    drawLineQuad(x - half, y - half, x + half, y - half, w, r, g, b, pulse);
    drawLineQuad(x + half, y - half, x + half, y + half, w, r, g, b, pulse);
    drawLineQuad(x + half, y + half, x - half, y + half, w, r, g, b, pulse);
    drawLineQuad(x - half, y + half, x - half, y - half, w, r, g, b, pulse);
    if (m->normaPhase == 2) {
        const float inner = half * 0.55f;
        float px[4], py[4];
        for (int k = 0; k < 4; ++k) {
            const float a = m->normaVisualAngle + 0.7853982f + 1.5707963f * (float)k;
            px[k] = x + cosf(a) * inner * 1.4142f;
            py[k] = y + sinf(a) * inner * 1.4142f;
        }
        for (int k = 0; k < 4; ++k) {
            const int n = (k + 1) & 3;
            drawLineQuad(px[k], py[k], px[n], py[n], 1.6f, r, g, b, 0.8f);
        }
    }
    drawCircle(x, y, base * 0.10f, r, g, b, 0.95f);
}

void drawMob(const Monster* m, float visualTime) {
    if (!m) return;
    if (!m->alive) {
        m->DrawDeathEffect(drawLineQuad, drawCircle,
            [](float x, float y, float radius, float phase,
               float r, float g, float b, float alpha) {
                DrawFilledHexagon(x, y, radius, r, g, b, alpha, phase);
            });
        return;
    }
    MarkMobSeen(m->kind);
    if (m->kind == MobKind::SWARM) MarkMobSeenId(CM_SWARM);
    if (m->kind == MobKind::GRAVIS) MarkMobSeenId(CM_GRAVIS);
    if (m->kind == MobKind::QUASAR) MarkMobSeenId(CM_QUASAR);
    if (m->kind == MobKind::GIMBAL) MarkMobSeenId(CM_GIMBAL);
    if (m->kind == MobKind::REGULUS) MarkMobSeenId(CM_REGULUS);
    if (m->kind == MobKind::MAGNETAR) MarkMobSeenId(CM_MAGNETAR);
    float base = (m->summoned ? 28.0f : 18.0f) * m->sizeScale;
    if (visualTime < 0.0f) visualTime = (float)glfwGetTime();
    if (m->kind != MobKind::REGULUS && m->regulusShield.hp > 0.0f) {
        DrawAegisMark(m->worldX, m->worldY, base * AegisMarkExtent(m->kind),
                      m->regulusShield.hp / std::max(1.0f, m->regulusShield.maxHp),
                      std::clamp(m->regulusShield.flashTimer / MOB_HIT_FLASH_TIME,
                                 0.0f, 1.0f),
                      visualTime * kAegisSpin, m->regulusShield.grantTimer);
    }
    if (m->kind == MobKind::NORMA) {
        DrawNormaPlaceholder(m, base, visualTime);
    } else if (m->kind == MobKind::GENESIS) {
        DrawGenesisLauncher(m, base, visualTime);
    } else if (m->kind == MobKind::GRAVIS) {
        // Gravis: heavy planetary controller with a visible gravity boundary.
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        ApplyMobStyleTint(cr, cg, cb);
        const float x = m->worldX, y = m->worldY;
        const float phase = m->gravisVisualAngle;
        const float pulse = 0.5f + 0.5f * sinf(visualTime * 2.4f);

        if (!g_SuppressMobSeen) {
            // Time-dilation field: the boundary plus two counter-rotating
            // partial arcs. As the player's influence rises the arcs close in
            // on each other and sharpen, so the slowdown can be read off them.
            const float fieldR = GravisField::RADIUS;
            const float infl = GravisField::g_playerInfluence;
            const float fa = m->GravisFieldActive() ? 1.0f : 0.30f;   // dimmed while a pattern runs
            const float arcAlpha = (0.26f + 0.32f * infl) * fa;
            const float gap = 0.20f * (1.0f - 0.6f * infl);
            // A dark under-stroke keeps the boundary readable on bright windows.
            DrawEnemyArc(x, y, fieldR, 0.0f, 6.2831853f, 96,
                         0.0f, 0.0f, 0.0f, 0.40f * fa, 3.6f, false);
            DrawEnemyArc(x, y, fieldR, 0.0f, 6.2831853f, 96,
                         cr, cg, cb, (0.50f + 0.18f * infl) * fa, 1.8f, false);
            DrawEnemyArc(x, y, fieldR * 0.92f, visualTime * 0.16f, 3.6f, 36,
                         cr, cg, cb, arcAlpha, 1.0f, false);
            DrawEnemyArc(x, y, fieldR * (0.92f - gap), -visualTime * 0.22f + 2.1f, 3.6f, 32,
                         cr, cg, cb, arcAlpha, 1.0f, false);
            DrawEnemyArc(x, y, fieldR * (0.92f - 2.0f * gap), visualTime * 0.30f + 4.2f, 3.6f, 28,
                         cr, cg, cb, arcAlpha * 0.8f, 0.9f, false);
        }

        float bodyR = cr, bodyG = cg, bodyB = cb;
        ApplyMobHitFlash(bodyR, bodyG, bodyB, m->hitFlashTimer);
        // Compression glide: the nodes gather toward the core and the body
        // flattens along the glide axis.
        const float comp = m->GravisCompress();
        const glm::vec2 gaim = m->gravisGlideAim;
        const glm::vec2 gperp(-gaim.y, gaim.x);
        auto gshape = [&](float px, float py) {
            const float rx = px - x, ry = py - y;
            const float gather = 1.0f - 0.55f * comp;
            const float along = (rx * gaim.x + ry * gaim.y) * gather * (1.0f + 0.9f * comp);
            const float across = (rx * gperp.x + ry * gperp.y) * gather * (1.0f - 0.35f * comp);
            return glm::vec2(x + gaim.x * along + gperp.x * across,
                             y + gaim.y * along + gperp.y * across);
        };
        EnemyNodeAnchor nodes[6];
        const float orbitR = base * 1.12f;
        for (int k = 0; k < 6; ++k) {
            const float a = phase + (float)k * 1.0471976f;
            const glm::vec2 p = gshape(x + cosf(a) * orbitR, y + sinf(a) * orbitR);
            nodes[k] = { p.x, p.y,
                         base * 0.13f, 0.82f + 0.12f * sinf(visualTime * 3.0f + k) };
            const int next = (k + 1) % 6;
            const float na = phase + (float)next * 1.0471976f;
            const glm::vec2 q = gshape(x + cosf(na) * orbitR, y + sinf(na) * orbitR);
            drawLineQuad(nodes[k].x, nodes[k].y, q.x, q.y,
                         1.15f, bodyR, bodyG, bodyB, 0.54f);
            drawLineQuad(x, y, nodes[k].x, nodes[k].y,
                         0.72f, bodyR, bodyG, bodyB, 0.12f);
        }
        for (const auto& node : nodes)
            DrawEnemyNode(node, bodyR, bodyG, bodyB, 0.94f);
        DrawEliteFrame(x, y, base * (1.0f - 0.45f * comp), phase, bodyR, bodyG, bodyB, 0.40f);

        // A tilted inner orbit precesses against the hexagon. Its satellite
        // dims on the far half so the orbit reads as depth, not a flat ring.
        const float stretch = m->GravisOrbitStretch();
        float tilt = -phase * 0.42f + 0.49f;
        if (stretch > 0.0f) {
            // The orbit is symmetric over pi: swing it onto the throw axis.
            float gap = atan2f(m->gravisAim.y, m->gravisAim.x) - tilt;
            gap = std::remainder(gap, 6.2831853f);
            gap -= 3.1415927f * floorf(gap / 3.1415927f + 0.5f);
            tilt += gap * (m->gravisSling == Monster::GRAVIS_SLING_PREP ? 0.5f * stretch : stretch);
        }
        const float tiltX = cosf(tilt), tiltY = sinf(tilt);
        const float orbitMajor = base * 0.86f * (1.0f + 1.1f * stretch) * (1.0f - 0.6f * comp);
        const float orbitMinor = base * 0.30f * (1.0f + 0.3f * stretch) * (1.0f - 0.4f * comp);
        const float liteR = bodyR + (1.0f - bodyR) * 0.45f;
        const float liteG = bodyG + (1.0f - bodyG) * 0.45f;
        const float liteB = bodyB + (1.0f - bodyB) * 0.45f;
        DrawQuasarEllipseArc(x, y, tiltX, tiltY, orbitMajor, orbitMinor,
                             0.0f, 6.2831853f, 32,
                             liteR, liteG, liteB, 0.40f, 0.85f);
        // While the sling is away the satellites are the thrown masses.
        const int satCount = m->GravisSlinging() ? Monster::GRAVIS_SLING_COUNT : 1;
        auto satAt = [&](int k, float& depthOut) {
            const float a = m->gravisOrbitPhase + (float)k * (6.2831853f / (float)Monster::GRAVIS_SLING_COUNT);
            const float dp = sinf(a);
            depthOut = dp;
            return EnemyNodeAnchor{ x - tiltY * cosf(a) * orbitMajor + tiltX * dp * orbitMinor,
                                    y + tiltX * cosf(a) * orbitMajor + tiltY * dp * orbitMinor,
                                    base * 0.085f, 0.70f + 0.30f * (0.5f + 0.5f * dp) };
        };
        for (int k = 0; k < satCount && !m->GravisSatAway(k); ++k) {
            float dp = 0.0f;
            const EnemyNodeAnchor sn = satAt(k, dp);
            if (dp < 0.0f) DrawEnemyNode(sn, 1.0f, 1.0f, 1.0f, 0.55f);
        }

        drawCircle(x, y, base * 0.58f, bodyR, bodyG, bodyB,
                   0.18f + pulse * 0.08f);
        DrawGenesisCore(x, y, base * 0.34f,
                        bodyR, bodyG, bodyB, 0.96f, false);
        // Elite inner contour: a thin hexagon outline around the core.
        DrawEnemyArc(x, y, base * 0.52f, phase * 1.3f, 6.2831853f, 6,
                     liteR, liteG, liteB, 0.50f, 0.8f, false);
        for (int k = 0; k < satCount && !m->GravisSatAway(k); ++k) {
            float dp = 0.0f;
            const EnemyNodeAnchor sn = satAt(k, dp);
            if (dp >= 0.0f) DrawEnemyNode(sn, 1.0f, 1.0f, 1.0f, 0.95f);
        }
        DrawGravisElite(m, base, bodyR, bodyG, bodyB);
    } else if (m->kind == MobKind::QUASAR) {
        // Quasar: a compact accretion instrument with a fixed polar firing axis.
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        ApplyMobStyleTint(cr, cg, cb);
        const float x = m->worldX, y = m->worldY;
        const float ax = m->quasarAimX, ay = m->quasarAimY;
        const float charge = m->quasarState == 1
            ? 0.22f
            : (m->quasarState == 2
                ? std::min(1.0f, m->quasarTimer / Monster::QUASAR_LOCK_TIME)
                : (m->quasarState == 3 ? 1.0f : 0.0f));

        // Damage-band telegraph. Everything drawn here sits exactly on the
        // band the beam will hurt (QUASAR_BEAM_RADIUS either side of the
        // axis), from the wind-up through the whole fire window:
        //   wind-up  the band follows the player live, edges fading in
        //   lock     the band freezes; edges go hard and a fill grows from
        //            the axis out to the edges, full at the moment of fire
        //   fire     the beam fills the band exactly, edges stay hard
        // The band is also darkened underneath and each edge is three layers
        // (dark under-stroke, beam-colored line, white core) so it reads on
        // bright, dark and busy windows alike; light alone vanishes on white.
        const float beamR = Monster::QUASAR_BEAM_RADIUS;
        const float pre = m->QuasarPrecharge();
        auto bandEdges = [&](float x0, float y0, float x1, float y1,
                             float alpha, float width) {
            for (int e = -1; e <= 1; e += 2) {
                const float ox = -ay * beamR * (float)e;
                const float oy = ax * beamR * (float)e;
                drawLineQuad(x0 + ox, y0 + oy, x1 + ox, y1 + oy, width + 4.0f,
                             0.0f, 0.0f, 0.0f, 0.70f * alpha);
                drawLineQuad(x0 + ox, y0 + oy, x1 + ox, y1 + oy, width,
                             cr, cg, cb, alpha);
                drawLineQuad(x0 + ox, y0 + oy, x1 + ox, y1 + oy, width * 0.45f,
                             1.0f, 1.0f, 1.0f, alpha);
            }
        };
        // Darkens the whole band so it stands out from bright backgrounds.
        auto bandShade = [&](float x0, float y0, float x1, float y1, float alpha) {
            drawLineQuad(x0, y0, x1, y1, beamR * 2.0f, 0.0f, 0.0f, 0.0f, alpha);
        };
        if ((m->quasarState == 0 && pre > 0.0f) ||
            (m->quasarState >= 1 && m->quasarState <= 3)) {
            const float beamLen = 6000.0f;
            const float growT = m->quasarState == 3
                ? std::min(1.0f, m->quasarTimer / 0.70f) : 1.0f;
            const float reachT = m->quasarState == 3
                ? growT * growT * growT : 1.0f;
            const float reach = beamLen * reachT;
            const float x0 = x - ax * reach;
            const float y0 = y - ay * reach;
            const float x1 = x + ax * reach;
            const float y1 = y + ay * reach;
            if (m->quasarState == 3) {
                // The beam fills the damage band exactly: body (2r), bright
                // inner column, white core, and the hard band edges.
                const float pulse = 0.82f + 0.18f * sinf(m->quasarTimer * 70.0f);
                const float swell = 0.35f + 0.65f * growT;      // inner layers only
                bandShade(x0, y0, x1, y1, 0.30f);
                drawLineQuad(x0, y0, x1, y1, beamR * 2.0f, cr, cg, cb, 0.40f * pulse);
                drawLineQuad(x0, y0, x1, y1, beamR * 0.9f * swell, cr, cg, cb, 0.62f * pulse);
                drawLineQuad(x0, y0, x1, y1, beamR * 0.30f * swell,
                             1.0f, 1.0f, 1.0f, 0.95f);
                bandEdges(x0, y0, x1, y1, 0.95f, 3.0f);

                // A physical release ring marks the acceleration phase without
                // using a full-screen or core flash.
                if (growT > 0.28f && growT < 1.0f) {
                    const float releaseT = (growT - 0.28f) / 0.72f;
                    const float ringR = base * (1.0f + releaseT * 1.9f);
                    DrawEnemyArc(x, y, ringR, 0.0f, 6.28318531f, 32,
                                 cr, cg, cb, (1.0f - releaseT) * 0.52f,
                                 2.2f, false);
                    DrawEnemyArc(x, y, ringR * 0.82f, 0.0f, 6.28318531f, 28,
                                 cr, cg, cb, (1.0f - releaseT) * 0.13f,
                                 1.1f, false);
                }
            } else if (m->quasarState == 0) {
                // Wind-up: the band tracks the player; edges fade in.
                const float fade = std::min(1.0f, pre * 1.6f);
                bandShade(x0, y0, x1, y1, 0.10f * fade);
                drawLineQuad(x0, y0, x1, y1, 1.6f, cr, cg, cb, 0.55f * fade);
                bandEdges(x0, y0, x1, y1, 0.45f + 0.35f * fade, 2.0f);
            } else {
                // Lock: frozen band, hard edges, and a fill growing from the
                // axis out to the edges as the countdown to fire.
                const float lockPulse = 0.80f + 0.20f * sinf(visualTime * 18.0f);
                bandShade(x0, y0, x1, y1, 0.22f);
                drawLineQuad(x0, y0, x1, y1, beamR * 2.0f * charge, cr, cg, cb,
                             0.18f + 0.22f * charge);
                drawLineQuad(x0, y0, x1, y1, 2.0f, 1.0f, 1.0f, 1.0f, 0.70f * lockPulse);
                bandEdges(x0, y0, x1, y1, (0.85f + 0.15f * charge) * lockPulse, 3.0f);
            }
        }

        DrawQuasarBody(m, base, visualTime, charge, cr, cg, cb);
        DrawQuasarFall(m, cr, cg, cb);
    } else if (m->kind == MobKind::GIMBAL) {
        // Gimbal (clamp): two facing ( ) brackets hold a diamond lens on a pin
        // axis. The lens flips about that axis (gimbalVisualAngle), faster and
        // faster while charging, as the brackets close in; it fires face-on
        // and the brackets snap open. Scope keeps the round ring language.
        float bodyR = m->color.r, bodyG = m->color.g, bodyB = m->color.b;
        ApplyMobStyleTint(bodyR, bodyG, bodyB);
        ApplyMobHitFlash(bodyR, bodyG, bodyB, m->hitFlashTimer);
        const float x = m->worldX, y = m->worldY;
        const float offset = (float)((size_t)m % 628) * 0.01f;
        const float clampSpin = visualTime * 0.30f + offset;
        const float flip = m->gimbalVisualAngle + offset;
        const bool burst = m->gimbalBursting;
        const float charge = m->gimbalCharging
            ? std::min(1.0f, m->gimbalChargeAngle /
                (6.2831853f * Monster::GIMBAL_CHARGE_ROTATIONS))
            : (burst ? 1.0f : 0.0f);
        const float pulse = burst ? 0.5f + 0.5f * sinf(m->gimbalBurstTimer * 40.0f) : 0.0f;
        const float snapOpen = burst
            ? std::max(0.0f, 1.0f - m->gimbalBurstTimer / 0.35f) : 0.0f;
        const float R = base * 1.15f;
        const float ca = cosf(clampSpin), sa = sinf(clampSpin);
        auto L = [&](float lx, float ly, float& ox, float& oy) {
            ox = x + lx * ca - ly * sa;
            oy = y + lx * sa + ly * ca;
        };
        const float liteR = bodyR + (1.0f - bodyR) * 0.45f;
        const float liteG = bodyG + (1.0f - bodyG) * 0.45f;
        const float liteB = bodyB + (1.0f - bodyB) * 0.45f;

        // Brackets: double contour with flat cut ends and a pin node.
        const float gap = R * (0.48f - 0.16f * charge * (burst ? 0.0f : 1.0f) + 0.16f * snapOpen);
        for (int s = -1; s <= 1; s += 2) {
            const float cx = gap * (float)s;
            float px = 0, py = 0, qx = 0, qy = 0;
            for (int i = 0; i < 16; ++i) {
                const float t0 = -1.15f + 2.30f * (float)i / 16.0f;
                const float t1 = -1.15f + 2.30f * (float)(i + 1) / 16.0f;
                L(cx + (float)s * R * 0.50f * cosf(t0), R * 0.95f * sinf(t0), px, py);
                L(cx + (float)s * R * 0.50f * cosf(t1), R * 0.95f * sinf(t1), qx, qy);
                drawLineQuad(px, py, qx, qy, 1.6f, bodyR, bodyG, bodyB, 0.90f + 0.10f * charge);
            }
            for (int i = 0; i < 14; ++i) {
                const float t0 = -1.0f + 2.0f * (float)i / 14.0f;
                const float t1 = -1.0f + 2.0f * (float)(i + 1) / 14.0f;
                L(cx + (float)s * R * 0.43f * cosf(t0), R * 0.82f * sinf(t0), px, py);
                L(cx + (float)s * R * 0.43f * cosf(t1), R * 0.82f * sinf(t1), qx, qy);
                drawLineQuad(px, py, qx, qy, 0.85f, bodyR, bodyG, bodyB, 0.40f + 0.25f * charge);
            }
            for (int e = -1; e <= 1; e += 2) {
                L(cx + (float)s * R * 0.50f * cosf(1.15f), R * 0.95f * sinf(1.15f) * (float)e, px, py);
                L(cx + (float)s * R * 0.43f * cosf(1.0f), R * 0.82f * sinf(1.0f) * (float)e, qx, qy);
                drawLineQuad(px, py, qx, qy, 1.2f, bodyR, bodyG, bodyB, 0.85f);
            }
            L(cx + (float)s * R * 0.50f, 0.0f, px, py);
            DrawEnemyNode({px, py, base * (0.14f + 0.02f * charge),
                           0.82f + 0.18f * charge}, bodyR, bodyG, bodyB, 0.92f);
        }
        // Pin axis between the brackets.
        {
            float ax, ay, bx, by;
            L(-gap - R * 0.44f, 0.0f, ax, ay);
            L(gap + R * 0.44f, 0.0f, bx, by);
            drawLineQuad(ax, ay, bx, by, 0.8f, bodyR, bodyG, bodyB, 0.22f + 0.20f * charge);
        }
        // Diamond lens flipping about the pin axis: its height follows
        // |cos(flip)|, so it reads edge-on <-> face-on.
        {
            const float hw = R * 0.50f;
            const float hh = R * 0.58f * (0.12f + 0.88f * fabsf(cosf(flip)));
            const float lr = burst ? 1.0f : bodyR + (liteR - bodyR) * charge;
            const float lg = burst ? 1.0f : bodyG + (liteG - bodyG) * charge;
            const float lb = burst ? 1.0f : bodyB + (liteB - bodyB) * charge;
            const float vx[4] = { hw, 0.0f, -hw, 0.0f }, vy[4] = { 0.0f, hh, 0.0f, -hh };
            for (int k = 0; k < 4; ++k) {
                const int n = (k + 1) % 4;
                float ax, ay, bx, by;
                L(vx[k], vy[k], ax, ay);  L(vx[n], vy[n], bx, by);
                drawLineQuad(ax, ay, bx, by, 1.5f, lr, lg, lb, 0.95f);
                L(vx[k] * 0.55f, vy[k] * 0.55f, ax, ay);  L(vx[n] * 0.55f, vy[n] * 0.55f, bx, by);
                drawLineQuad(ax, ay, bx, by, 0.85f, bodyR, bodyG, bodyB, 0.40f + 0.30f * charge);
            }
        }
        if (burst) {
            drawCircle(x, y, base * (1.5f + 0.25f * pulse),
                       1.0f, 1.0f, 1.0f, 0.16f * pulse);
            DrawEnemyCore(x, y, base * (0.30f + 0.08f * pulse),
                          1.0f, 1.0f, 1.0f, 0.88f + 0.12f * pulse);
        } else {
            DrawEnemyCore(x, y, base * (0.26f + charge * 0.05f),
                          bodyR, bodyG, bodyB,
                          0.84f + 0.16f * charge, false);
        }
    } else if (m->kind == MobKind::REGULUS) {
        // Regulus: shield crest body inside four curved shield plates.
        float bodyR = m->color.r, bodyG = m->color.g, bodyB = m->color.b;
        ApplyMobStyleTint(bodyR, bodyG, bodyB);
        ApplyMobHitFlash(bodyR, bodyG, bodyB, m->hitFlashTimer);
        const float x = m->worldX, y = m->worldY;
        const float phase = m->regulusVisualAngle +
                            (float)((size_t)m % 628u) * 0.01f;
        const float ratio = m->regulusShield.hp /
                            std::max(1.0f, m->regulusShield.maxHp);
        const float regen = m->regulusShield.hp > 0.0f ? 1.0f
            : 1.0f - std::clamp(m->regulusShieldRegenTimer /
                                Monster::REGULUS_SHIELD_REGEN_DELAY, 0.0f, 1.0f);
        DrawRegulusPlates(x, y, base, visualTime * kAegisSpin, ratio, regen,
                          std::clamp(m->regulusShield.flashTimer / MOB_HIT_FLASH_TIME,
                                     0.0f, 1.0f));
        drawCircle(x, y, base * 0.85f, 0.40f, 0.84f, 1.0f, 0.06f);
        DrawRegulusCrest(x, y, base, phase, bodyR, bodyG, bodyB, visualTime);
    } else if (m->kind == MobKind::ANTARES) {
        DrawAntares(m, base);
    } else if (m->kind == MobKind::MAGNETAR) {
        DrawMagnetar(m, base, visualTime);
    } else if (m->kind == MobKind::SWARM) {
        // Swarm: a smaller triangular packet, visually subordinate to Rotor.
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        ApplyMobStyleTint(cr, cg, cb);
        ApplyMobHitFlash(cr, cg, cb, m->hitFlashTimer);
        float x = m->worldX, y = m->worldY;
        float phOff = (float)((size_t)m % 628) * 0.01f;
        float t = visualTime;
        float phase = t * 0.62f + phOff - 1.5708f;
        float radius = base * 0.92f;
        if (m->thrownPhase == 1)
            DrawThrownBlastZone(m->thrownTargetX, m->thrownTargetY, 1.0f,
                                std::min(1.0f, m->thrownTimer /
                                               std::max(0.01f, m->thrownDuration)));
        if (m->thrownPhase != 0) {
            float ghostR = m->color.r, ghostG = m->color.g, ghostB = m->color.b;
            ApplyMobStyleTint(ghostR, ghostG, ghostB);
            DrawThrownSwarmGhosts(m, base, ghostR, ghostG, ghostB);
            // In flight the body spins with its speed; on landing it keeps
            // that angle and rattles (Antares stun) as it settles.
            phase = m->thrownSpin;
            if (m->thrownPhase == 1) {
                radius *= 1.15f;
            } else {
                // Landing shockwave: one ring out to the damage radius.
                if (m->thrownTimer < 0.22f) {
                    const float k = m->thrownTimer / 0.22f;
                    DrawEnemyArc(x, y, Monster::THROWN_SWARM_BLAST_RADIUS * (0.25f + 0.75f * k),
                                 0.0f, 6.2831853f, 28, 1.0f, 0.30f, 0.24f,
                                 0.75f * (1.0f - k), 2.0f, false);
                }
                const float left = 1.0f - std::min(1.0f,
                    m->thrownTimer / Monster::THROWN_SWARM_STUN);
                x += sinf(m->thrownTimer * 71.0f) * 3.0f * left;
                y += cosf(m->thrownTimer * 53.0f) * 3.0f * left;
            }
        }
        EnemyNodeAnchor nodes[3];
        for (int k = 0; k < 3; ++k) {
            const float a = phase + (float)k * 2.0943951f;
            const float jitter = base * 0.05f * sinf(t * 3.0f + phOff * 7.0f + k);
            nodes[k] = { x + cosf(a) * (radius + jitter),
                         y + sinf(a) * (radius + jitter),
                         base * 0.15f,
                         0.82f + 0.12f * (0.5f + 0.5f * sinf(t * 4.0f + k)) };
        }
        float vx[3], vy[3];
        for (int k = 0; k < 3; ++k) { vx[k] = nodes[k].x; vy[k] = nodes[k].y; }
        DrawEnemyWirePolygon(vx, vy, 3, cr, cg, cb, 0.68f, 0.08f, 0.75f);
        for (int k = 0; k < 3; ++k)
            DrawEnemyNode(nodes[k], cr, cg, cb, 0.88f);
        const float swarmCoreA = m->thrownPhase == 2
            ? 0.62f + 0.30f * (0.5f + 0.5f * sinf(m->thrownTimer * 40.0f)) : 0.94f;
        DrawEnemyCore(x, y, base * 0.28f, cr, cg, cb, swarmCoreA, true);
    } else {
        // Rotor (ROTOR): one square frame with four shared star anchors.
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        ApplyMobStyleTint(cr, cg, cb);
        ApplyMobHitFlash(cr, cg, cb, m->hitFlashTimer);
        float x = m->worldX, y = m->worldY;
        float phOff = (float)((size_t)m % 628) * 0.01f;
        const float half = base * 0.78f;
        float angle = visualTime * 0.78f + phOff;
        if(m->genesisRotorPhase>0) angle=m->thrownSpin;
        else if(m->genesisThrown && m->thrownDuration>0) {
            const float chaseAge=std::max(0.0f,m->thrownTimer-0.25f);
            angle=m->thrownSpin+0.78f*chaseAge*Monster::GenesisSmooth(chaseAge/0.3f);
        }
        if(m->genesisRotorPhase==2) {
            // A short history-based, solid-line tail: no bloom or gradient field.
            for(int i=0;i<2;++i) {
                const glm::vec2 a=m->antaresTrail[i*2];
                const glm::vec2 b=m->antaresTrail[i*2+2];
                drawLineQuad(a.x,a.y,b.x,b.y,1.6f,cr,cg,cb,0.65f);
            }
        }
        float vx[4], vy[4];
        EnemyNodeAnchor nodes[4];
        for (int k = 0; k < 4; ++k) {
            const float a = angle + 0.7853982f + (float)k * 1.5707963f;
            vx[k] = x + cosf(a) * half * 1.4142f;
            vy[k] = y + sinf(a) * half * 1.4142f;
            nodes[k] = { vx[k], vy[k], base * 0.16f, 0.92f };
        }
        if(m->genesisThrown) {
            if(m->genesisRotorPhase==2)
                DrawThrownBlastZone(m->thrownTargetX,m->thrownTargetY,1.0f,
                    std::clamp(m->thrownTimer/m->thrownDuration,0.0f,1.0f),Monster::GENESIS_ROTOR_BLAST_RADIUS);
            if(m->genesisRotorPhase==3 && m->genesisRotorDetonated) {
                const float u=std::clamp(m->thrownTimer/0.25f,0.0f,1.0f);
                const float radius=Monster::GENESIS_ROTOR_BLAST_RADIUS;
                drawCircle(x,y,radius,cr,cg,cb,0.07f*(1.0f-u));
                DrawEnemyArc(x,y,radius,0,6.2831853f,48,cr,cg,cb,0.9f*(1.0f-u),1.8f,false);
                DrawEnemyArc(x,y,radius*(0.25f+0.75f*u),0,6.2831853f,48,
                             1.0f,0.70f,0.30f,1.0f-u,2.0f,false);
            }
        }
        const float build=m->genesisRotorPhase==1 ?
            std::clamp(m->thrownTimer/Monster::GENESIS_ROTOR_FORM,0.0f,1.0f) : 1.0f;
        DrawEnemyWirePolygon(vx, vy, 4, cr, cg, cb, 0.80f*build, 0.08f*build, 0.85f);
        for (int k = 0; k < 4; ++k) DrawEnemyNode(nodes[k], cr, cg, cb, 0.94f*build);
        DrawEnemyCore(x, y, base * 0.29f, cr, cg, cb, 0.96f*build);
    }
    if (m->burnTimer > 0.0f) {
        float pulse = 0.35f + 0.25f * sinf(visualTime * 14.0f);
        drawCircle(m->worldX, m->worldY, base * 1.2f,
                   1.0f, 0.55f, 0.12f, pulse);
    }
}

void drawRangedMob(const RangedMob* r, float visualTime) {
    if (!r || !r->alive || r->deathScale <= 0.0f) return;
    if (visualTime < 0.0f) visualTime = (float)glfwGetTime();

    const float sc = r->deathScale;
    const float base = RangedMob::VISUAL_BASE_PX * sc;
    const float x = r->worldX;
    const float y = r->worldY;
    const float phaseOffset = (float)((size_t)r % 628) * 0.01f;
    const float chargeT = r->lensState == RangedMob::State::CHARGING
        ? std::min(r->chargeAngle /
                   (2.0f * 3.14159f * RangedMob::CHARGE_ROTATIONS), 1.0f)
        : (r->lensState == RangedMob::State::BURST ? 1.0f : 0.0f);
    float cr = r->color.r;
    float cg = r->color.g;
    float cb = r->color.b;
    ApplyMobHitFlash(cr, cg, cb, r->hitFlashTimer);
    if (r->regulusShield.hp > 0.0f) {
        DrawAegisMark(x, y, base * 1.70f,
                      r->regulusShield.hp / std::max(1.0f, r->regulusShield.maxHp),
                      std::clamp(r->regulusShield.flashTimer / MOB_HIT_FLASH_TIME,
                                 0.0f, 1.0f),
                      visualTime * kAegisSpin, r->regulusShield.grantTimer);
    }
    const float instrumentR = base * (1.26f + 0.12f * chargeT);
    const float phase = r->rotAngle + phaseOffset;
    const bool burst = r->lensState == RangedMob::State::BURST;

    // Scope: an observation instrument whose inner axis ring turns in depth
    // (its minor radius breathes) and tumbles faster while charging.
    DrawEnemyArc(x, y, instrumentR, phase, 5.45f, 44,
                 cr, cg, cb, 0.62f + 0.18f * chargeT, 0.90f, false);
    const float axis = -phase * 0.55f + 0.52f;
    const float axisX = cosf(axis), axisY = sinf(axis);
    const float ringMajor = instrumentR * 0.70f;
    const float squash = 0.16f + 0.84f * fabsf(cosf(phase * 1.6f));
    DrawQuasarEllipseArc(x, y, axisX, axisY, ringMajor, ringMajor * squash,
                         0.0f, 6.2831853f, 28,
                         cr, cg, cb, burst ? 0.22f : 0.46f + 0.30f * chargeT,
                         0.90f);
    EnemyNodeAnchor nodes[4];
    for (int i = 0; i < 2; ++i) {
        const float a = phase + (float)i * 3.1415927f;
        nodes[i] = { x + cosf(a) * instrumentR,
                     y + sinf(a) * instrumentR,
                     base * (0.13f + 0.02f * chargeT),
                     0.86f + 0.10f * sinf(visualTime * 5.0f + i) };
        const float s = i ? -1.0f : 1.0f;
        nodes[2 + i] = { x - axisY * ringMajor * s,
                         y + axisX * ringMajor * s,
                         base * 0.10f, 0.80f + 0.18f * chargeT };
    }
    for (int i = 0; i < 4; ++i)
        DrawEnemyNode(nodes[i], cr, cg, cb, 0.88f + 0.12f * chargeT);

    if (burst) {
        const float pulse = 0.5f + 0.5f * sinf(r->burstTimer * 40.0f);
        drawCircle(x, y, base * (1.5f + 0.25f * pulse),
                   1.0f, 1.0f, 1.0f, 0.16f * pulse * sc);
        DrawEnemyCore(x, y, base * (0.38f + 0.08f * pulse),
                      1.0f, 1.0f, 1.0f, (0.88f + 0.12f * pulse) * sc);
    } else {
        DrawEnemyCore(x, y, base * 0.32f, cr, cg, cb,
                      0.84f + 0.16f * chargeT);
    }
}

void drawCodexMobPreview(int codexId, float x, float y, float scale) {
    // Codex previews must not count as discoveries while reusing gameplay draw
    // code.  This keeps the preview silhouette and its animation in lockstep
    // with the live enemy renderer.
    const bool wasSuppressed = g_SuppressMobSeen;
    g_SuppressMobSeen = true;
    if (codexId == CM_SCOPE) {
        RangedMob preview(x, y, 1920, 1080);
        preview.rotAngle = (float)glfwGetTime() * RangedMob::IDLE_ROT;
        // RangedMob's gameplay base is 25.6px; normalize it to the Monster
        // base (18px) so the miniature preserves cross-enemy size ratios.
        preview.deathScale = scale / (RangedMob::VISUAL_BASE_PX / 18.0f);
        drawRangedMob(&preview);
    } else {
        MobKind kind = MobKind::ROTOR;
        switch (codexId) {
        case CM_GENESIS: kind = MobKind::GENESIS; break; // GENESIS
        case CM_SWARM:    kind = MobKind::SWARM;    break; // SWARM
        case CM_GRAVIS:  kind = MobKind::GRAVIS;  break;
        case CM_QUASAR:  kind = MobKind::QUASAR;  break; // QUASAR
        case CM_GIMBAL:  kind = MobKind::GIMBAL;  break;
        case CM_REGULUS: kind = MobKind::REGULUS; break;
        case CM_ANTARES: kind = MobKind::ANTARES; break;
        case CM_MAGNETAR: kind = MobKind::MAGNETAR; break;
        default: break;                                // ROTOR
        }
        Monster preview(x, y, 1.0f, 1.0f, false);
        preview.MakeKind(kind);
        preview.sizeScale = scale;
        const float t = (float)glfwGetTime();
        preview.hiveOrbitAngle = t * 0.36f;
        preview.gravisVisualAngle = t * 0.18f;
        preview.gravisOrbitPhase = t * 1.35f;
        preview.quasarVisualAngle = t * 0.24f;
        preview.quasarShellSpin = t * 2.2f;
        preview.gimbalVisualAngle = t * 0.24f;
        preview.regulusVisualAngle = t * 0.10f;
        preview.antaresVisualAngle = t * 0.5236f;
        preview.magnetarFacing = t * 0.4f;
        preview.magnetarVisualAngle = t * 0.72f;
        drawMob(&preview);
    }
    g_SuppressMobSeen = wasSuppressed;
}
