#include "EntityDraw.h"
#include "../Entity/MonsterManager.h"
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
        alpha = std::min(0.46f, alpha + openLift + 0.34f * pulseEase);
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
        radiusScale = 3.05f;
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
            const float rearScale = monster->kind == MobKind::GRAVIS ? 3.35f
                : (monster->kind == MobKind::QUASAR ? 3.05f : 2.55f);
            rearMarkers.push_back({monster->worldX, monster->worldY,
                                   base * rearScale});
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
    DrawBlackSightRear(m->worldX, m->worldY, base * 2.55f);
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

static void DrawGenesisBluntBlade(float cx, float cy, float angle,
                                  float inner, float outer, float width,
                                  float r, float g, float b, float alpha) {
    const float dx = cosf(angle), dy = sinf(angle);
    const float tx = -dy, ty = dx;
    const float shoulder = outer - width * 0.42f;
    const float rootHalf = width * 0.34f;
    const float bodyHalf = width * 0.62f;
    const float x0a = cx + dx * inner    + tx * rootHalf;
    const float y0a = cy + dy * inner    + ty * rootHalf;
    const float x0b = cx + dx * inner    - tx * rootHalf;
    const float y0b = cy + dy * inner    - ty * rootHalf;
    const float x1a = cx + dx * shoulder + tx * bodyHalf;
    const float y1a = cy + dy * shoulder + ty * bodyHalf;
    const float x1b = cx + dx * shoulder - tx * bodyHalf;
    const float y1b = cy + dy * shoulder - ty * bodyHalf;
    const float x2a = cx + dx * outer    + tx * bodyHalf;
    const float y2a = cy + dy * outer    + ty * bodyHalf;
    const float x2b = cx + dx * outer    - tx * bodyHalf;
    const float y2b = cy + dy * outer    - ty * bodyHalf;

    BatchTri(x0a, y0a, x1a, y1a, x1b, y1b, r, g, b, alpha);
    BatchTri(x0a, y0a, x1b, y1b, x0b, y0b, r, g, b, alpha);
    BatchTri(x1a, y1a, x2a, y2a, x2b, y2b, r, g, b, alpha);
    BatchTri(x1a, y1a, x2b, y2b, x1b, y1b, r, g, b, alpha);
    drawLineQuad(x0a, y0a, x1a, y1a, 0.90f, r, g, b, alpha * 0.92f);
    drawLineQuad(x1a, y1a, x2a, y2a, 0.90f, r, g, b, alpha * 0.92f);
    drawLineQuad(x2a, y2a, x2b, y2b, 1.05f, r, g, b, alpha * 0.92f);
    drawLineQuad(x2b, y2b, x1b, y1b, 0.90f, r, g, b, alpha * 0.92f);
    drawLineQuad(x1b, y1b, x0b, y0b, 0.90f, r, g, b, alpha * 0.92f);
}

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

static void DrawGenesisInnerMechanism(float cx, float cy, float base,
                                      float phase, float time,
                                      int hivePhase, float openFactor,
                                      float r, float g, float b,
                                      float alpha) {
    const float pi2 = 6.2831853f;
    const float pulse = 0.5f + 0.5f * sinf(time * 4.8f);
    const bool spawning = hivePhase == 1 || hivePhase == 2;
    const float sync = spawning ? (0.16f + 0.22f * openFactor) : 0.0f;
    const float prep = hivePhase == 1 ? openFactor : (hivePhase == 2 ? 1.0f : 0.0f);

    // Three broad shuttle blades replace circular inner rings. Their flat
    // caps and counter-rotation make the heavy station motion readable.
    const float bladePhase = phase * 0.45f + time * 0.72f;
    for (int i = 0; i < 3; ++i) {
        const float a = bladePhase + 2.0943951f * (float)i;
        DrawGenesisBluntBlade(cx, cy, a, base * 0.22f, base * 0.58f,
                              base * 0.13f, r, g, b,
                              alpha * (0.42f + 0.12f * pulse + 0.16f * prep));
    }

    // Four compact transfer blocks counter-rotate between the blades. They
    // add mechanical depth without returning to a ring-shaped silhouette.
    const float blockPhase = -time * 0.48f + phase * 0.18f;
    for (int i = 0; i < 4; ++i) {
        const float a = blockPhase + 1.5707963f * (float)i;
        const float radius = base * (0.39f + 0.06f * prep);
        const float bx = cx + cosf(a) * radius;
        const float by = cy + sinf(a) * radius;
        const float half = base * 0.055f;
        drawRect(bx - half, by - half, half * 2.0f, half * 2.0f,
                 r, g, b, alpha * (0.34f + 0.18f * prep));
        drawLineQuad(bx - half, by - half, bx + half, by + half,
                     0.75f, r, g, b, alpha * 0.72f);
    }

    // Rotating radial rails expose the central power transfer without
    // changing the two permanent outer egress openings.
    const float railPhase = -time * 1.05f + phase * 0.25f;
    for (int i = 0; i < 6; ++i) {
        const float a = railPhase + pi2 * (float)i / 6.0f;
        const float inner = base * 0.24f;
        const float outer = base * (0.55f + 0.20f * prep
                                  + 0.035f * sinf(time * 3.0f + i));
        drawLineQuad(cx + cosf(a) * inner, cy + sinf(a) * inner,
                     cx + cosf(a) * outer, cy + sinf(a) * outer,
                     0.72f, r, g, b, alpha * (0.22f + 0.16f * pulse
                                            + 0.14f * prep));
    }

    // During preparation, two aperture indicators sweep toward the fixed
    // opposite lanes. This gives a readable charge window before emission.
    if (prep > 0.0f) {
        const float laneAxis = phase + 0.5235988f;
        const float sweep = 0.22f + 0.42f * prep;
        const float laneRadius = base * (0.48f + 0.24f * prep);
        for (int lane = 0; lane < 2; ++lane) {
            const float a = laneAxis + (lane ? 3.1415926f : 0.0f);
            const float charge = alpha * (0.24f + 0.34f * prep
                                         + 0.08f * pulse);
            DrawEnemyArc(cx, cy, laneRadius, a - sweep * 0.5f, sweep,
                         8, r, g, b, charge, 1.15f, false);
        }
    }

    // Four small moving relays orbit the inner ring and brighten during the
    // opening/spawn phase, giving the summon cycle a visible heartbeat.
    for (int i = 0; i < 4; ++i) {
        const float a = time * (0.62f + i * 0.04f) + phase * 0.18f
                      + 1.5707963f * (float)i;
        EnemyNodeAnchor relay = {
            cx + cosf(a) * base * 0.58f,
            cy + sinf(a) * base * 0.58f,
            base * 0.045f,
            0.52f + 0.22f * pulse + sync
        };
        DrawEnemyNode(relay, r, g, b, alpha * 0.72f);
    }

    // The two fixed lanes are echoed inside the shell as paired rails. Their
    // direction rotates with the station, but their opposite layout never
    // changes between summon cycles.
    const float laneAxis = phase + 0.5235988f;
    for (int lane = 0; lane < 2; ++lane) {
        const float a = laneAxis + (lane ? 3.1415926f : 0.0f);
        const float dx = cosf(a), dy = sinf(a);
        const float tx = -dy, ty = dx;
        for (int side = -1; side <= 1; side += 2) {
            const float offset = base * 0.075f * (float)side;
            drawLineQuad(cx + dx * base * 0.28f + tx * offset,
                         cy + dy * base * 0.28f + ty * offset,
                         cx + dx * base * 0.78f + tx * offset,
                         cy + dy * base * 0.78f + ty * offset,
                         0.78f, r, g, b,
                         alpha * (0.30f + sync + 0.06f * pulse));
        }
    }
}

static void DrawGenesisStationRing(float cx, float cy, float base,
                                   float phase, float time,
                                   int hivePhase, float openFactor,
                                   float r, float g, float b, float alpha) {
    const float pi2 = 6.2831853f;
    const float step = pi2 / 6.0f;
    // Rotate the complete station as one rigid silhouette. Opposite edges
    // remain open as two parallel egress lanes while the inner mechanism
    // keeps its own relative counter-rotation.
    const float shellPhase = phase - 0.5235988f; // gap axis follows phase
    const float outerRadius = base * 1.52f;
    const float innerRadius = base * 0.62f;

    float outerX[6], outerY[6];
    float innerX[6], innerY[6];
    for (int k = 0; k < 6; ++k) {
        const float shellAngle = shellPhase + (float)k * step;
        outerX[k] = cx + cosf(shellAngle) * outerRadius;
        outerY[k] = cy + sinf(shellAngle) * outerRadius;
        const float innerAngle = phase + (float)k * step;
        innerX[k] = cx + cosf(innerAngle) * innerRadius;
        innerY[k] = cy + sinf(innerAngle) * innerRadius;
    }

    // Opposite edges 0 and 3 are permanently omitted, producing two
    // parallel openings that rotate together with the station.
    for (int k = 0; k < 6; ++k) {
        const int next = (k + 1) % 6;
        if (k == 0 || k == 3) continue;
        drawLineQuad(outerX[k], outerY[k], outerX[next], outerY[next],
                     1.55f, r, g, b, alpha * 0.90f);
    }

    // The inner scope is a continuously rotating observatory mechanism:
    // concentric dashed rings, a second offset ring, and instrument ticks.
    const float ringRadii[2] = { innerRadius, innerRadius * 0.72f };
    const int ringSegments[2] = { 36, 30 };
    for (int ringIndex = 0; ringIndex < 2; ++ringIndex) {
        const float rr = ringRadii[ringIndex];
        const int count = ringSegments[ringIndex];
        const float ringPhase = phase * (ringIndex == 0 ? 1.0f : -0.72f);
        for (int i = 0; i < count; ++i) {
            if ((i + ringIndex) % 3 == 1) continue;
            const float a0 = ringPhase + pi2 * (float)i / (float)count;
            const float a1 = ringPhase + pi2 * (float)(i + 1) / (float)count;
            drawLineQuad(cx + cosf(a0) * rr,
                         cy + sinf(a0) * rr,
                         cx + cosf(a1) * rr,
                         cy + sinf(a1) * rr,
                         ringIndex == 0 ? 0.90f : 0.70f,
                         r, g, b, alpha * (ringIndex == 0 ? 0.46f : 0.34f));
        }
    }

    const int tickCount = 12;
    for (int i = 0; i < tickCount; ++i) {
        const float a = phase * 1.28f + pi2 * (float)i / (float)tickCount;
        const float tickOuter = innerRadius * (i % 3 == 0 ? 1.08f : 0.98f);
        const float tickInner = innerRadius * (i % 3 == 0 ? 0.88f : 0.92f);
        drawLineQuad(cx + cosf(a) * tickInner,
                     cy + sinf(a) * tickInner,
                     cx + cosf(a) * tickOuter,
                     cy + sinf(a) * tickOuter,
                     i % 3 == 0 ? 1.0f : 0.65f,
                     r, g, b, alpha * (i % 3 == 0 ? 0.58f : 0.34f));
    }

    DrawGenesisInnerMechanism(cx, cy, base, phase, time,
                              hivePhase, openFactor, r, g, b, alpha);

    for (int k = 0; k < 6; ++k) {
        const float a = shellPhase + (float)k * step;
        const float dx = cosf(a), dy = sinf(a);
        const float tx = -dy, ty = dx;
        const float armStart = base * 0.42f;
        const float armEnd = outerRadius - base * 0.12f;
        const float armHalf = base * 0.055f;
        drawLineQuad(cx + dx * armStart + tx * armHalf,
                     cy + dy * armStart + ty * armHalf,
                     cx + dx * armEnd + tx * armHalf,
                     cy + dy * armEnd + ty * armHalf,
                     0.85f, r, g, b, alpha * 0.48f);
        drawLineQuad(cx + dx * armStart - tx * armHalf,
                     cy + dy * armStart - ty * armHalf,
                     cx + dx * armEnd - tx * armHalf,
                     cy + dy * armEnd - ty * armHalf,
                     0.85f, r, g, b, alpha * 0.48f);

        // Broad docking modules sit on the six vertices. The squared ends
        // preserve the station silhouette without making Genesis look like a
        // collection of attack spikes.
        const float moduleHalf = base * 0.12f;
        const float moduleStart = outerRadius - base * 0.16f;
        const float moduleEnd = outerRadius + base * 0.18f;
        drawLineQuad(cx + dx * moduleStart + tx * moduleHalf,
                     cy + dy * moduleStart + ty * moduleHalf,
                     cx + dx * moduleEnd + tx * moduleHalf,
                     cy + dy * moduleEnd + ty * moduleHalf,
                     1.25f, r, g, b, alpha * 0.76f);
        drawLineQuad(cx + dx * moduleStart - tx * moduleHalf,
                     cy + dy * moduleStart - ty * moduleHalf,
                     cx + dx * moduleEnd - tx * moduleHalf,
                     cy + dy * moduleEnd - ty * moduleHalf,
                     1.25f, r, g, b, alpha * 0.76f);
        drawLineQuad(cx + dx * moduleEnd + tx * moduleHalf,
                     cy + dy * moduleEnd + ty * moduleHalf,
                     cx + dx * moduleEnd - tx * moduleHalf,
                     cy + dy * moduleEnd - ty * moduleHalf,
                     1.10f, r, g, b, alpha * 0.82f);
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

static void DrawRegulusShieldFrame(float x, float y, float base,
                                   float phase, float ratio, int layers,
                                   float alpha, float visualTime) {
    const float shieldR = 0.722f, shieldG = 0.937f, shieldB = 1.0f;
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    const float pulse = 0.5f + 0.5f * sinf(visualTime * 3.2f + phase);
    const float strength = alpha * (0.70f + 0.30f * ratio) *
                           (0.92f + 0.12f * pulse);
    for (int layer = 0; layer < layers; ++layer) {
        const float radius = base * (layers == 1 ? 2.05f + 0.045f * pulse
            : (layer == 0 ? 1.22f + 0.03f * pulse
                          : 2.12f + 0.045f * pulse));
        const float offset = phase + visualTime * (layer ? -0.28f : 0.38f);
        for (int side = 0; side < 6; ++side) {
            const float a0 = offset - 1.5707963f + side * 1.0471976f;
            const float a1 = a0 + 1.0471976f;
            const float trim = 0.10f;
            const float sx = x + cosf(a0) * radius * (1.0f - trim)
                               + cosf(a1) * radius * trim;
            const float sy = y + sinf(a0) * radius * (1.0f - trim)
                               + sinf(a1) * radius * trim;
            const float ex = x + cosf(a0) * radius * trim
                               + cosf(a1) * radius * (1.0f - trim);
            const float ey = y + sinf(a0) * radius * trim
                               + sinf(a1) * radius * (1.0f - trim);
            drawLineQuad(sx, sy, ex, ey, 8.0f,
                         shieldR, shieldG, shieldB, strength * 0.20f);
            drawLineQuad(sx, sy, ex, ey, 2.7f,
                         shieldR, shieldG, shieldB, strength * 0.88f);
            const float nx = (sx + ex) * 0.5f;
            const float ny = (sy + ey) * 0.5f;
            const float nodePulse = 0.5f + 0.5f *
                sinf(visualTime * 4.0f + side * 1.0471976f + phase);
            drawCircle(nx, ny, 4.3f + nodePulse * 1.4f,
                       shieldR, shieldG, shieldB, strength * 0.22f);
            drawCircle(nx, ny, 1.7f + nodePulse * 0.6f,
                       0.92f, 0.99f, 1.0f, strength * 0.92f);
        }
    }
}

void DrawRegulusTethers(const MonsterManager& manager, float visualTime) {
    for (const auto* source : manager.monsters) {
        if (!source->alive || source->kind != MobKind::REGULUS) continue;
        for (const auto& link : source->regulusLinks) {
            if ((!link.monsterTarget && !link.rangedTarget)) continue;
            const bool targetAlive = link.monsterTarget
                ? link.monsterTarget->alive && link.monsterTarget->regulusShield.hp > 0.0f
                : link.rangedTarget->alive && link.rangedTarget->regulusShield.hp > 0.0f;
            if (!targetAlive) continue;
            const float tx = link.monsterTarget ? link.monsterTarget->worldX
                                                : link.rangedTarget->worldX;
            const float ty = link.monsterTarget ? link.monsterTarget->worldY
                                                : link.rangedTarget->worldY;
            const float dx = tx - source->worldX;
            const float dy = ty - source->worldY;
            drawLineQuad(source->worldX, source->worldY, tx, ty,
                         8.0f, 0.42f, 0.84f, 1.0f, 0.18f);
            drawLineQuad(source->worldX, source->worldY, tx, ty,
                         3.4f, 0.722f, 0.937f, 1.0f, 0.42f);
            drawLineQuad(source->worldX, source->worldY, tx, ty,
                         1.6f, 0.90f, 0.98f, 1.0f, 0.92f);
            const float phase = (float)((size_t)source % 101u) * 0.013f;
            for (int pulse = 0; pulse < 3; ++pulse) {
                const float progress = fmodf(visualTime * 0.34f + phase +
                                             pulse / 3.0f, 1.0f);
                const float x = source->worldX + dx * progress;
                const float y = source->worldY + dy * progress;
                const float glow = 0.5f + 0.5f *
                    sinf(visualTime * 5.0f + pulse * 2.0943951f + phase);
                drawCircle(x, y, 5.0f + glow * 1.5f,
                           0.48f, 0.88f, 1.0f, 0.22f + glow * 0.12f);
                drawCircle(x, y, 1.9f + glow * 0.7f,
                           0.92f, 0.99f, 1.0f, 0.86f);
            }
        }
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

static void DrawMagnetar(const Monster* m, float base, float visualTime) {
    const float x = m->worldX, y = m->worldY;
    float cr = m->color.r, cg = m->color.g, cb = m->color.b;
    ApplyMobStyleTint(cr, cg, cb);
    ApplyMobHitFlash(cr, cg, cb, m->hitFlashTimer);
    const float phase = m->magnetarVisualAngle +
                        (float)((size_t)m % 628u) * 0.01f;
    const float pulse = 0.5f + 0.5f * sinf(visualTime * 3.1f);
    const float zap = std::min(1.0f, m->magnetarInterceptTimer / 0.08f);

    // The field stays local and broken; the whole enclosure only flashes on interception.
    if (!g_SuppressMobSeen) {
        const float fieldRadius = Monster::MAGNETAR_FIELD_RADIUS;
        for (int i = 0; i < 4; ++i) {
            const float angle = phase + (float)i * 1.5707963f;
            DrawEnemyArc(x, y, fieldRadius, angle - 0.16f, 0.32f, 5,
                         cr, cg, cb, 0.045f + pulse * 0.025f + zap * 0.42f,
                         1.0f + zap * 0.9f, false);
        }
    }

    // Four hooked shards turn the silhouette into a pinwheel while keeping clear gaps.
    for (int i = 0; i < 4; ++i) {
        const float angle = phase + (float)i * 1.5707963f;
        const float elbowAngle = angle + 0.14f;
        const float tipAngle = angle + 0.29f;
        const float rootR = base * 0.37f;
        const float elbowR = base * 0.83f;
        const float tipR = base * 1.48f;
        const float ax = x + cosf(angle) * rootR;
        const float ay = y + sinf(angle) * rootR;
        const float bx = x + cosf(elbowAngle) * elbowR;
        const float by = y + sinf(elbowAngle) * elbowR;
        const float txp = x + cosf(tipAngle) * tipR;
        const float typ = y + sinf(tipAngle) * tipR;

        // Offset each side of the bent centerline to form one continuous hooked blade.
        const float abLen = sqrtf((bx - ax) * (bx - ax) + (by - ay) * (by - ay));
        const float btLen = sqrtf((txp - bx) * (txp - bx) + (typ - by) * (typ - by));
        const float n1x = -(by - ay) / abLen, n1y = (bx - ax) / abLen;
        const float n2x = -(typ - by) / btLen, n2y = (txp - bx) / btLen;
        const float rootW = base * 0.085f;
        const float elbowW = base * 0.205f;
        const float a1x = ax + n1x * rootW, a1y = ay + n1y * rootW;
        const float b1x = bx + n2x * elbowW, b1y = by + n2y * elbowW;
        const float b2x = bx - n2x * elbowW, b2y = by - n2y * elbowW;
        const float a2x = ax - n1x * rootW, a2y = ay - n1y * rootW;

        BatchTri(a1x, a1y, b1x, b1y, txp, typ, cr, cg, cb, 0.38f);
        BatchTri(a1x, a1y, txp, typ, b2x, b2y, cr, cg, cb, 0.38f);
        BatchTri(a1x, a1y, b2x, b2y, a2x, a2y, 0.18f, 0.15f, 0.08f, 0.58f);
        drawLineQuad(a1x, a1y, b1x, b1y, 2.0f, cr, cg, cb, 0.96f);
        drawLineQuad(b1x, b1y, txp, typ, 2.0f, cr, cg, cb, 0.96f);
        drawLineQuad(a2x, a2y, b2x, b2y, 1.5f, cr, cg, cb, 0.78f);
        drawLineQuad(b2x, b2y, txp, typ, 1.2f, 1.0f, 0.98f, 0.78f, 0.78f);

        for (int mark = 0; mark < 2; ++mark) {
            const int chargeIndex = i * 2 + mark;
            const float t = mark == 0 ? 0.62f : 0.48f;
            const float px = mark == 0 ? ax + (bx - ax) * t : bx + (txp - bx) * t;
            const float py = mark == 0 ? ay + (by - ay) * t : by + (typ - by) * t;
            const float nx = mark == 0 ? n1x : n2x;
            const float ny = mark == 0 ? n1y : n2y;
            const int fullStacks = (int)std::floor(m->magnetarStacks + 0.001f);
            const float fractional = m->magnetarStacks - (float)fullStacks;
            const float alpha = chargeIndex < fullStacks ? 0.98f
                : (chargeIndex == fullStacks ? 0.26f + 0.60f * fractional : 0.20f);
            drawLineQuad(px - nx * base * 0.075f, py - ny * base * 0.075f,
                         px + nx * base * 0.075f, py + ny * base * 0.075f,
                         1.8f, chargeIndex < fullStacks ? 1.0f : cr,
                         chargeIndex < fullStacks ? 0.99f : cg,
                         chargeIndex < fullStacks ? 0.86f : cb, alpha);
        }
    }

    if (m->magnetarInterceptTimer > 0.0f) {
        float startX = x, startY = y;
        float nearest = 1e18f;
        for (int i = 0; i < 4; ++i) {
            const float angle = phase + (float)i * 1.5707963f + 0.29f;
            const float px = x + cosf(angle) * base * 1.48f;
            const float py = y + sinf(angle) * base * 1.48f;
            const float dx = px - m->magnetarInterceptX;
            const float dy = py - m->magnetarInterceptY;
            const float d2 = dx * dx + dy * dy;
            if (d2 < nearest) { nearest = d2; startX = px; startY = py; }
        }
        const float dx = m->magnetarInterceptX - startX;
        const float dy = m->magnetarInterceptY - startY;
        const float len = std::sqrt(dx * dx + dy * dy) + 0.001f;
        const float px = -dy / len, py = dx / len;
        float prevX = startX, prevY = startY;
        for (int i = 1; i <= 5; ++i) {
            const float t = (float)i / 5.0f;
            const float side = i == 5 ? 0.0f : ((i & 1) ? 4.5f : -4.5f);
            const float nextX = startX + dx * t + px * side;
            const float nextY = startY + dy * t + py * side;
            drawLineQuad(prevX, prevY, nextX, nextY, 2.0f,
                         1.0f, 0.99f, 0.82f, zap * 0.96f);
            prevX = nextX; prevY = nextY;
        }
    }

    DrawEnemyCore(x, y, base * (0.30f + zap * 0.05f),
                  cr, cg, cb, 0.92f + zap * 0.08f);
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
        float shieldAlpha = 0.50f;
        if (m->regulusShield.flashTimer > 0.0f)
            shieldAlpha += 0.24f * m->regulusShield.flashTimer / MOB_HIT_FLASH_TIME;
        DrawRegulusShieldFrame(
            m->worldX, m->worldY, base,
            visualTime * 0.045f + (float)((size_t)m % 37u) * 0.02f,
            m->regulusShield.hp / std::max(1.0f, m->regulusShield.maxHp),
            1, shieldAlpha, visualTime);
    }
    if (m->kind == MobKind::GENESIS) {
        // Genesis: a generation station with two fixed parallel egress lanes.
        float x = m->worldX, y = m->worldY;
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        float t = visualTime;
        ApplyMobStyleTint(cr, cg, cb);
        ApplyMobHitFlash(cr, cg, cb, m->hitFlashTimer);
        const float phase = m->hiveOrbitAngle;
        const float stationBase = base * 1.35f;
        // Match the rotating shell and its two opposite openings. The
        // station rotates continuously, but summon count cannot reselect a
        // different lane.
        const float shellPhase = phase - 0.5235988f;
        const float radius = stationBase * 1.52f;
        EnemyNodeAnchor nodes[6];
        for (int k = 0; k < 6; ++k) {
            const float a = shellPhase + (float)k * 1.0471976f;
            nodes[k] = { x + cosf(a) * radius,
                         y + sinf(a) * radius,
                         stationBase * 0.13f,
                         0.86f + 0.10f * sinf(t * 8.0f + k) };
        }
        DrawGenesisStationRing(x, y, stationBase, phase, t,
                               m->hivePhase, m->hiveOpenFactor,
                               cr, cg, cb, 0.46f);
        for (int k = 0; k < 6; ++k)
            DrawEnemyNode(nodes[k], cr, cg, cb, 0.92f);
        const float corePulse = (m->hivePhase == 1)
                              ? 0.94f + 0.06f * m->hiveOpenFactor
                              : (m->hivePhase == 2 ? 1.0f : 0.94f);
        DrawGenesisCore(x, y, stationBase * 0.30f,
                        cr, cg, cb, corePulse, false);
    } else if (m->kind == MobKind::GRAVIS) {
        // Gravis: heavy planetary controller with a visible gravity boundary.
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        ApplyMobStyleTint(cr, cg, cb);
        const float x = m->worldX, y = m->worldY;
        const float phase = m->gravisVisualAngle;
        const float pulse = 0.5f + 0.5f * sinf(visualTime * 2.4f);

        if (!g_SuppressMobSeen) {
            const float fieldR = 285.0f;
            drawCircle(x, y, fieldR, cr, cg, cb, 0.035f + pulse * 0.018f);
            for (int k = 0; k < 24; ++k) {
                const float a0 = phase * 0.18f + (float)k * 0.2617994f;
                const float a1 = a0 + 0.115f;
                drawLineQuad(x + cosf(a0) * fieldR, y + sinf(a0) * fieldR,
                             x + cosf(a1) * fieldR, y + sinf(a1) * fieldR,
                             1.0f, cr, cg, cb, 0.20f);
            }
        }

        float bodyR = cr, bodyG = cg, bodyB = cb;
        ApplyMobHitFlash(bodyR, bodyG, bodyB, m->hitFlashTimer);
        EnemyNodeAnchor nodes[6];
        const float orbitR = base * 1.12f;
        for (int k = 0; k < 6; ++k) {
            const float a = phase + (float)k * 1.0471976f;
            nodes[k] = { x + cosf(a) * orbitR, y + sinf(a) * orbitR,
                         base * 0.13f, 0.82f + 0.12f * sinf(visualTime * 3.0f + k) };
            const int next = (k + 1) % 6;
            const float na = phase + (float)next * 1.0471976f;
            drawLineQuad(nodes[k].x, nodes[k].y,
                         x + cosf(na) * orbitR, y + sinf(na) * orbitR,
                         1.15f, bodyR, bodyG, bodyB, 0.54f);
            drawLineQuad(x, y, nodes[k].x, nodes[k].y,
                         0.72f, bodyR, bodyG, bodyB, 0.22f);
        }
        for (const auto& node : nodes)
            DrawEnemyNode(node, bodyR, bodyG, bodyB, 0.94f);
        drawCircle(x, y, base * 0.58f, bodyR, bodyG, bodyB,
                   0.18f + pulse * 0.08f);
        DrawGenesisCore(x, y, base * 0.34f,
                        bodyR, bodyG, bodyB, 0.96f, false);
    } else if (m->kind == MobKind::QUASAR) {
        // Quasar: a compact accretion instrument with a fixed polar firing axis.
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        ApplyMobStyleTint(cr, cg, cb);
        const float x = m->worldX, y = m->worldY;
        const float ax = m->quasarAimX, ay = m->quasarAimY;
        // The weapon axis tracks independently. The body uses its own rotating
        // basis so free movement still has an unmistakable self-spin.
        const float bodyAx = cosf(m->quasarVisualAngle);
        const float bodyAy = sinf(m->quasarVisualAngle);
        const float bodySx = -bodyAy, bodySy = bodyAx;
        // State 1 searches for an intersecting beam lane. Charging begins only
        // after that lane reaches the player (state 2), so the telegraph and
        // the damage timing communicate the same sequence.
        const float charge = m->quasarState == 1
            ? 0.22f
            : (m->quasarState == 2
                ? std::min(1.0f, m->quasarTimer / 0.55f)
                : (m->quasarState == 3 ? 1.0f : 0.0f));

        if (m->quasarState >= 1 && m->quasarState <= 3) {
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
                const float pulse = 0.82f + 0.18f * sinf(m->quasarTimer * 70.0f);
                drawLineQuad(x0, y0, x1, y1, 76.0f, cr, cg, cb, 0.10f * pulse);
                drawLineQuad(x0, y0, x1, y1, 34.0f, cr, cg, cb, 0.60f * pulse);
                drawLineQuad(x0, y0, x1, y1, 6.2f, 1.0f, 1.0f, 1.0f, 0.98f);

                // A physical release ring marks the acceleration phase without
                // using a full-screen or core flash.
                if (growT > 0.28f && growT < 1.0f) {
                    const float releaseT = (growT - 0.28f) / 0.72f;
                    const float ringR = base * (0.85f + releaseT * 1.75f);
                    DrawEnemyArc(x, y, ringR, 0.0f, 6.28318531f, 32,
                                 cr, cg, cb, (1.0f - releaseT) * 0.52f,
                                 2.2f, false);
                    DrawEnemyArc(x, y, ringR * 0.82f, 0.0f, 6.28318531f, 28,
                                 cr, cg, cb, (1.0f - releaseT) * 0.20f,
                                 1.1f, true);
                }
            } else {
                const float lockPulse = m->quasarState == 2
                    ? 0.72f + 0.28f * sinf(visualTime * 18.0f) : charge;
                drawLineQuad(x0, y0, x1, y1, 1.4f, cr, cg, cb,
                             0.12f + 0.34f * lockPulse);
            }
        }

        float bodyR = cr, bodyG = cg, bodyB = cb;
        ApplyMobHitFlash(bodyR, bodyG, bodyB, m->hitFlashTimer);
        const float majorR = base * (1.32f + charge * 0.10f);
        const float minorR = base * 0.42f;
        const float discSpin = m->quasarVisualAngle * 0.35f;
        DrawQuasarEllipseArc(x, y, bodyAx, bodyAy, majorR, minorR,
                             0.22f + discSpin, 2.70f, 14,
                             bodyR, bodyG, bodyB,
                             0.62f + charge * 0.20f, 1.05f);
        DrawQuasarEllipseArc(x, y, bodyAx, bodyAy, majorR, minorR,
                             3.36f + discSpin, 2.70f, 14,
                             bodyR, bodyG, bodyB,
                             0.62f + charge * 0.20f, 1.05f);
        DrawQuasarEllipseArc(x, y, bodyAx, bodyAy, majorR * 0.73f, minorR * 0.62f,
                             -discSpin * 0.54f, 5.72f, 20,
                             bodyR, bodyG, bodyB,
                             0.24f + charge * 0.14f, 0.72f);

        EnemyNodeAnchor nodes[4] = {
            { x + bodyAx * base * 1.18f, y + bodyAy * base * 1.18f,
              base * 0.17f, 0.90f + charge * 0.10f },
            { x - bodyAx * base * 1.18f, y - bodyAy * base * 1.18f,
              base * 0.17f, 0.82f + charge * 0.10f },
            { x + bodySx * majorR, y + bodySy * majorR, base * 0.14f, 0.78f },
            { x - bodySx * majorR, y - bodySy * majorR, base * 0.14f, 0.78f }
        };
        for (int side = 0; side < 2; ++side) {
            drawLineQuad(nodes[side].x, nodes[side].y,
                         nodes[2].x, nodes[2].y, 0.82f,
                         bodyR, bodyG, bodyB, 0.42f);
            drawLineQuad(nodes[side].x, nodes[side].y,
                         nodes[3].x, nodes[3].y, 0.82f,
                         bodyR, bodyG, bodyB, 0.42f);
        }
        for (const auto& node : nodes)
            DrawEnemyNode(node, bodyR, bodyG, bodyB, 0.92f);

        const float corePulse = m->quasarState == 3
            ? 0.92f + 0.08f * sinf(m->quasarTimer * 70.0f)
            : 0.88f + charge * 0.10f;
        DrawGenesisCore(x, y, base * (0.30f + charge * 0.04f),
                        bodyR, bodyG, bodyB, corePulse, false);
    } else if (m->kind == MobKind::GIMBAL) {
        // Gimbal uses Scope's circular optic silhouette at a smaller scale.
        float bodyR = m->color.r, bodyG = m->color.g, bodyB = m->color.b;
        ApplyMobStyleTint(bodyR, bodyG, bodyB);
        ApplyMobHitFlash(bodyR, bodyG, bodyB, m->hitFlashTimer);
        const float x = m->worldX, y = m->worldY;
        const float phase = m->gimbalVisualAngle +
                            (float)((size_t)m % 628) * 0.01f;
        const bool burst = m->gimbalBursting;
        const float charge = m->gimbalCharging
            ? std::min(1.0f, m->gimbalChargeAngle /
                (6.2831853f * Monster::GIMBAL_CHARGE_ROTATIONS))
            : (burst ? 1.0f : 0.0f);
        const float instrumentR = base * (1.10f + 0.05f * charge);
        const float ringAlpha = 0.70f + 0.25f * charge;

        DrawEnemyArc(x, y, instrumentR, phase, 5.45f, 36,
                     bodyR, bodyG, bodyB, ringAlpha, 1.25f, false);

        EnemyNodeAnchor nodes[4];
        for (int i = 0; i < 4; ++i) {
            const float angle = phase + (float)i * 1.5707963f;
            nodes[i] = { x + cosf(angle) * instrumentR,
                         y + sinf(angle) * instrumentR,
                         base * (0.15f + 0.015f * charge),
                         0.86f + 0.10f * charge +
                             0.04f * sinf(visualTime * 5.0f + i) };
        }
        for (const auto& node : nodes)
            DrawEnemyNode(node, bodyR, bodyG, bodyB, 0.88f + 0.12f * charge);

        if (burst) {
            const float pulse = 0.5f + 0.5f *
                sinf(m->gimbalBurstTimer * 40.0f);
            drawCircle(x, y, base * (1.5f + 0.25f * pulse),
                       1.0f, 1.0f, 1.0f, 0.16f * pulse);
            DrawEnemyCore(x, y, base * (0.38f + 0.08f * pulse),
                          1.0f, 1.0f, 1.0f, 0.88f + 0.12f * pulse);
        } else {
            DrawEnemyCore(x, y, base * (0.33f + charge * 0.05f),
                          bodyR, bodyG, bodyB,
                          0.84f + 0.16f * charge, false);
        }
    } else if (m->kind == MobKind::REGULUS) {
        float bodyR = m->color.r, bodyG = m->color.g, bodyB = m->color.b;
        ApplyMobStyleTint(bodyR, bodyG, bodyB);
        ApplyMobHitFlash(bodyR, bodyG, bodyB, m->hitFlashTimer);
        const float x = m->worldX, y = m->worldY;
        const float phase = m->regulusVisualAngle +
                            (float)((size_t)m % 628u) * 0.01f;
        if (m->regulusShield.hp > 0.0f) {
            float shieldAlpha = 0.64f;
            if (m->regulusShield.flashTimer > 0.0f)
                shieldAlpha += 0.25f * m->regulusShield.flashTimer /
                               MOB_HIT_FLASH_TIME;
            DrawRegulusShieldFrame(
                x, y, base, phase,
                m->regulusShield.hp / std::max(1.0f, m->regulusShield.maxHp),
                1, shieldAlpha, visualTime);
        }
        const float corePulse = 0.5f + 0.5f * sinf(visualTime * 3.6f);
        const float hullPulse = 0.5f + 0.5f * sinf(visualTime * 2.4f + phase);
        const float hullInner = base * 0.66f;
        const float hullOuter = base * 1.08f;
        const float hullRadius = base * 1.02f;
        const float sideStep = 1.0471976f;

        // A six-section outer chassis makes the body read as a built support
        // unit around its core instead of a core with only short spokes.
        for (int i = 0; i < 6; ++i) {
            const float angle = phase - 1.5707963f + i * sideStep;
            const float innerX = x + cosf(angle) * hullInner;
            const float innerY = y + sinf(angle) * hullInner;
            const float outerX = x + cosf(angle) * hullOuter;
            const float outerY = y + sinf(angle) * hullOuter;
            drawLineQuad(innerX, innerY, outerX, outerY, 3.0f,
                         bodyR, bodyG, bodyB, 0.72f);
            drawLineQuad(innerX, innerY, outerX, outerY, 1.0f,
                         0.92f, 0.99f, 1.0f, 0.72f);

            const float plateStart = angle + 0.15f;
            DrawEnemyArc(x, y, hullRadius, plateStart, 0.74f, 7,
                         bodyR, bodyG, bodyB, 0.86f, 3.0f, false);
            const float nodePulse = 0.78f + 0.18f *
                sinf(visualTime * 3.2f + i * sideStep);
            DrawEnemyNode({outerX, outerY, base * (0.15f + 0.015f * hullPulse),
                           nodePulse}, bodyR, bodyG, bodyB, 0.95f);
            drawCircle(outerX, outerY, base * 0.055f,
                       0.94f, 1.0f, 1.0f, 0.74f + 0.18f * hullPulse);
        }

        drawCircle(x, y, base * (0.62f + 0.05f * corePulse),
                   0.40f, 0.84f, 1.0f, 0.08f + 0.08f * corePulse);
        for (int i = 0; i < 6; ++i) {
            const float angle = phase - 1.5707963f + i * sideStep;
            const float nx = x + cosf(angle) * base * 0.52f;
            const float ny = y + sinf(angle) * base * 0.52f;
            drawLineQuad(x, y, nx, ny, 1.4f,
                         bodyR, bodyG, bodyB, 0.54f);
            DrawEnemyNode({nx, ny, base * 0.10f,
                           0.78f + 0.18f * sinf(visualTime * 3.2f + i)},
                          bodyR, bodyG, bodyB, 0.90f);
        }
        DrawEnemyCore(x, y, base * 0.42f,
                      bodyR, bodyG, bodyB,
                      0.92f + 0.08f * sinf(visualTime * 3.0f), false);
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
        const float phase = t * 0.62f + phOff - 1.5708f;
        const float radius = base * 0.92f;
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
        DrawEnemyCore(x, y, base * 0.28f, cr, cg, cb, 0.94f, true);
    } else {
        // Rotor (ROTOR): one square frame with four shared star anchors.
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        ApplyMobStyleTint(cr, cg, cb);
        ApplyMobHitFlash(cr, cg, cb, m->hitFlashTimer);
        float x = m->worldX, y = m->worldY;
        float phOff = (float)((size_t)m % 628) * 0.01f;
        const float half = base * 0.78f;
        const float angle = visualTime * 0.78f + phOff;
        float vx[4], vy[4];
        EnemyNodeAnchor nodes[4];
        for (int k = 0; k < 4; ++k) {
            const float a = angle + 0.7853982f + (float)k * 1.5707963f;
            vx[k] = x + cosf(a) * half * 1.4142f;
            vy[k] = y + sinf(a) * half * 1.4142f;
            nodes[k] = { vx[k], vy[k], base * 0.16f, 0.92f };
        }
        DrawEnemyWirePolygon(vx, vy, 4, cr, cg, cb, 0.80f, 0.08f, 0.85f);
        for (int k = 0; k < 4; ++k)
            DrawEnemyNode(nodes[k], cr, cg, cb, 0.94f);
        DrawEnemyCore(x, y, base * 0.29f, cr, cg, cb, 0.96f);
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
        float shieldAlpha = 0.50f;
        if (r->regulusShield.flashTimer > 0.0f)
            shieldAlpha += 0.24f * r->regulusShield.flashTimer / MOB_HIT_FLASH_TIME;
        DrawRegulusShieldFrame(
            x, y, base, visualTime * 0.045f + phaseOffset,
            r->regulusShield.hp / std::max(1.0f, r->regulusShield.maxHp),
            1, shieldAlpha, visualTime);
    }
    const float instrumentR = base * (1.26f + 0.12f * chargeT);
    const float phase = r->rotAngle + phaseOffset;
    const bool burst = r->lensState == RangedMob::State::BURST;

    // Scope: a circular observation instrument, not a second triangular body.
    DrawEnemyArc(x, y, instrumentR, phase, 5.45f, 44,
                 cr, cg, cb, 0.62f + 0.18f * chargeT, 0.90f, false);
    DrawEnemyArc(x, y, instrumentR * 0.74f, -phase * 0.72f + 0.45f,
                 burst ? 5.85f : 4.55f, 36,
                 cr, cg, cb, burst ? 0.18f : 0.42f + 0.12f * chargeT,
                 0.72f, false);
    EnemyNodeAnchor nodes[4];
    for (int i = 0; i < 4; ++i) {
        const float a = phase + (float)i * 1.5707963f;
        nodes[i] = { x + cosf(a) * instrumentR,
                     y + sinf(a) * instrumentR,
                     base * (0.13f + 0.02f * chargeT),
                     0.86f + 0.10f * sinf(visualTime * 5.0f + i) };
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
        preview.quasarVisualAngle = t * 0.08f;
        preview.gimbalVisualAngle = t * 0.24f;
        preview.regulusVisualAngle = t * 0.10f;
        preview.antaresVisualAngle = t * 0.5236f;
        drawMob(&preview);
    }
    g_SuppressMobSeen = wasSuppressed;
}
