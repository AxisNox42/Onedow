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
    if (b.normaShot) {
        // NORMA's shot: a turned square outline with a short tail.
        const float ang = atan2f(b.dirY, b.dirX) + (float)glfwGetTime() * 6.0f;
        const float h = r * 1.15f;
        float px[4], py[4];
        for (int k = 0; k < 4; ++k) {
            const float a = ang + 0.7853982f + 1.5707963f * (float)k;
            px[k] = b.x + cosf(a) * h * 1.4142f;
            py[k] = b.y + sinf(a) * h * 1.4142f;
        }
        const float tail = std::min(26.0f, b.speed * 0.07f);
        drawLineQuad(b.x - b.dirX * h, b.y - b.dirY * h,
                     b.x - b.dirX * (h + tail), b.y - b.dirY * (h + tail),
                     2.0f, b.color.r, b.color.g, b.color.b, 0.35f);
        for (int k = 0; k < 4; ++k) {
            const int n = (k + 1) & 3;
            drawLineQuad(px[k], py[k], px[n], py[n], 3.4f,
                         b.color.r * 0.22f, b.color.g * 0.22f, b.color.b * 0.22f, 0.6f);
            drawLineQuad(px[k], py[k], px[n], py[n], 1.8f, b.color.r, b.color.g, b.color.b, 1.0f);
        }
        drawCircle(b.x, b.y, r * 0.30f, 1.0f, 1.0f, 1.0f, 0.95f);
        return;
    }
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
    } else if (m->kind == MobKind::NORMA) {
        // The boss: a stronger glow out past its bracket ring, in its one
        // colour. It runs red while the boss controls a pane, and swells
        // brighter (same colour) while exposed.
        alpha = 0.19f;
        radiusScale = 3.00f;
        if (m->normaPattern == norma::Pattern::Ram && m->normaExposed <= 0.0f &&
            (m->normaStage == norma::Stage::Telegraph || m->normaStage == norma::Stage::Act)) {
            r = 1.0f; g = 0.30f; b = 0.26f;
            alpha = 0.24f;
        }
        // A charge reddens it over the wind-up and holds red through the run.
        if (m->normaPattern == norma::Pattern::Dash && m->normaStage == norma::Stage::Act &&
            m->normaDashStep < 2) {
            const float k = m->normaDashStep == 1 ? 1.0f : m->NormaDashWindShare();
            r += (1.0f - r) * k; g += (0.30f - g) * k; b += (0.26f - b) * k;
            alpha += 0.05f * k;
        }
        if (m->normaExposed > 0.0f) {
            const float k = std::min(1.0f, m->normaExposed / 0.2f);
            alpha += 0.12f * k;
            radiusScale += 0.35f * k;
        }
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
    // NORMA's dark field reaches past its orbiting bracket ring (2.6 base).
    case MobKind::NORMA:   return 3.20f;
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
    // Genesis' own colour family (shared with the enemies it controls).
    const float pR = Monster::GENESIS_PRODUCTION.r, pG = Monster::GENESIS_PRODUCTION.g, pB = Monster::GENESIS_PRODUCTION.b;
    const float hR = Monster::GENESIS_HEAT.r, hG = Monster::GENESIS_HEAT.g, hB = Monster::GENESIS_HEAT.b;
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
    case MobKind::MAGNETAR: return 4.30f;   // wraps the whole polyhedron shell, vertices included
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

// Elite extension of the Regulus crest: the old body is untouched, layers are
// added outward. (1) a second ring of four wide, thin plates outside the
// shield plates, turning against them; (2) three diamond nodes on a ring
// between crest and plates that light up as the next volley charges. One
// colour family, solid thin lines only.
static void DrawRegulusExtension(const Monster* m, float base, float phase,
                                 float r, float g, float b, float visualTime) {
    const float x = m->worldX, y = m->worldY;
    // (1) outer plates, offset 45 degrees from the shield plates.
    for (int k = 0; k < 4; ++k) {
        const float a0 = -phase * 0.55f + 0.7853982f + 1.5707963f * (float)k + 0.10f;
        DrawEnemyArc(x, y, base * 2.02f, a0, 1.5707963f - 0.20f, 14,
                     r, g, b, 0.36f, 1.1f, false);
        DrawEnemyArc(x, y, base * 1.93f, a0 + 0.12f, 1.5707963f - 0.44f, 10,
                     0.92f, 0.99f, 1.0f, 0.22f, 0.8f, false);
    }
    // (2) charge nodes: dim at rest, brighter and larger over the last 0.8 s.
    constexpr float CHARGE_WINDOW = 0.8f;
    const float charge = m->regulusHasTarget && m->regulusShotTimer < CHARGE_WINDOW
        ? 1.0f - std::max(0.0f, m->regulusShotTimer) / CHARGE_WINDOW : 0.0f;
    for (int k = 0; k < 3; ++k) {
        const float a = visualTime * 0.45f + 2.0943951f * (float)k;
        const float cx = x + cosf(a) * base * 1.40f, cy = y + sinf(a) * base * 1.40f;
        const float s = base * (0.10f + 0.07f * charge);
        const float al = 0.45f + 0.55f * charge;
        const float dx[4] = { 0.0f, s, 0.0f, -s }, dy[4] = { -s, 0.0f, s, 0.0f };
        for (int i = 0; i < 4; ++i) {
            const int n = (i + 1) & 3;
            drawLineQuad(cx + dx[i], cy + dy[i], cx + dx[n], cy + dy[n],
                         1.2f + 0.8f * charge, 0.92f, 0.99f, 1.0f, al);
        }
    }
}

// Instant shield laser: a straight line from the Regulus to the target that
// flashes and fades over REGULUS_BEAM_TIME. No projectile is drawn.
// Thin solid ring built from line segments (no fills, no dashes).
static void DrawLineRing(float cx, float cy, float radius, float width,
                         float r, float g, float b, float alpha) {
    constexpr int SEG = 72;
    constexpr float STEP = 6.2831853f / (float)SEG;
    for (int i = 0; i < SEG; ++i) {
        const float a0 = (float)i * STEP, a1 = a0 + STEP;
        drawLineQuad(cx + cosf(a0) * radius, cy + sinf(a0) * radius,
                     cx + cosf(a1) * radius, cy + sinf(a1) * radius,
                     width, r, g, b, alpha);
    }
}

// Buff markers on a buffed mob, one colour per buff: speed = four amber arcs
// spinning fast, stealth = one thin violet ring. Drawn at full alpha so the
// cloaked body stays locatable. (The shield has its own bracket drawing.)
void DrawRegulusBuffMarks(const Monster* m, float visualTime) {
    // Marks sit just outside each kind's silhouette (same extent the shield
    // brackets use), so big mobs get big marks and small ones stay tight.
    const float ext = (m->summoned ? 28.0f : 18.0f) * m->sizeScale * AegisMarkExtent(m->kind);
    const float sp = m->RegulusSpeedLevel();
    if (sp > 0.0f) {
        const glm::vec3 c = Monster::RegulusBuffColor(Monster::REGULUS_BUFF_SPEED);
        const float rad = ext * 1.0f, spin = visualTime * 5.0f;
        for (int k = 0; k < 4; ++k) {
            const float a0 = spin + (float)k * 1.5707963f;
            for (int i = 0; i < 6; ++i) {
                const float u0 = a0 + (float)i * 0.1f, u1 = u0 + 0.1f;
                drawLineQuad(m->worldX + cosf(u0) * rad, m->worldY + sinf(u0) * rad,
                             m->worldX + cosf(u1) * rad, m->worldY + sinf(u1) * rad,
                             1.6f, c.r, c.g, c.b, 0.85f * sp);
            }
        }
    }
    const float st = m->RegulusStealthLevel();
    if (st > 0.0f) {
        const glm::vec3 c = Monster::RegulusBuffColor(Monster::REGULUS_BUFF_STEALTH);
        // Triangle broken in three places (a gap mid-edge, three corners
        // remain), solid lines, turning slowly.
        // Circumradius 1.35 ext, short corner pieces: they stay ~0.95 ext from the centre.
        const float rad = ext * 1.35f, spin = visualTime * 0.6f;
        const float alpha = (0.75f + 0.15f * sinf(visualTime * 3.0f)) * st;
        constexpr float KEEP = 0.22f;   // share of each edge kept at either end
        for (int k = 0; k < 3; ++k) {
            const float a0 = spin + (float)k * 2.0943951f - 1.5707963f;
            const float a1 = a0 + 2.0943951f;
            const float x0 = m->worldX + cosf(a0) * rad, y0 = m->worldY + sinf(a0) * rad;
            const float x1 = m->worldX + cosf(a1) * rad, y1 = m->worldY + sinf(a1) * rad;
            const float dx = x1 - x0, dy = y1 - y0;
            drawLineQuad(x0, y0, x0 + dx * KEEP, y0 + dy * KEEP, 1.4f, c.r, c.g, c.b, alpha);
            drawLineQuad(x1 - dx * KEEP, y1 - dy * KEEP, x1, y1, 1.4f, c.r, c.g, c.b, alpha);
        }
    }
}

void DrawRegulusBeams(const MonsterManager& manager, float /*visualTime*/) {
    for (const auto& beam : manager.regulusBeams) {
        const float t = std::clamp(beam.age / Monster::REGULUS_BEAM_TIME, 0.0f, 1.0f);
        const float fade = (1.0f - t) * (1.0f - t);
        const glm::vec3 bc = Monster::RegulusBuffColor(beam.buff);
        const glm::vec3 hi = glm::mix(bc, glm::vec3(1.0f), 0.8f);
        drawLineQuad(beam.x0, beam.y0, beam.x1, beam.y1, 7.0f * (1.0f - 0.5f * t),
                     bc.r, bc.g, bc.b, 0.22f * fade);
        drawLineQuad(beam.x0, beam.y0, beam.x1, beam.y1, 1.8f,
                     hi.r, hi.g, hi.b, 0.95f * fade);
        drawCircle(beam.x1, beam.y1, 10.0f * (1.0f + t), bc.r, bc.g, bc.b,
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
        float edgeFront[12][12] = {};   // 0 back .. 1 front, smooth in depth
        float edgeLit[12][12] = {};
        bool edgeSeen[12][12] = {};
        float edgeBroken[12][12];
        float edgeRestored[12][12];
        for (int a = 0; a < 12; ++a)
            for (int b = 0; b < 12; ++b) { edgeBroken[a][b] = -1.0f; edgeRestored[a][b] = 99.0f; }
        for (int f = 0; f < 20; ++f) {
            const int ids[3] = { ML::kF[f][0], ML::kF[f][1], ML::kF[f][2] };
            const float zf = (pv[ids[0]].z + pv[ids[1]].z + pv[ids[2]].z) / 3.0f;
            const float ft = std::clamp((zf + 0.25f) / 0.5f, 0.0f, 1.0f);
            const float front = ft * ft * (3.0f - 2.0f * ft);
            const float bt = m->magnetarFaceBroken[f];
            const float lit = bt >= 0.0f ? 0.0f : faceLit(f);
            for (int e = 0; e < 3; ++e) {
                const int a = std::min(ids[e], ids[(e + 1) % 3]);
                const int b = std::max(ids[e], ids[(e + 1) % 3]);
                edgeSeen[a][b] = true;
                edgeFront[a][b] = std::max(edgeFront[a][b], front);
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
                    float width = 0.8f + 0.6f * edgeFront[a][b];
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
                                 std::min(1.0f, (0.25f + 0.50f * edgeFront[a][b]) * flick));
                } else if (edgeLit[a][b] > 0.0f) {
                    drawLineQuad(vx[a], vy[a], vx[b], vy[b], 2.4f,
                                 1.0f, 1.0f, 1.0f, 0.95f * edgeLit[a][b]);
                } else {
                    const float snap = std::max(0.0f, 1.0f - edgeRestored[a][b] / kRestoreTime);
                    const float ef = edgeFront[a][b];
                    drawLineQuad(vx[a], vy[a], vx[b], vy[b], 0.9f + 0.7f * ef + 0.8f * snap,
                                 cr + (1.0f - cr) * snap, cg + (1.0f - cg) * snap,
                                 cb + (1.0f - cb) * snap,
                                 0.22f + 0.58f * ef + 0.20f * snap);
                }
            }
        }
        for (int i = 0; i < 12; ++i)
            if (pv[i].z > 0.0f) {
                const float dz = std::min(1.0f, pv[i].z / 0.3f);   // fade in, no pop
                if (vBroken[i] >= 0.0f) drawCircle(vx[i], vy[i], 1.8f, 1.0f, 0.80f, 0.55f, 0.55f * dz);
                else drawCircle(vx[i], vy[i], 2.0f, 1.0f, 0.97f, 0.88f, 0.70f * dz);
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
    // Lines only: no filled glow under the seed. A face break flashes the
    // seed's own edges instead (below).
    const float seedFlash = newestBreak < 0.30f ? 1.0f - newestBreak / 0.30f : 0.0f;
    (void)pulse;
    static const int kOctE[12][2] = { {0, 2}, {0, 3}, {0, 4}, {0, 5}, {1, 2}, {1, 3},
                                      {1, 4}, {1, 5}, {2, 4}, {2, 5}, {3, 4}, {3, 5} };
    // The seed always glows: every edge stays bright, depth only trims it a
    // little and smoothly, so no edge switches on or off as it turns.
    for (const auto& e : kOctE) {
        const float zf = 0.5f * (ov[e[0]].z + ov[e[1]].z);
        const float ft = std::clamp(0.5f + 0.5f * zf, 0.0f, 1.0f);
        const float depth = ft * ft * (3.0f - 2.0f * ft);
        drawLineQuad(sx[e[0]], sy[e[0]], sx[e[1]], sy[e[1]],
                     (1.4f + 0.4f * depth) + 1.2f * seedFlash,
                     sr + (1.0f - sr) * seedFlash, sg + (1.0f - sg) * seedFlash,
                     sb + (1.0f - sb) * seedFlash, 0.85f + 0.15f * depth);
    }
    drawCircle(seedX, seedY, base * 0.07f, 1.0f, 1.0f, 1.0f, 0.95f);

    // Tethers: four seed tips (equator) each hold one fixed lattice vertex
    // (two antipodal pairs, so the seed is held from all sides). The pairing
    // never changes as the lattice turns; only the brightness follows depth,
    // so nothing pops on and off. A cut tether (broken face on its vertex)
    // fades out quickly and returns on restore.
    {
        static const int kTips[4] = { 0, 1, 4, 5 };
        static const int kHold[4] = { 9, 10, 0, 3 };
        for (int k = 0; k < 4; ++k) {
            const int tip = kTips[k], v = kHold[k];
            float hold = 1.0f;
            if (vBroken[v] >= 0.0f) hold = std::max(0.0f, 1.0f - vBroken[v] / 0.15f);
            else hold = std::min(1.0f, vRestored[v] / kRestoreTime);
            if (vBroken[v] < 0.0f && vRestored[v] >= 90.0f) hold = 1.0f;
            if (hold <= 0.01f) continue;
            const float z01 = std::clamp(0.5f + 0.5f * pv[v].z, 0.0f, 1.0f);
            const float depth = 0.70f + 0.30f * z01 * z01 * (3.0f - 2.0f * z01);
            drawLineQuad(sx[tip], sy[tip], vx[v], vy[v], 1.4f, cr, cg, cb,
                         0.70f * depth * hold);
            drawCircle(vx[v], vy[v], 2.4f, 1.0f, 1.0f, 1.0f, 0.6f * depth * hold * z01);
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

static void DrawQuasarFall(const Monster* m, float cr, float cg, float cb) {
    if (!m->QuasarFalling()) return;
    const float age = m->quasarTimer;
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
        const float power = m->quasarFallPower[i];            // 0 near the Quasar .. 1 far
        const float radius = m->QuasarFallRadius(i);
        if (after < 0.0f) {
            const float progress = std::clamp((age - m->quasarFallLaunchTimes[i]) / m->QuasarFallWarning(i), 0.0f, 1.0f);
            // Dark under-stroke keeps the unfilled danger disc legible on white windows.
            DrawEnemyArc(target.x, target.y, radius, 0.0f, 6.2831853f, 64,
                         0.0f, 0.0f, 0.0f, 0.75f, 5.0f, false);
            DrawEnemyArc(target.x, target.y, radius, 0.0f, 6.2831853f, 64,
                         cr, cg, cb, 0.65f + 0.25f * power, 1.5f + 1.5f * power, false);
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
            if (m->quasarFallPredictedMask & (1u << i)) {
                // Aimed shot: a second, inner ring and four diagonal ticks,
                // all white, so it reads apart from the scattered ones.
                DrawEnemyArc(target.x, target.y, radius * 0.62f, 0.0f, 6.2831853f, 48,
                             1.0f, 1.0f, 1.0f, 0.55f + 0.35f * progress, 1.4f, false);
                for (int k = 0; k < 4; ++k) {
                    const float a = 0.7853982f + k * 1.5707963f;
                    const glm::vec2 dir(cosf(a), sinf(a));
                    const glm::vec2 p0 = target + dir * (radius * 0.62f + 3.0f);
                    const glm::vec2 p1 = target + dir * (radius * 0.62f + 11.0f);
                    drawLineQuad(p0.x, p0.y, p1.x, p1.y, 1.6f, 1.0f, 1.0f, 1.0f, 0.80f);
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
            // Impact: a hard hit, not a bubble. A white flash line cross, a
            // thick shockwave ring that snaps out to the damage radius, jagged
            // fracture lines splitting from the centre, a crater hexagon that
            // collapses, and heavy shards with streaks. Power (distance from
            // the Quasar) thickens and lengthens all of it.
            const float t = after / Monster::QUASAR_FALL_IMPACT_FX;          // 0..1
            const float out = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);   // ease out
            const float fade = (1.0f - t) * (1.0f - t);
            const float heavy = 1.0f + 0.7f * power;
            // Instant flash: cross lines across the whole zone for the first frames.
            if (t < 0.22f) {
                const float f = 1.0f - t / 0.22f;
                for (int k = 0; k < 4; ++k) {
                    const float a = k * 0.7853982f + 0.2f;
                    drawLineQuad(target.x - cosf(a) * radius * 1.05f, target.y - sinf(a) * radius * 1.05f,
                                 target.x + cosf(a) * radius * 1.05f, target.y + sinf(a) * radius * 1.05f,
                                 3.5f * heavy * f + 1.0f, 1.0f, 1.0f, 1.0f, 0.85f * f);
                }
            }
            // Shockwave: thick bright ring (with a dark under-stroke) out to the radius.
            DrawEnemyArc(target.x, target.y, radius * (0.35f + 0.65f * out), 0.0f, 6.2831853f, 64,
                         0.0f, 0.0f, 0.0f, 0.55f * fade, 7.0f * heavy * (1.0f - 0.6f * t), false);
            DrawEnemyArc(target.x, target.y, radius * (0.35f + 0.65f * out), 0.0f, 6.2831853f, 64,
                         1.0f, 1.0f, 1.0f, fade, 4.5f * heavy * (1.0f - 0.7f * t), false);
            DrawEnemyArc(target.x, target.y, radius * (0.20f + 0.55f * out), 0.0f, 6.2831853f, 48,
                         cr, cg, cb, 0.85f * fade, 2.2f * heavy, false);
            // The damage radius stays marked until the effect ends.
            DrawEnemyArc(target.x, target.y, radius, 0.0f, 6.2831853f, 64,
                         cr, cg, cb, 0.55f * (1.0f - t), 2.0f, false);
            // Fracture lines: three-part jagged rays.
            for (int k = 0; k < 9; ++k) {
                const float a = k * 0.6981317f + 0.37f * i;
                const float len = radius * (0.30f + 0.78f * out) * (0.78f + 0.22f * ((k * 7) % 5) / 4.0f);
                const glm::vec2 d0(cosf(a), sinf(a));
                const glm::vec2 side(-d0.y, d0.x);
                const float kink = ((k & 1) ? 1.0f : -1.0f) * radius * 0.07f;
                glm::vec2 q0 = target + d0 * (radius * 0.10f);
                const glm::vec2 q1 = target + d0 * (len * 0.45f) + side * kink;
                const glm::vec2 q2 = target + d0 * (len * 0.75f) - side * kink * 0.6f;
                const glm::vec2 q3 = target + d0 * len;
                const float w = (2.6f * heavy) * (1.0f - 0.6f * t);
                drawLineQuad(q0.x, q0.y, q1.x, q1.y, w, 1.0f, 1.0f, 1.0f, 0.95f * fade);
                drawLineQuad(q1.x, q1.y, q2.x, q2.y, w * 0.8f, cr, cg, cb, 0.90f * fade);
                drawLineQuad(q2.x, q2.y, q3.x, q3.y, w * 0.55f, cr, cg, cb, 0.75f * fade);
            }
            // Crater: a hexagon collapsing to the centre.
            {
                const float cr0 = radius * 0.34f * (1.0f - out * 0.85f);
                for (int k = 0; k < 6; ++k) {
                    const float a0 = k * 1.0471976f + 0.3f * i, a1 = a0 + 1.0471976f;
                    drawLineQuad(target.x + cosf(a0) * cr0, target.y + sinf(a0) * cr0,
                                 target.x + cosf(a1) * cr0, target.y + sinf(a1) * cr0,
                                 2.4f * heavy, 1.0f, 1.0f, 1.0f, 0.95f * (1.0f - t * 0.5f));
                }
            }
            // Heavy shards with streaks.
            for (int k = 0; k < 8; ++k) {
                const float a = k * 0.7853982f + 0.4f + i * 0.37f;
                const float d = radius * (0.25f + 1.15f * out);
                const glm::vec2 dir(cosf(a), sinf(a));
                const glm::vec2 pos = target + dir * d;
                const glm::vec2 tail = target + dir * std::max(radius * 0.18f, d - radius * 0.38f);
                drawLineQuad(tail.x, tail.y, pos.x, pos.y, 3.0f * heavy * (1.0f - t * 0.5f),
                             cr, cg, cb, 0.80f * fade);
                diamond(pos, (8.0f + 4.0f * power) * (1.0f - 0.6f * t), 1.0f - t * t, a);
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
        const float size = 10.5f + 2.5f * Monster::QuasarEase(flight / m->QuasarFallWarning(i));
        diamond(p, size, 0.98f, angle);
        if (m->quasarFallPredictedMask & (1u << i))
            DrawEnemyArc(p.x, p.y, size * 1.9f, 0.0f, 6.2831853f, 24,
                         1.0f, 1.0f, 1.0f, 0.65f, 1.3f, false);
        const glm::vec2 side(cosf(angle), sinf(angle));
        diamond(p - side * 14.0f, 2.8f, 0.65f, angle);
        diamond(p + side * 12.0f, 2.2f, 0.65f, angle);

    }
}

// Soft light around the beam (user-approved exception to "lines only"): a strip
// along the leg whose alpha falls off smoothly with the distance from the axis,
// built with per-vertex alpha so it spreads like light at any angle instead of
// stepping like stacked bands. Inside the band it is steady; outside it fades
// out over `spread` px. `tipFade` > 0 fades the strip out over the last
// tipFade px of the leg (the growing beam head).
static void DrawBeamGlow(float x0, float y0, float x1, float y1, float bandR, float spread,
                         float strength, float tipFade, float cr, float cg, float cb) {
    const float len = std::hypot(x1 - x0, y1 - y0);
    if (len < 1.0f || strength <= 0.0f || bandR < 1.0f) return;
    const glm::vec2 a(x0, y0), dir((x1 - x0) / len, (y1 - y0) / len), nrm(-dir.y, dir.x);
    const float gr = cr + (1.0f - cr) * 0.35f, gg = cg + (1.0f - cg) * 0.35f, gb = cb + (1.0f - cb) * 0.35f;
    constexpr int RINGS = 9;                                   // steps across the fade
    // lateral samples from the far left to the far right, with the alpha at each
    float off[2 * RINGS + 4], al[2 * RINGS + 4];
    int m = 0;
    for (int i = RINGS; i >= 1; --i) {                          // left halo: dark -> band edge
        const float u = (float)i / (float)RINGS;                // 1 at the far edge, 0 at the band edge
        off[m] = -(bandR + spread * u);
        al[m++] = strength * std::pow(1.0f - u, 2.2f);
    }
    off[m] = -bandR; al[m++] = strength;
    off[m] = 0.0f;   al[m++] = strength;
    off[m] = bandR;  al[m++] = strength;
    for (int i = 1; i <= RINGS; ++i) {                          // right halo
        const float u = (float)i / (float)RINGS;
        off[m] = bandR + spread * u;
        al[m++] = strength * std::pow(1.0f - u, 2.2f);
    }
    // one strip for the body, one for the fading tip
    const float bodyEnd = tipFade > 0.0f ? std::max(0.0f, len - tipFade) : len;
    const auto strip = [&](float sA, float sB, float fA, float fB) {
        if (sB - sA < 0.5f) return;
        const glm::vec2 pa = a + dir * sA, pb = a + dir * sB;
        for (int j = 0; j + 1 < m; ++j) {
            const glm::vec2 a0 = pa + nrm * off[j], a1 = pa + nrm * off[j + 1];
            const glm::vec2 b0 = pb + nrm * off[j], b1 = pb + nrm * off[j + 1];
            BatchVtx(a0.x, a0.y, gr, gg, gb, al[j] * fA);
            BatchVtx(a1.x, a1.y, gr, gg, gb, al[j + 1] * fA);
            BatchVtx(b0.x, b0.y, gr, gg, gb, al[j] * fB);
            BatchVtx(a1.x, a1.y, gr, gg, gb, al[j + 1] * fA);
            BatchVtx(b1.x, b1.y, gr, gg, gb, al[j + 1] * fB);
            BatchVtx(b0.x, b0.y, gr, gg, gb, al[j] * fB);
        }
    };
    strip(0.0f, bodyEnd, 1.0f, 1.0f);
    if (tipFade > 0.0f) strip(bodyEnd, len, 1.0f, 0.0f);
}

// Energy running through the beam: bright streaks that flow from the Quasar out
// along the leg, and a slower, wider pulse that rolls out every 0.6 s. Both are
// positioned by the path distance from the Quasar (`startDist` carries them over
// bounced legs), so they run on through a bounce instead of restarting.
static void DrawBeamFlow(float x0, float y0, float x1, float y1, float bandR, float startDist,
                         float t, float intensity, int streaks, int legIndex,
                         float cr, float cg, float cb) {
    const float len = std::hypot(x1 - x0, y1 - y0);
    if (len < 1.0f || intensity <= 0.0f) return;
    const glm::vec2 a(x0, y0), dir((x1 - x0) / len, (y1 - y0) / len), nrm(-dir.y, dir.x);
    // Draw the part of the path-distance range [D0, D1] that falls on this leg.
    const auto seg = [&](float D0, float D1, float lateral, float wColor, float wWhite,
                         float aColor, float aWhite) {
        const float s0 = std::max(D0, startDist) - startDist, s1 = std::min(D1, startDist + len) - startDist;
        if (s1 - s0 < 0.5f) return;
        const glm::vec2 p0 = a + dir * s0 + nrm * lateral, p1 = a + dir * s1 + nrm * lateral;
        drawLineQuad(p0.x, p0.y, p1.x, p1.y, wColor, cr, cg, cb, aColor);
        drawLineQuad(p0.x, p0.y, p1.x, p1.y, wWhite, 1.0f, 1.0f, 1.0f, aWhite);
    };
    // Streaks: thin, fast, each on its own fixed lane inside the band, brightest at the head.
    constexpr float WRAP = 3200.0f, SPEED = 2400.0f;
    for (int k = 0; k < streaks; ++k) {
        const float h0 = QuasarHash(k * 3.71f + 0.5f), h1 = QuasarHash(k * 5.13f + 1.5f), h2 = QuasarHash(k * 2.29f + 2.5f);
        const float lateral = (h0 * 2.0f - 1.0f) * 0.74f * bandR;
        const float length = 90.0f + 130.0f * h1;
        const float phase = std::fmod(t * SPEED / WRAP + h2, 1.0f);
        const float head = phase * WRAP;
        const float fade = std::clamp(1.0f - 0.55f * phase, 0.0f, 1.0f) * intensity;
        for (int p = 0; p < 3; ++p) {                           // tail -> head, brightening
            const float d0 = head - length + length * (float)p / 3.0f, d1 = head - length + length * (float)(p + 1) / 3.0f;
            const float k3 = 0.20f + 0.40f * (float)p;           // 0.20, 0.60, 1.00 of the head's strength
            seg(d0, d1, lateral, 2.6f, 1.3f, 0.55f * k3 * fade, 0.90f * k3 * fade);
        }
    }
    // Pulse: a wider bright knot leaves the Quasar every 0.6 s and rolls out.
    constexpr float PULSE_SPEED = 1800.0f;
    for (int n = 0; n < 6; ++n) {
        const float age = t - (0.20f + 0.60f * (float)n);
        if (age < 0.0f) continue;
        const float Dw = age * PULSE_SPEED;
        if (Dw > 5200.0f) continue;
        const float fade = std::clamp(1.0f - Dw / 4600.0f, 0.0f, 1.0f) * intensity;
        // Feathered knot: short slices whose strength peaks in the middle, so both
        // ends taper away instead of ending in a square block.
        constexpr int SLICES = 9;
        for (int i = 0; i < SLICES; ++i) {
            const float u0 = -1.0f + 2.0f * (float)i / SLICES, u1 = -1.0f + 2.0f * (float)(i + 1) / SLICES;
            const float w = std::pow(1.0f - std::fabs(0.5f * (u0 + u1)), 1.6f);
            seg(Dw + u0 * 150.0f, Dw + u1 * 150.0f + 1.0f, 0.0f, bandR * 1.1f * (0.4f + 0.6f * w),
                bandR * 0.50f * (0.3f + 0.7f * w), 0.22f * w * fade, 0.75f * w * fade);
        }
    }
    (void)legIndex;
}

// The rift: the beam band opening out from the Quasar. A bright seam runs out
// along the axis (its tip travels 6000 px in QUASAR_BEAM_RIFT_PROP) and the band
// opens behind the tip, so the lips spread outward from the Quasar like a tear.
// `t` is the time since the beam fired and `startDist` the path length from the
// Quasar to this leg's start (bounced legs carry the tear on). Smooth curves
// only; once fully open the lips are exactly the straight band edges.
static void DrawBeamRift(float x0, float y0, float x1, float y1, float bandR, float t,
                         float startDist, float cr, float cg, float cb) {
    const float len = std::hypot(x1 - x0, y1 - y0);
    if (len < 1.0f) return;
    const glm::vec2 a(x0, y0), dir((x1 - x0) / len, (y1 - y0) / len), nrm(-dir.y, dir.x);
    const float PROP = Monster::QUASAR_BEAM_RIFT_PROP, OPEN = Monster::QUASAR_BEAM_RIFT_OPEN;
    const float TOTAL = 6000.0f;
    const float tipD = TOTAL * std::clamp(t / PROP, 0.0f, 1.0f);       // how far the seam has run
    const auto openAt = [&](float D) {                                  // 0 = seam, 1 = fully open band
        return Monster::QuasarEase((t - PROP * D / TOTAL) / OPEN);
    };
    const float rift = 1.0f - Monster::QuasarEase(t / 0.55f);           // the void inside, clearing
    const int n = std::min(96, (int)std::ceil(len / 40.0f));
    const float step = len / (float)n;
    glm::vec2 pc(0.0f), pl(0.0f), pr(0.0f);
    float po = 0.0f;
    for (int i = 0; i <= n; ++i) {
        const float s = i * step, D = startDist + s;
        const float o = bandR * openAt(D);
        const glm::vec2 c = a + dir * s;
        const glm::vec2 l = c + nrm * o, r = c - nrm * o;
        if (i > 0) {
            const float dm = startDist + (i - 0.5f) * step;
            if (dm <= tipD) {
                const float om = 0.5f * (o + po);
                const float seam = 1.0f - openAt(dm);
                // The void between the lips, and a thin tint of the beam colour.
                if (om > 0.5f) {
                    drawLineQuad(pc.x, pc.y, c.x, c.y, 2.0f * om, 0.0f, 0.0f, 0.0f, 0.82f * rift);
                    drawLineQuad(pc.x, pc.y, c.x, c.y, 2.0f * om, cr, cg, cb, 0.14f);
                    const float la = std::min(1.0f, om / 8.0f);
                    drawLineQuad(pl.x, pl.y, l.x, l.y, 4.0f, 0.0f, 0.0f, 0.0f, 0.60f * la);
                    drawLineQuad(pl.x, pl.y, l.x, l.y, 2.6f, cr, cg, cb, 0.95f * la);
                    drawLineQuad(pl.x, pl.y, l.x, l.y, 1.2f, 1.0f, 1.0f, 1.0f, 0.95f * la);
                    drawLineQuad(pr.x, pr.y, r.x, r.y, 4.0f, 0.0f, 0.0f, 0.0f, 0.60f * la);
                    drawLineQuad(pr.x, pr.y, r.x, r.y, 2.6f, cr, cg, cb, 0.95f * la);
                    drawLineQuad(pr.x, pr.y, r.x, r.y, 1.2f, 1.0f, 1.0f, 1.0f, 0.95f * la);
                }
                // The seam itself: a bright hairline with a soft halo, gone once the band is open.
                if (seam > 0.02f) {
                    drawLineQuad(pc.x, pc.y, c.x, c.y, 9.0f * seam + 1.0f, cr, cg, cb, 0.45f * seam);
                    drawLineQuad(pc.x, pc.y, c.x, c.y, 2.6f, 1.0f, 1.0f, 1.0f, seam);
                }
            }
        }
        pc = c; pl = l; pr = r; po = o;
    }
}

// Eruption (two beats: the rings, then the lane band and the body zone), staged so it builds instead of springing: (1) the Quasar draws in
// and a few soft pulse rings leave it; (2) the rings of zones unfold one after
// the other from the inside out, each with a ripple travelling out to it;
// (3) inward energy streaks flow into the Quasar and every zone's countdown arc
// closes at the same moment; (4) all zones go off together with a compact
// version of the starfall impact and one big ring leaving the Quasar.
// Lines only.
static void DrawQuasarEruption(const Monster* m, float cr, float cg, float cb) {
    if (!m->alive || !m->quasarEruptActive) return;
    const float age = m->quasarEruptTimer;
    const float Z = Monster::QUASAR_ERUPT_ZONE_RADIUS;
    const float blast = Monster::QUASAR_ERUPT_BLAST_TIME;
    const float fx = Monster::QUASAR_FALL_IMPACT_FX;
    const float qx = m->worldX, qy = m->worldY;
    const auto ease = [](float x) { x = std::clamp(x, 0.0f, 1.0f); return 1.0f - (1.0f - x) * (1.0f - x) * (1.0f - x); };

    if (age < blast) {
        const float charge = Monster::QuasarEase(age / (blast * 0.9f));     // 0..1 build-up
        // (1) wind-up: two soft pulse rings leave the body, slow and low.
        if (age < Monster::QUASAR_ERUPT_WINDUP + 0.4f) {
            for (int k = 0; k < 2; ++k) {
                const float u = std::clamp((age - 0.15f * k) / 0.9f, 0.0f, 1.0f);
                if (u <= 0.0f || u >= 1.0f) continue;
                DrawEnemyArc(qx, qy, 40.0f + 260.0f * ease(u), 0.0f, 6.2831853f, 64,
                             cr, cg, cb, 0.45f * (1.0f - u), 2.0f, false);
            }
        }
        // (3) energy streaks flowing into the Quasar from every direction; they
        // thicken and brighten as the blast nears.
        for (int k = 0; k < 28; ++k) {
            const float a = k * 0.2244f + 0.31f * (k % 3);
            const float ph = std::fmod(age * (0.55f + 0.5f * charge) + k * 0.371f, 1.0f);
            const float r0 = 400.0f * (1.0f - ph * ph) + 60.0f, r1 = r0 + 26.0f + 40.0f * charge;
            const glm::vec2 d(cosf(a), sinf(a));
            drawLineQuad(qx + d.x * r0, qy + d.y * r0, qx + d.x * r1, qy + d.y * r1,
                         1.1f + 1.4f * charge, cr, cg, cb, 0.65f * (0.20f + 0.55f * charge) * (1.0f - 0.7f * ph));
        }
    }

    for (int i = 0; i < m->quasarEruptCount; ++i) {
        const glm::vec2 c = m->quasarEruptZones[i];
        const int beat = m->quasarEruptBeat[i];
        const float Zi = m->QuasarEruptZoneRadius(i);
        const float blastI = m->QuasarEruptZoneBlast(i);
        const float showAt = m->QuasarEruptZoneShow(i);
        if (age < blastI) {
            if (age < showAt) continue;
            const float u = std::clamp((age - showAt) / Monster::QUASAR_ERUPT_RING_GROW, 0.0f, 1.0f);
            const float grow = ease(u);                                   // unfolding
            const float prog = std::clamp((age - showAt) / (blastI - showAt), 0.0f, 1.0f);
            const float rad = Zi * (0.40f + 0.60f * grow);
            const float pulse = 0.78f + 0.22f * sinf(age * (12.0f + 16.0f * prog) + i);
            // Beat 1 waits dim and thin under the first beat, then lights up the
            // moment it goes off: the old safe ground is the new danger.
            const float lit = beat == 0 ? 1.0f : ease((age - Monster::QUASAR_ERUPT_BLAST_TIME) / 0.15f);
            const float armed = beat == 0 ? 1.0f : 0.28f + 0.72f * lit;
            const float thin = beat == 0 ? 1.0f : 0.55f + 0.45f * lit;
            DrawEnemyArc(c.x, c.y, rad, 0.0f, 6.2831853f, 40, 0.0f, 0.0f, 0.0f, 0.70f * grow * armed, 5.5f * thin, false);
            DrawEnemyArc(c.x, c.y, rad, 0.0f, 6.2831853f, 40, cr, cg, cb, 0.85f * grow * pulse * armed, 2.4f * thin, false);
            // The inner ring turns the other way, faster as the blastI nears.
            DrawEnemyArc(c.x, c.y, rad * 0.55f, (beat ? -1.0f : 1.0f) * age * (1.0f + 3.0f * prog), 4.7f, 24,
                         cr, cg, cb, 0.55f * grow * armed, 1.4f, false);
            DrawEnemyArc(c.x, c.y, rad, -1.5707963f, prog * 6.2831853f, 40,
                         1.0f, 1.0f, 1.0f, 0.90f * grow * armed, 2.6f * thin, false);
            for (int k = 0; k < 4; ++k) {
                const float a = k * 1.5707963f + 0.7853982f + 0.5f * prog;
                const glm::vec2 d(cosf(a), sinf(a));
                const float breathe = Zi * (0.62f + 0.10f * sinf(age * 8.0f + k));
                const glm::vec2 p0 = c + d * (rad - 4.0f), p1 = c + d * (breathe * (rad / Zi));
                drawLineQuad(p0.x, p0.y, p1.x, p1.y, 1.8f, 1.0f, 1.0f, 1.0f, 0.55f * grow * armed);
            }
        } else {
            const float after = age - blastI;
            if (after >= fx) continue;
            const float t = after / fx;
            const float out = ease(t);
            const float fade = (1.0f - t) * (1.0f - t);
            if (t < 0.22f) {
                const float f = 1.0f - t / 0.22f;
                for (int k = 0; k < 2; ++k) {
                    const float a = k * 1.5707963f + 0.4f;
                    drawLineQuad(c.x - cosf(a) * Zi * 1.05f, c.y - sinf(a) * Zi * 1.05f,
                                 c.x + cosf(a) * Zi * 1.05f, c.y + sinf(a) * Zi * 1.05f,
                                 3.0f * f + 1.0f, 1.0f, 1.0f, 1.0f, 0.85f * f);
                }
            }
            DrawEnemyArc(c.x, c.y, Zi * (0.35f + 0.65f * out), 0.0f, 6.2831853f, 40,
                         0.0f, 0.0f, 0.0f, 0.5f * fade, 6.0f * (1.0f - 0.6f * t), false);
            DrawEnemyArc(c.x, c.y, Zi * (0.35f + 0.65f * out), 0.0f, 6.2831853f, 40,
                         1.0f, 1.0f, 1.0f, fade, 3.8f * (1.0f - 0.7f * t), false);
            DrawEnemyArc(c.x, c.y, Zi, 0.0f, 6.2831853f, 40, cr, cg, cb, 0.55f * (1.0f - t), 2.0f, false);
            for (int k = 0; k < 6; ++k) {
                const float a = k * 1.0471976f + 0.37f * i;
                const float len = Zi * (0.30f + 0.70f * out);
                const glm::vec2 d(cosf(a), sinf(a)), side(-d.y, d.x);
                const float kink = ((k & 1) ? 1.0f : -1.0f) * Zi * 0.07f;
                const glm::vec2 q0 = c + d * (Zi * 0.10f), q1 = c + d * (len * 0.55f) + side * kink, q2 = c + d * len;
                const float w = 2.4f * (1.0f - 0.6f * t);
                drawLineQuad(q0.x, q0.y, q1.x, q1.y, w, 1.0f, 1.0f, 1.0f, 0.95f * fade);
                drawLineQuad(q1.x, q1.y, q2.x, q2.y, w * 0.7f, cr, cg, cb, 0.8f * fade);
            }
        }
    }
    // (2) ripples: each ring's unfolding is led by a thin ring leaving the Quasar
    // and arriving at that ring's radius.
    if (age < blast) {
        for (int ring = 0; ring < 8; ++ring) {
            const float showAt = m->QuasarEruptShowTime(ring);
            const float r = Monster::QUASAR_ERUPT_FIRST_RING + Monster::QUASAR_ERUPT_RING_STEP * (float)ring;
            const float u = (age - (showAt - 0.25f)) / 0.45f;
            if (u <= 0.0f || u >= 1.0f) continue;
            bool exists = false;
            for (int i = 0; i < m->quasarEruptCount && !exists; ++i) exists = m->quasarEruptBeat[i] == 0 && m->quasarEruptRing[i] == ring;
            if (!exists) continue;
            DrawEnemyArc(qx, qy, r * ease(u), 0.0f, 6.2831853f, 72,
                         1.0f, 1.0f, 1.0f, 0.50f * (1.0f - u), 2.0f, false);
        }
    }
    // One big shockwave leaves the Quasar and runs out through the whole field.
    if (age >= blast && age < blast + fx) {
        const float t = (age - blast) / fx;
        const float reach = (Monster::QUASAR_ERUPT_MAX_REACH + 90.0f) * ease(t);
        DrawEnemyArc(qx, qy, reach, 0.0f, 6.2831853f, 96,
                     0.0f, 0.0f, 0.0f, 0.45f * (1.0f - t), 9.0f, false);
        DrawEnemyArc(qx, qy, reach, 0.0f, 6.2831853f, 96,
                     1.0f, 1.0f, 1.0f, 0.90f * (1.0f - t), 4.0f, false);
    }
    // Beat 1: a shorter wave, out to the lane band only.
    const float blast2 = Monster::QUASAR_ERUPT_SECOND_BLAST;
    if (age >= blast2 && age < blast2 + fx) {
        const float t = (age - blast2) / fx;
        const float reach = Monster::QUASAR_ERUPT_MAX_REACH * 0.85f * ease(t);
        DrawEnemyArc(qx, qy, reach, 0.0f, 6.2831853f, 96,
                     0.0f, 0.0f, 0.0f, 0.45f * (1.0f - t), 9.0f, false);
        DrawEnemyArc(qx, qy, reach, 0.0f, 6.2831853f, 96,
                     1.0f, 1.0f, 1.0f, 0.90f * (1.0f - t), 4.0f, false);
    }
}

// NORMA placeholder body (design pending): a square outline and centre point
// in ice blue. Phase 2 adds a turning inner square; entering it flashes white.
// NORMA's tesseract: a wire cube turning in 3D with a smaller cube inside it
// (turning the other way), drawn like the mobs: edges split short of each
// vertex over a faint glow, star nodes on the vertices (dimmer at the back),
// thin links between the two cubes' matching vertices.
struct NormaCube {
    float sx[8], sy[8], depth[8];   // projected points, depth -1 (back) .. 1 (front)
};

// drill: a turn about the diagonal through corner `axisCorner` and its
// opposite. A whole number of thirds of a turn maps the cube onto itself.
static NormaCube ProjectNormaCube(float x, float y, float radius, float yaw, float pitch, float roll,
                                  float drill = 0.0f, int axisCorner = 6) {
    const auto& kCorner = norma::kCubeCorner;
    NormaCube c{};
    const float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch);
    const float cr = cosf(roll), sr = sinf(roll);
    const float cdr = cosf(drill), sdr = sinf(drill);
    static constexpr float kAxis = 0.57735027f;   // 1 / sqrt(3)
    const float ax = kCorner[axisCorner][0] * kAxis, ay = kCorner[axisCorner][1] * kAxis,
                az = kCorner[axisCorner][2] * kAxis;
    for (int i = 0; i < 8; ++i) {
        float px = kCorner[i][0], py = kCorner[i][1], pz = kCorner[i][2];
        if (drill != 0.0f) {
            // Rodrigues' turn about the diagonal.
            const float dot = px * ax + py * ay + pz * az;
            const float qx = ay * pz - az * py, qy = az * px - ax * pz, qz = ax * py - ay * px;
            const float k = dot * (1.0f - cdr);
            const float nx = px * cdr + qx * sdr + ax * k, ny = py * cdr + qy * sdr + ay * k,
                        nz = pz * cdr + qz * sdr + az * k;
            px = nx; py = ny; pz = nz;
        }
        const float x1 = px * cy + pz * sy, z1 = -px * sy + pz * cy;
        const float y2 = py * cp - z1 * sp, z2 = py * sp + z1 * cp;
        const float rx = x1 * cr - y2 * sr, ry = x1 * sr + y2 * cr;
        c.sx[i] = x + rx * radius;
        c.sy[i] = y + ry * radius;
        c.depth[i] = z2 / 1.7320508f;
    }
    return c;
}

// Presses the projected cube flat along screen direction (ux, uy) by
// `share` (0 none .. 1 flat), about its centre.
static void SquashNormaCube(NormaCube& c, float x, float y, float ux, float uy, float share) {
    for (int i = 0; i < 8; ++i) {
        const float d = (c.sx[i] - x) * ux + (c.sy[i] - y) * uy;
        c.sx[i] -= ux * d * share;
        c.sy[i] -= uy * d * share;
    }
}

// Just the outer edges, thin: afterimages and the charge lane.
static void DrawNormaCubeOutline(const NormaCube& c, float r, float g, float b, float alpha) {
    static constexpr int kEdge[12][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 },
        { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
    for (const auto& e : kEdge)
        drawLineQuad(c.sx[e[0]], c.sy[e[0]], c.sx[e[1]], c.sy[e[1]], 1.1f, r, g, b, alpha);
}

// How a cube's edges are drawn: split short of the stars by `gap`, pushed
// out from (cx, cy) by `spread` (phase 2's loosened shell), with or without
// the faint glow under them.
struct NormaEdgeStyle {
    float gap = 0.16f;
    float spread = 0.0f;
    float cx = 0.0f, cy = 0.0f;
    bool glow = true;
};

static void DrawNormaCubeEdges(const NormaCube& c, float r, float g, float b,
                               float alpha, float width, const NormaEdgeStyle& st) {
    static constexpr int kEdge[12][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 },
        { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
    for (const auto& e : kEdge) {
        const int i = e[0], j = e[1];
        const float front = 0.5f + 0.25f * (c.depth[i] + c.depth[j]);   // 0 back .. 1 front
        const float a = alpha * (0.40f + 0.55f * front);
        float ax = c.sx[i] + (c.sx[j] - c.sx[i]) * st.gap, ay = c.sy[i] + (c.sy[j] - c.sy[i]) * st.gap;
        float bx = c.sx[j] + (c.sx[i] - c.sx[j]) * st.gap, by = c.sy[j] + (c.sy[i] - c.sy[j]) * st.gap;
        if (st.spread != 0.0f) {
            const float mx = (ax + bx) * 0.5f - st.cx, my = (ay + by) * 0.5f - st.cy;
            const float len = std::max(1.0f, std::sqrt(mx * mx + my * my));
            ax += mx / len * st.spread; bx += mx / len * st.spread;
            ay += my / len * st.spread; by += my / len * st.spread;
        }
        if (st.glow) drawLineQuad(ax, ay, bx, by, width + 2.4f, r, g, b, a * 0.14f);
        drawLineQuad(ax, ay, bx, by, width, r, g, b, a);
    }
}

// The cube's stars. `lit` of them shine (norma::kStarOutOrder decides which
// go dark first); a dark one keeps only its outline and a faint point. The
// star that just went dark (`outStar`) flares as it fades (`outFlash` 1..0).
static void DrawNormaCubeStars(const NormaCube& c, float r, float g, float b, float alpha,
                               float nodeR, int lit = 8, int outStar = -1, float outFlash = 0.0f) {
    bool dark[8] = {};
    for (int k = lit; k < norma::kHealthStars; ++k) dark[norma::kStarOutOrder[k]] = true;
    for (int i = 0; i < 8; ++i) {
        const float front = 0.5f + 0.5f * c.depth[i];
        const float rr = nodeR * (0.8f + 0.3f * front);
        if (!dark[i]) {
            DrawEnemyNode({ c.sx[i], c.sy[i], rr, 0.55f + 0.45f * front }, r, g, b, alpha);
            continue;
        }
        DrawEnemyArc(c.sx[i], c.sy[i], rr * 0.72f, 0.0f, 6.2831853f, 16, r, g, b, 0.45f * alpha, 1.1f, false);
        drawCircle(c.sx[i], c.sy[i], rr * 0.16f, r, g, b, 0.30f * alpha);
        if (i == outStar && outFlash > 0.0f) {
            const float t = 1.0f - outFlash;
            DrawEnemyArc(c.sx[i], c.sy[i], rr * (0.8f + 2.2f * t), 0.0f, 6.2831853f, 24,
                         1.0f, 1.0f, 1.0f, 0.9f * outFlash, 2.0f, false);
            drawCircle(c.sx[i], c.sy[i], rr * 0.9f * outFlash, 1.0f, 1.0f, 1.0f, 0.8f * outFlash);
        }
    }
}

static void DrawNormaCube(const NormaCube& c, float r, float g, float b,
                          float alpha, float width, float nodeR) {
    DrawNormaCubeEdges(c, r, g, b, alpha, width, NormaEdgeStyle{});
    DrawNormaCubeStars(c, r, g, b, alpha, nodeR);
}

static void DrawNormaBody(const Monster* m, float base, float visualTime) {
    // The body shivers in the last moment of a charge's wind-up.
    const float windShare = m->NormaDashWindShare();
    const float windLeft = (1.0f - windShare) * norma::DashWindFor(m->normaPhase, m->normaDash);
    const float shiver = windShare > 0.0f && windLeft < norma::kDashTremble ? 1.2f : 0.0f;
    const float x = m->worldX + shiver * sinf(visualTime * 97.0f);
    const float y = m->worldY + shiver * cosf(visualTime * 83.0f);
    const float flash = std::clamp(m->normaPhaseFlash / norma::kPhaseFlashSeconds, 0.0f, 1.0f);
    const float hit = std::clamp(m->hitFlashTimer / MOB_HIT_FLASH_TIME, 0.0f, 1.0f);
    const float recoil = std::clamp(m->normaRecoil / norma::kRecoilSeconds, 0.0f, 1.0f);
    // Charging (telegraph): the square tightens and brightens as it gathers.
    float charge = 0.0f;
    if (m->normaStage == norma::Stage::Telegraph) {
        const float span = m->normaPattern == norma::Pattern::Refuge ? norma::kRefugeTelegraph
                         : m->normaPattern == norma::Pattern::Aim ? norma::kAimTelegraph
                         : m->normaPattern == norma::Pattern::Throw ? norma::kThrowAppear + norma::kThrowFill
                         : m->normaPattern == norma::Pattern::Detonate ? norma::kDetonateFuseWait
                         : m->normaPattern == norma::Pattern::Ram ? norma::RamAimFor(m->normaPhase)
                         : 1.0f;
        charge = std::clamp(m->normaTimer / span, 0.0f, 1.0f);
    }
    const float half = base * 1.3f * (1.0f - 0.14f * charge + 0.10f * recoil);
    const float lift = std::max({ flash, hit * 0.6f, recoil * 0.5f, charge * 0.35f });
    const bool p2 = m->normaPhase == 2;
    // One fixed colour through every motion and both phases; only a hit
    // flashes it (briefly) and pane control turns it red (below).
    const float r = m->color.r + (1.0f - m->color.r) * hit * 0.6f;
    const float g = m->color.g + (1.0f - m->color.g) * hit * 0.6f;
    const float b = m->color.b;
    const float pulse = 0.85f + 0.15f * sinf(visualTime * 2.0f);

    // Body: the tesseract, turned as the boss keeps it (Monster::TurnNormaCube:
    // the aimed stream and the charge roll the nearest corner onto their
    // direction, the short way). For the stream that star is the muzzle, for
    // the charge the leading point; a charge then drills about that corner's
    // diagonal a whole number of thirds of a turn, so it lands as it set off.
    const float va = m->normaVisualAngle;
    const float yaw = m->normaCubeYaw, pitch = m->normaCubePitch, roll = m->normaCubeRoll;
    const int lead = m->normaCubeLead;
    const bool dashing = m->normaPattern == norma::Pattern::Dash;
    float hold = 0.0f;   // 0..1 how far the cube has settled onto its corner
    if (m->normaPattern == norma::Pattern::Aim || dashing) {
        if (m->normaStage == norma::Stage::Telegraph)
            hold = std::min(1.0f, m->normaTimer / 0.25f);
        else if (m->normaStage == norma::Stage::Act)
            hold = dashing && m->normaDash == 0 && m->normaDashStep == 0
                 ? std::min(1.0f, m->normaDashTimer / 0.2f) : 1.0f;
        else if (m->normaStage == norma::Stage::Recover)
            hold = std::max(0.0f, 1.0f - m->normaTimer / 0.3f);
    }
    float drill = 0.0f;
    // Firing the aimed stream, the cube turns slowly about its muzzle corner.
    if (m->normaPattern == norma::Pattern::Aim && m->normaStage == norma::Stage::Act)
        drill = m->normaTimer * norma::kAimDrill;
    float windK = 0.0f;   // 0..1 through a charge's wind-up
    float spring = 0.0f;  // 0..1 how pressed the body is: builds in the wind-up, lets go early in the run
    if (dashing && m->normaStage == norma::Stage::Act) {
        if (m->normaDashStep == 0) {
            windK = windShare;
            spring = windK * windK * (3.0f - 2.0f * windK);
        } else if (m->normaDashStep == 1) {
            const float k = std::clamp(m->normaDashTimer / norma::kDashTime, 0.0f, 1.0f);
            const float ease = k * k * (3.0f - 2.0f * k);   // as the body moves
            const float release = std::max(0.0f, 1.0f - k / 0.35f);
            spring = release * release * (3.0f - 2.0f * release);
            // About a third of a turn per two body lengths travelled.
            const float thirds = std::max(1.0f, std::round(m->normaDashLength / (base * 4.6f)));
            drill = thirds * 2.0943951f * ease;
        }
    }
    // Controlling a pane (the ram), or charging: the whole body runs red.
    float red = 0.0f;
    if (dashing && m->normaStage == norma::Stage::Act) {
        red = m->normaDashStep == 0 ? windK * windK * (3.0f - 2.0f * windK)
            : m->normaDashStep == 1 ? 1.0f
            : 1.0f - std::clamp(m->normaDashTimer / norma::kDashRest, 0.0f, 1.0f);
    }
    if (m->normaPattern == norma::Pattern::Ram) {
        if (m->normaStage == norma::Stage::Telegraph) red = std::min(1.0f, m->normaTimer / 0.2f);
        else if (m->normaStage == norma::Stage::Act) red = 1.0f;
        else if (m->normaStage == norma::Stage::Recover) red = std::max(0.0f, 1.0f - m->normaTimer / 0.3f);
    }
    // Once the rammed pane hits, the boss is exposed: the red lets go at
    // once so the opening reads in its own colour.
    if (m->normaExposed > 0.0f) red = 0.0f;
    const float br = r + (1.0f - r) * red, bg = g + (0.30f - g) * red, bb = b + (0.26f - b) * red;
    // Winding up a charge, the inner cube draws in tight.
    const float outerR = base * 1.29f * (1.0f - 0.10f * charge + 0.08f * recoil);   // +12% on 1.15
    // Exposed (an opening after a charge or a ram): the inner cube swells out
    // to the outer one, the core flares and the brackets drift wide and dim.
    const float exposed = std::min(1.0f, m->normaExposed / 0.2f);
    const float innerShare = (p2 ? 0.62f : 0.45f) * (1.0f - 0.35f * spring) * (1.0f - exposed) +
                             0.96f * exposed;
    NormaCube outer = ProjectNormaCube(x, y, outerR, yaw, pitch, roll, drill, lead);
    NormaCube inner = ProjectNormaCube(x, y, outerR * innerShare,
                                       -yaw * 1.3f + 0.4f, pitch * 0.6f + (p2 ? va * 1.4f : 0.0f), roll,
                                       -drill, lead);
    // Winding up, the body presses flat along the charge like a spring.
    if (spring > 0.0f) {
        SquashNormaCube(outer, x, y, m->normaDashDirX, m->normaDashDirY, 0.18f * spring);
        SquashNormaCube(inner, x, y, m->normaDashDirX, m->normaDashDirY, 0.18f * spring);
    }
    // Charge form: in the run the body stretches along the lane (a negative
    // squash), most at mid-run, back to shape as it lands.
    if (dashing && m->normaStage == norma::Stage::Act && m->normaDashStep == 1) {
        const float k = std::clamp(m->normaDashTimer / norma::kDashTime, 0.0f, 1.0f);
        const float stretch = 0.30f * std::sin(3.14159265f * k);
        SquashNormaCube(outer, x, y, m->normaDashDirX, m->normaDashDirY, -stretch);
        SquashNormaCube(inner, x, y, m->normaDashDirX, m->normaDashDirY, -stretch);
    }
    // Afterimages of a glide or a charge: thin outlines of the cube as it is.
    if (m->normaGhostLife > 0.0f) {
        const float life = m->normaGhostLife / 0.25f;
        for (int i = 0; i < norma::kGhostCount; ++i) {
            const int idx = (m->normaGhostHead - i + norma::kGhostCount) % norma::kGhostCount;
            const float gx = m->normaGhostX[idx], gy = m->normaGhostY[idx];
            if (gx == 0.0f && gy == 0.0f) continue;
            const float a = life * 0.40f * (1.0f - (float)i / norma::kGhostCount);
            const NormaCube ghost = ProjectNormaCube(gx, gy, outerR, yaw, pitch, roll,
                                                     drill * (1.0f - 0.08f * (float)(i + 1)), lead);
            DrawNormaCubeOutline(ghost, br, bg, bb, a);
        }
    }
    // Hierarchy: the outer shell carries the silhouette (bold, big stars);
    // the inner cube and the links stay quiet. In phase 2 the shell comes
    // apart (edges split wider, pushed out, dimmer) and the inner cube takes
    // over, bold and bright. The outer stars are the health (one per eighth).
    for (int i = 0; i < 8; ++i)
        drawLineQuad(outer.sx[i], outer.sy[i], inner.sx[i], inner.sy[i], 0.7f, br, bg, bb, p2 ? 0.10f : 0.16f);
    const float punch = 1.2f * flash + 0.8f * recoil;
    const int lit = m->normaLitStars;
    const int outStar = lit < norma::kHealthStars ? norma::kStarOutOrder[lit] : -1;
    const float outFlash = m->normaStarFlash / norma::kStarOutFlash;
    if (p2) {
        NormaEdgeStyle shell;
        shell.gap = 0.30f;
        shell.spread = base * 0.32f;
        shell.cx = x;
        shell.cy = y;
        DrawNormaCubeEdges(outer, br, bg, bb, 0.55f * pulse, 1.6f + punch, shell);
        DrawNormaCubeEdges(inner, br, bg, bb, pulse, 2.2f + punch, NormaEdgeStyle{});
        DrawNormaCubeStars(inner, br, bg, bb, pulse, base * 0.15f);
        DrawNormaCubeStars(outer, br, bg, bb, pulse, base * 0.20f * (1.0f + 0.4f * lift), lit, outStar, outFlash);
    } else {
        NormaEdgeStyle quiet;
        quiet.glow = false;
        DrawNormaCubeEdges(inner, br, bg, bb, 0.45f * pulse, 0.9f, quiet);
        DrawNormaCubeEdges(outer, br, bg, bb, pulse, 2.6f + punch, NormaEdgeStyle{});
        DrawNormaCubeStars(inner, br, bg, bb, 0.6f * pulse, base * 0.08f);
        DrawNormaCubeStars(outer, br, bg, bb, pulse, base * 0.22f * (1.0f + 0.4f * lift), lit, outStar, outFlash);
    }
    if (hold > 0.0f && !dashing)   // the aimed stream's muzzle star (it stays put while drilling)
        DrawEnemyNode({ outer.sx[lead], outer.sy[lead], base * 0.30f, 1.0f }, br, bg, bb, hold);
    const float flare = exposed * (0.5f + 0.5f * sinf(visualTime * 18.0f));
    DrawEnemyCore(x, y, base * ((p2 ? 0.48f : 0.40f) + 0.20f * exposed), br, bg, bb,
                  0.85f + 0.15f * pulse + 0.3f * flare, false);

    // The panes' corner brackets orbit the body only while it handles panes
    // (hiding in one, summoning through them, ramming with one): they open
    // out from the body as they appear and fold back in as they go.
    const float ring = m->normaRingShow;
    if (ring > 0.01f) {
        const float orbit = base * 2.9f * (0.6f + 0.4f * ring);
        const float arm = base * 0.45f;
        const float spin = va;
        for (int k = 0; k < 4; ++k) {
            const float a = spin + 0.7853982f + 1.5707963f * (float)k;
            const float bx = x + cosf(a) * orbit, by = y + sinf(a) * orbit;
            const float ia = a + 3.14159265f;   // arms point back toward the body
            const float a1 = ia - 0.7853982f, a2 = ia + 0.7853982f;
            const float ba = 0.85f * ring;
            drawLineQuad(bx, by, bx + cosf(a1) * arm, by + sinf(a1) * arm, 1.8f, br, bg, bb, ba);
            drawLineQuad(bx, by, bx + cosf(a2) * arm, by + sinf(a2) * arm, 1.8f, br, bg, bb, ba);
        }
    }

    // Gathering lines while charging: short strokes sliding into the body.
    if (charge > 0.0f) {
        for (int i = 0; i < 12; ++i) {
            const float a = 6.2831853f * (float)i / 12.0f + 0.13f * (float)i;
            const float phase = fmodf(visualTime * 1.8f + (float)i * 0.37f, 1.0f);
            const float r0 = half * (2.6f - 1.4f * phase), r1 = r0 - half * 0.35f;
            const float al = charge * (1.0f - phase) * 0.8f;
            drawLineQuad(x + cosf(a) * r0, y + sinf(a) * r0, x + cosf(a) * r1, y + sinf(a) * r1,
                         1.4f, r, g, b, al);
        }
    }

    // Pattern warnings, thin violet lines (warning colour; danger is ice).
    // Warnings share the body's colour (the charge lane alone is red).
    const float vr = r, vg = g, vb = b;
    if (m->normaPattern == norma::Pattern::Dash && m->normaStage == norma::Stage::Act &&
        m->normaDashStep <= 1) {
        // The landing spot is a red target ring the size of the charge's
        // reach, filling clockwise over the wind-up (Gravis' destination
        // circle); slim red chevrons run along the lane toward it. Once the
        // lane locks both brighten and the ring gets its inward ticks.
        // In the run the ring stays (full) until the body arrives, fading as
        // it closes in; the chevrons are gone.
        const bool running = m->normaDashStep == 1;
        const float runK = running ? std::clamp(m->normaDashTimer / norma::kDashTime, 0.0f, 1.0f) : 0.0f;
        const float k = running ? 1.0f : windShare;
        const float locked = k >= norma::kDashLock ? 1.0f : 0.0f;
        const float ux = m->normaDashDirX, uy = m->normaDashDirY;
        const float px = -uy, py = ux;
        const float tx = m->normaDashToX, ty = m->normaDashToY;
        const float laneLen = std::max(1.0f, (tx - x) * ux + (ty - y) * uy);
        const float reach = base * 1.3f + norma::kDashReach;
        const float appear = std::min(1.0f, k * 4.0f) * (1.0f - runK * runK);
        const float rr = 1.0f, rg = 0.30f, rb = 0.26f;
        // the ring: faint whole, bright fill, a small inner ring at the centre
        DrawEnemyArc(tx, ty, reach, 0.0f, 6.2831853f, 40, rr, rg, rb, (0.25f + 0.25f * locked) * appear, 1.2f, false);
        DrawEnemyArc(tx, ty, reach, -1.5707963f, 6.2831853f * k, 40, rr, rg, rb, 0.9f * appear, 2.2f, false);
        DrawEnemyArc(tx, ty, reach * 0.32f, 0.0f, 6.2831853f, 20, rr, rg, rb, (0.35f + 0.45f * locked) * appear, 1.4f, false);
        if (locked > 0.0f) {
            for (int t = 0; t < 4; ++t) {
                const float a = 0.7853982f + 1.5707963f * (float)t;
                drawLineQuad(tx + cosf(a) * reach * 0.62f, ty + sinf(a) * reach * 0.62f,
                             tx + cosf(a) * reach * 0.86f, ty + sinf(a) * reach * 0.86f, 1.8f, rr, rg, rb, 0.85f);
            }
        }
        // the lane: slim chevrons between the body and the ring
        const float start = outerR * 1.9f, stop = laneLen - reach * 1.15f;
        if (!running && stop > start) {
            const float march = std::fmod(visualTime * 2.0f, 1.0f);
            const float chev = base * 0.30f;
            const float strength = (0.35f + 0.45f * locked) * appear;
            for (int c = 0; c < 5; ++c) {
                const float u = ((float)c + march) / 5.0f;
                const float d = start + (stop - start) * u;
                const float cx = x + ux * d, cy = y + uy * d;
                const float a = strength * std::sin(3.14159265f * u);
                for (float side : { -1.0f, 1.0f }) {
                    const float bx = cx - ux * chev * 0.8f + px * chev * side;
                    const float by = cy - uy * chev * 0.8f + py * chev * side;
                    drawLineQuad(bx, by, cx, cy, 3.6f, rr, rg, rb, a * 0.16f);
                    drawLineQuad(bx, by, cx, cy, 1.6f, rr, rg, rb, a);
                }
            }
        }
    }
    if (m->normaPattern == norma::Pattern::Aim &&
        (m->normaStage == norma::Stage::Telegraph || m->normaStage == norma::Stage::Act)) {
        // A short aim line that tracks the player, with a reticle bracket at
        // its end; it brightens into the firing barrel.
        const bool firing = m->normaStage == norma::Stage::Act;
        const float a = m->normaAimAngle;
        const float ux = cosf(a), uy = sinf(a);
        const float r0 = half * 1.2f, r1 = half + norma::kAimLineLength;
        const float lr = vr, lg = vg, lb = vb;
        drawLineQuad(x + ux * r0, y + uy * r0, x + ux * r1, y + uy * r1,
                     firing ? 2.4f : 1.4f, lr, lg, lb, firing ? 0.55f : 0.8f);
        const float ex = x + ux * r1, ey = y + uy * r1;
        const float px = -uy, py = ux, s2 = 12.0f;
        drawLineQuad(ex + px * s2, ey + py * s2, ex + px * s2 - ux * s2, ey + py * s2 - uy * s2,
                     1.6f, lr, lg, lb, 0.85f);
        drawLineQuad(ex - px * s2, ey - py * s2, ex - px * s2 - ux * s2, ey - py * s2 - uy * s2,
                     1.6f, lr, lg, lb, 0.85f);
        drawLineQuad(ex + px * s2, ey + py * s2, ex - px * s2, ey - py * s2, 1.0f, lr, lg, lb, 0.5f);
    }
    if (m->normaStage == norma::Stage::Telegraph) {
        const float t = m->normaTimer;
        if (m->normaPattern == norma::Pattern::Refuge) {
            // The coming flood: short rays that brighten. They stop well short
            // of the screen edge (no screen-wide aim lines, per the art rules).
            const float k = std::clamp(t / norma::kRefugeTelegraph, 0.0f, 1.0f);
            const float rayEnd = half + 300.0f;
            for (int i = 0; i < 24; ++i) {
                const float a = m->normaSpin + 6.2831853f * (float)i / 24.0f;
                drawLineQuad(x + cosf(a) * half * 1.2f, y + sinf(a) * half * 1.2f,
                             x + cosf(a) * rayEnd, y + sinf(a) * rayEnd,
                             1.0f, vr, vg, vb, 0.18f + 0.30f * k);
            }
        }
    }
}

// The Genesis' Rotor-launcher shot: landing zone while it flies, the burst when it lands.
static void DrawGenesisLauncherShot(const Monster* m, float x, float y, float cr, float cg, float cb) {
    if (m->genesisRotorPhase == 2)
        DrawThrownBlastZone(m->thrownTargetX, m->thrownTargetY, 1.0f,
            std::clamp(m->thrownTimer / m->thrownDuration, 0.0f, 1.0f), Monster::GENESIS_ROTOR_BLAST_RADIUS);
    if (m->genesisRotorPhase == 3 && m->genesisRotorDetonated) {
        const float u = std::clamp(m->thrownTimer / 0.25f, 0.0f, 1.0f);
        const float radius = Monster::GENESIS_ROTOR_BLAST_RADIUS;
        drawCircle(x, y, radius, cr, cg, cb, 0.07f * (1.0f - u));
        DrawEnemyArc(x, y, radius, 0, 6.2831853f, 48, cr, cg, cb, 0.9f * (1.0f - u), 1.8f, false);
        DrawEnemyArc(x, y, radius * (0.25f + 0.75f * u), 0, 6.2831853f, 48,
                     1.0f, 0.70f, 0.30f, 1.0f - u, 2.0f, false);
    }
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
    if (m->genesisCarryScope) {
        // A capsule that opens into a Scope: it is a Scope from the moment it is thrown.
        static RangedMob proxy(0.0f, 0.0f, 1280, 720);
        if (m->thrownPhase == 1)
            DrawThrownBlastZone(m->thrownTargetX, m->thrownTargetY, 1.0f,
                                std::min(1.0f, m->thrownTimer / std::max(0.01f, m->thrownDuration)));
        if (m->genesisFromLauncher)
            DrawGenesisLauncherShot(m, m->worldX, m->worldY, m->color.r, m->color.g, m->color.b);
        proxy.worldX = m->worldX; proxy.worldY = m->worldY;
        proxy.deathScale = m->sizeScale;
        proxy.rotAngle = (m->thrownPhase == 1 || m->genesisRotorPhase > 0) ? m->thrownSpin : visualTime * RangedMob::IDLE_ROT;
        proxy.color = m->color;
        proxy.hitFlashTimer = m->hitFlashTimer;
        proxy.lensState = RangedMob::State::IDLE;
        drawRangedMob(&proxy, visualTime);
        return;
    }
    if (m->genesisFromLauncher && m->kind != MobKind::ROTOR)
        DrawGenesisLauncherShot(m, m->worldX, m->worldY, m->color.r, m->color.g, m->color.b);
    if (m->genesisSummoned && m->thrownPhase == 1 && m->kind != MobKind::SWARM)
        DrawThrownBlastZone(m->thrownTargetX, m->thrownTargetY, 1.0f,
                            std::min(1.0f, m->thrownTimer / std::max(0.01f, m->thrownDuration)));
    if (m->kind != MobKind::REGULUS && m->regulusShield.hp > 0.0f) {
        DrawAegisMark(m->worldX, m->worldY, base * AegisMarkExtent(m->kind),
                      m->regulusShield.hp / std::max(1.0f, m->regulusShield.maxHp),
                      std::clamp(m->regulusShield.flashTimer / MOB_HIT_FLASH_TIME,
                                 0.0f, 1.0f),
                      visualTime * kAegisSpin, m->regulusShield.grantTimer);
    }
    if (m->kind == MobKind::NORMA) {
        DrawNormaBody(m, base, visualTime);
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
            const float arcAlpha = 0.26f + 0.32f * infl;
            const float gap = 0.20f * (1.0f - 0.6f * infl);
            // A dark under-stroke keeps the boundary readable on bright windows.
            DrawEnemyArc(x, y, fieldR, 0.0f, 6.2831853f, 160,
                         0.0f, 0.0f, 0.0f, 0.40f, 3.6f, false);
            DrawEnemyArc(x, y, fieldR, 0.0f, 6.2831853f, 160,
                         cr, cg, cb, 0.50f + 0.18f * infl, 1.8f, false);
            DrawEnemyArc(x, y, fieldR * 0.92f, visualTime * 0.16f, 3.6f, 36,
                         cr, cg, cb, arcAlpha, 1.0f, false);
            DrawEnemyArc(x, y, fieldR * (0.92f - gap), -visualTime * 0.22f + 2.1f, 3.6f, 32,
                         cr, cg, cb, arcAlpha, 1.0f, false);
            DrawEnemyArc(x, y, fieldR * (0.92f - 2.0f * gap), visualTime * 0.30f + 4.2f, 3.6f, 28,
                         cr, cg, cb, arcAlpha * 0.8f, 0.9f, false);
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
            drawLineQuad(nodes[k].x, nodes[k].y, x + cosf(na) * orbitR, y + sinf(na) * orbitR,
                         1.15f, bodyR, bodyG, bodyB, 0.54f);
            drawLineQuad(x, y, nodes[k].x, nodes[k].y,
                         0.72f, bodyR, bodyG, bodyB, 0.12f);
        }
        for (const auto& node : nodes)
            DrawEnemyNode(node, bodyR, bodyG, bodyB, 0.94f);
        // A tilted inner orbit precesses against the hexagon. Its satellite
        // dims on the far half so the orbit reads as depth, not a flat ring.
        const float tilt = -phase * 0.42f + 0.49f;
        const float tiltX = cosf(tilt), tiltY = sinf(tilt);
        const float orbitMajor = base * 0.86f;
        const float orbitMinor = base * 0.30f;
        const float liteR = bodyR + (1.0f - bodyR) * 0.45f;
        const float liteG = bodyG + (1.0f - bodyG) * 0.45f;
        const float liteB = bodyB + (1.0f - bodyB) * 0.45f;
        DrawQuasarEllipseArc(x, y, tiltX, tiltY, orbitMajor, orbitMinor,
                             0.0f, 6.2831853f, 32,
                             liteR, liteG, liteB, 0.40f, 0.85f);
        const float satA = m->gravisOrbitPhase;
        const float satDepth = sinf(satA);
        const EnemyNodeAnchor sat{ x - tiltY * cosf(satA) * orbitMajor + tiltX * satDepth * orbitMinor,
                                   y + tiltX * cosf(satA) * orbitMajor + tiltY * satDepth * orbitMinor,
                                   base * 0.085f, 0.70f + 0.30f * (0.5f + 0.5f * satDepth) };
        if (satDepth < 0.0f) DrawEnemyNode(sat, 1.0f, 1.0f, 1.0f, 0.55f);

        drawCircle(x, y, base * 0.58f, bodyR, bodyG, bodyB,
                   0.18f + pulse * 0.08f);
        DrawGenesisCore(x, y, base * 0.34f,
                        bodyR, bodyG, bodyB, 0.96f, false);
        if (satDepth >= 0.0f) DrawEnemyNode(sat, 1.0f, 1.0f, 1.0f, 0.95f);
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
                : (m->quasarState == 3 ? 1.0f
                   : (m->quasarState == 4
                       ? 1.0f - Monster::QuasarEase(m->quasarTimer / 0.55f)   // cools down, no drop
                       : 0.0f)));

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
        float bandR = beamR;                 // current band half-width (thins while the beam fades)
        const float pre = m->QuasarPrecharge();
        auto bandEdges = [&](float x0, float y0, float x1, float y1,
                             float alpha, float width) {
            for (int e = -1; e <= 1; e += 2) {
                const float sl = std::max(1e-3f, std::hypot(x1 - x0, y1 - y0));
                const float ox = -(y1 - y0) / sl * bandR * (float)e;
                const float oy = (x1 - x0) / sl * bandR * (float)e;
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
            drawLineQuad(x0, y0, x1, y1, bandR * 2.0f, 0.0f, 0.0f, 0.0f, alpha);
        };
        if ((m->quasarState == 0 && pre > 0.0f) ||
            (m->quasarState >= 1 && m->quasarState <= 3)) {
            const float beamLen = 6000.0f;
            const float growT = m->quasarState == 3
                ? std::min(1.0f, m->quasarTimer / Monster::QUASAR_BEAM_GROW) : 1.0f;
            const float reachT = m->quasarState == 3
                ? growT * growT * growT : 1.0f;
            const float reach = beamLen * reachT;
            // Each end may bounce off the arena edge (HARD 1x, NIGHTMARE 2x);
            // every bounced leg is drawn a little dimmer than the one before.
            std::array<glm::vec4, Monster::QUASAR_BEAM_MAX_SEGMENTS> seg;
            std::array<int, Monster::QUASAR_BEAM_MAX_SEGMENTS> bnc{};
            const int segCount = m->QuasarBeamSegments(reach, seg, &bnc);
            const float batchSaved = g_BatchAlpha;
            const float ws = m->QuasarBeamWidth();                         // 1 -> 0 over the last 0.45 s
            const float lockPulse = 0.80f + 0.20f * sinf(visualTime * 18.0f);
            // Hand-off from the lock to the beam: the full-length lock band stays
            // and fades out while the beam grows into it, so nothing pops.
            if (m->quasarState == 3 && growT < 1.0f) {
                std::array<glm::vec4, Monster::QUASAR_BEAM_MAX_SEGMENTS> full;
                std::array<int, Monster::QUASAR_BEAM_MAX_SEGMENTS> fullB{};
                const int fullCount = m->QuasarBeamSegments(beamLen, full, &fullB);
                const float keep = 1.0f - Monster::QuasarEase(m->quasarTimer / 0.12f);   // the lock band clears fast so the rift reads
                for (int k = 0; k < fullCount; ++k) {
                    g_BatchAlpha = batchSaved * keep * (1.0f - 0.22f * (float)fullB[k]);
                    bandR = beamR;
                    bandShade(full[k].x, full[k].y, full[k].z, full[k].w, 0.16f);
                    drawLineQuad(full[k].x, full[k].y, full[k].z, full[k].w, beamR * 2.0f, cr, cg, cb, 0.14f);
                    drawLineQuad(full[k].x, full[k].y, full[k].z, full[k].w, 1.4f, 1.0f, 1.0f, 1.0f, 0.50f * lockPulse);
                    if (m->quasarTimer >= Monster::QUASAR_BEAM_RIFT_TIME)
                        bandEdges(full[k].x, full[k].y, full[k].z, full[k].w, 0.95f * lockPulse, 2.5f);
                }
            }
            // The rift: from the instant the beam fires, a seam runs out from the Quasar
            // and the band opens behind it (whole length, bounced legs included).
            if (m->quasarState == 3 && m->quasarTimer < Monster::QUASAR_BEAM_RIFT_TIME) {
                std::array<glm::vec4, Monster::QUASAR_BEAM_MAX_SEGMENTS> rip;
                std::array<int, Monster::QUASAR_BEAM_MAX_SEGMENTS> ripB{};
                const int ripCount = m->QuasarBeamSegments(beamLen, rip, &ripB);
                const float tt = m->quasarTimer;
                float carried = 0.0f;
                for (int k = 0; k < ripCount; ++k) {
                    if (ripB[k] == 0) carried = 0.0f;                          // a new end starts at the Quasar
                    g_BatchAlpha = batchSaved * (1.0f - 0.22f * (float)ripB[k]);
                    DrawBeamRift(rip[k].x, rip[k].y, rip[k].z, rip[k].w, beamR, tt, carried, cr, cg, cb);
                    carried += std::hypot(rip[k].z - rip[k].x, rip[k].w - rip[k].y);
                }
                g_BatchAlpha = batchSaved;
                // The origin: a ring draws in on the Quasar, then bursts back out.
                const float implode = 1.0f - Monster::QuasarEase(tt / 0.14f);
                if (tt < 0.14f)
                    DrawEnemyArc(x, y, base * (0.45f + 2.9f * implode), 0.0f, 6.2831853f, 40,
                                 1.0f, 1.0f, 1.0f, 0.45f + 0.55f * (1.0f - implode), 2.4f, false);
                if (tt >= 0.10f && tt < 0.60f) {
                    const float u = (tt - 0.10f) / 0.50f;
                    const float out = 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);
                    DrawEnemyArc(x, y, base * (0.45f + 3.2f * out), 0.0f, 6.2831853f, 48,
                                 1.0f, 1.0f, 1.0f, 0.95f * (1.0f - u), 3.2f, false);
                    DrawEnemyArc(x, y, base * (0.45f + 2.2f * out), 0.0f, 6.2831853f, 40,
                                 cr, cg, cb, 0.60f * (1.0f - u), 1.5f, false);
                }
                if (tt < 0.22f)
                    DrawEnemyArc(x, y, base * 0.34f, 0.0f, 6.2831853f, 24,
                                 1.0f, 1.0f, 1.0f, 1.0f - tt / 0.22f, 2.0f, false);
            }
            // Path distance from the Quasar to the start of each leg (the flow carries on over bounces).
            std::array<float, Monster::QUASAR_BEAM_MAX_SEGMENTS> legStart{};
            {
                float carried = 0.0f;
                for (int k = 0; k < segCount; ++k) {
                    if (bnc[k] == 0) carried = 0.0f;
                    legStart[k] = carried;
                    carried += std::hypot(seg[k].z - seg[k].x, seg[k].w - seg[k].y);
                }
            }
            for (int sIdx = 0; sIdx < segCount; ++sIdx) {
            const float x0 = seg[sIdx].x, y0 = seg[sIdx].y;
            const float x1 = seg[sIdx].z, y1 = seg[sIdx].w;
            g_BatchAlpha = batchSaved * (1.0f - 0.22f * (float)bnc[sIdx]);
            bandR = beamR;
            if (m->quasarState == 3) {
                // The beam fills the damage band exactly: body (2r), bright
                // inner column, white core, and the hard band edges. During the
                // fade-out the whole thing thins together (ws) and dims a little.
                bandR = beamR * ws;
                g_BatchAlpha *= 0.35f + 0.65f * std::sqrt(ws);
                // Breathing: ~3 times a second, +-12% (no fast flicker), and the beam bursts
                // 30% brighter at the instant it fires, settling over a quarter second.
                const float pulse = 0.88f + 0.12f * sinf(m->quasarTimer * 18.85f);
                const float flash = 1.0f + 0.30f * (1.0f - Monster::QuasarEase(m->quasarTimer / 0.25f));
                const float tt = m->quasarTimer;                              // seconds since the beam fired
                const float swell = (0.35f + 0.65f * Monster::QuasarEase(tt / 0.16f)) * ws;   // snaps open
                // The rift: the band is dark as a hole in the picture at first, then clears.
                const float rift = 1.0f - Monster::QuasarEase(tt / 0.40f);
                // Light spreading out of the beam (soft strip, fades with the distance).
                {
                    const bool finalLeg = sIdx + 1 >= segCount || bnc[sIdx + 1] != bnc[sIdx] + 1;
                    DrawBeamGlow(x0, y0, x1, y1, bandR, 72.0f * (0.55f + 0.45f * ws),
                                 std::min(0.60f, 0.36f * pulse * flash) * (0.35f + 0.65f * ws),
                                 (finalLeg && growT < 1.0f) ? 260.0f : 0.0f, cr, cg, cb);
                }
                bandShade(x0, y0, x1, y1, 0.30f + 0.25f * rift);
                drawLineQuad(x0, y0, x1, y1, bandR * 2.0f, cr, cg, cb, std::min(1.0f, 0.40f * pulse * flash));
                drawLineQuad(x0, y0, x1, y1, beamR * 0.9f * swell, cr, cg, cb, std::min(1.0f, 0.62f * pulse * flash));
                drawLineQuad(x0, y0, x1, y1, beamR * 0.40f * swell,
                             1.0f, 1.0f, 1.0f, 0.95f);
                // Energy flowing out along the beam (after the rift has opened it).
                if (tt > 0.10f && ws > 0.05f)
                    DrawBeamFlow(x0, y0, x1, y1, bandR, legStart[sIdx], tt - 0.10f, ws,
                                 bnc[sIdx] == 0 ? 10 : 6, sIdx, cr, cg, cb);
                if (tt >= Monster::QUASAR_BEAM_RIFT_TIME)      // before that the rift's own lips are the edges
                    bandEdges(x0, y0, x1, y1, 0.95f, 3.0f * (0.5f + 0.5f * ws));

                // A physical release ring marks the acceleration phase without
                // using a full-screen or core flash.
                if (sIdx == 0 && growT > 0.28f && growT < 1.0f) {
                    const float releaseT = (growT - 0.28f) / 0.72f;
                    const float ringR = base * (1.0f + releaseT * 1.9f);
                    DrawEnemyArc(x, y, ringR, 0.0f, 6.28318531f, 32,
                                 cr, cg, cb, (1.0f - releaseT) * 0.52f,
                                 2.2f, false);
                    DrawEnemyArc(x, y, ringR * 0.82f, 0.0f, 6.28318531f, 28,
                                 cr, cg, cb, (1.0f - releaseT) * 0.13f,
                                 1.1f, false);
                }
                // Leading edge: a bright head runs ahead of the growing beam so the
                // front reads as one moving thing (final leg of each end only).
                const bool lastLeg = sIdx + 1 >= segCount || bnc[sIdx + 1] != bnc[sIdx] + 1;
                if (lastLeg && growT < 1.0f) {
                    const float head = 1.0f - Monster::QuasarEase(growT);
                    DrawEnemyArc(x1, y1, beamR * (0.55f + 0.35f * head), 0.0f, 6.2831853f, 24,
                                 1.0f, 1.0f, 1.0f, 0.55f * head, 2.0f, false);
                }
            } else if (m->quasarState == 0) {
                // Wind-up: the band tracks the player; edges fade in.
                const float fade = std::min(1.0f, pre * 1.6f);
                // The warning is a quiet outline on purpose: the beam has to read as a
                // different, much stronger thing when it fires.
                bandShade(x0, y0, x1, y1, 0.08f * fade);
                drawLineQuad(x0, y0, x1, y1, 1.2f, cr, cg, cb, 0.32f * fade);
                bandEdges(x0, y0, x1, y1, 0.35f + 0.35f * fade, 1.8f);
            } else {
                // Lock: frozen band, hard edges, and a fill growing from the
                // axis out to the edges as the countdown to fire. The look eases
                // in from the wind-up band over the first 0.18 s instead of jumping.
                const float blend = m->quasarState == 2
                    ? Monster::QuasarEase(m->quasarTimer / 0.18f) : 0.0f;
                const float fill = Monster::QuasarEase(charge);
                // Quiet on purpose (see the wind-up): a barely-there fill that grows as the
                // countdown, thin edges, a hairline axis. The beam is far brighter.
                bandShade(x0, y0, x1, y1, 0.08f + 0.08f * blend);
                drawLineQuad(x0, y0, x1, y1, beamR * 2.0f * fill, cr, cg, cb,
                             0.05f + 0.09f * fill);
                drawLineQuad(x0, y0, x1, y1, 1.4f, 1.0f, 1.0f, 1.0f,
                             (0.32f + (0.50f * lockPulse - 0.32f) * blend));
                const float lockEdgeAlpha = 0.55f + ((0.78f + 0.17f * fill) * lockPulse - 0.55f) * blend;
                bandEdges(x0, y0, x1, y1, lockEdgeAlpha, 1.8f + 0.7f * blend);
                // Just before the beam fires a short seam starts to show at the Quasar
                // and a ring draws in on it: the first sign of the tear.
                const float lead = Monster::QUASAR_BEAM_RIFT_LEAD;
                const float seamUp = m->quasarState == 2
                    ? Monster::QuasarEase((m->quasarTimer - (Monster::QUASAR_LOCK_TIME - lead)) / lead) : 0.0f;
                if (seamUp > 0.0f && bnc[sIdx] == 0) {
                    const float sl = std::max(1e-3f, std::hypot(x1 - x0, y1 - y0));
                    const float reachS = std::min(sl, 70.0f + 360.0f * seamUp);
                    const float ex = x0 + (x1 - x0) / sl * reachS, ey = y0 + (y1 - y0) / sl * reachS;
                    drawLineQuad(x0, y0, ex, ey, 6.0f * seamUp + 1.0f, cr, cg, cb, 0.35f * seamUp);
                    drawLineQuad(x0, y0, ex, ey, 1.8f, 1.0f, 1.0f, 1.0f, 0.35f + 0.60f * seamUp);
                }
            }
            // Bounce point: a thin ring where this leg meets the next one.
            if (sIdx + 1 < segCount && bnc[sIdx + 1] == bnc[sIdx] + 1) {
                DrawEnemyArc(x1, y1, beamR * 0.62f, 0.0f, 6.2831853f, 24,
                             1.0f, 1.0f, 1.0f, 0.75f, 1.6f, false);
                DrawEnemyArc(x1, y1, beamR * 0.30f, 0.0f, 6.2831853f, 16,
                             cr, cg, cb, 0.85f, 1.6f, false);
            }
            }
            g_BatchAlpha = batchSaved;
        }

        DrawQuasarBody(m, base, visualTime, charge, cr, cg, cb);
        DrawQuasarFall(m, cr, cg, cb);
        DrawQuasarEruption(m, cr, cg, cb);
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
        const float savedBatchAlpha = g_BatchAlpha;
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
        DrawRegulusExtension(m, base, phase, bodyR, bodyG, bodyB, visualTime);
        g_BatchAlpha = savedBatchAlpha;
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
        // A free Swarm's core is the warning red; one a Genesis controls keeps the controlled colour.
        DrawEnemyCore(x, y, base * 0.28f, cr, cg, cb, swarmCoreA, m->color != Monster::GENESIS_CONTROLLED_COLOR);
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
        else if(m->genesisThrown && (m->genesisFromLauncher || !m->genesisSummoned) && m->thrownDuration>0) {
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
