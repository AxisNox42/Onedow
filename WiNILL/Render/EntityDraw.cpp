#include "EntityDraw.h"
#include "Camera.h"
#include "DrawPrim.h"
#include "IconSystem.h"
#include "Codex.h"
#include "TextRenderer.h"
#include "UiColors.h"
#include "Settings.h"
#include <GLFW/glfw3.h>
#include <cmath>

extern TextRenderer g_TextS;

static void ApplyMobStyleTint(float& r, float& g, float& b) {
    if (g_MobVisualStyle != MobVisualStyle::SOFT) return;
    r = r * 0.72f + 0.14f;
    g = g * 0.72f + 0.16f;
    b = b * 0.72f + 0.20f;
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

static void drawRotTriangle(float cx, float cy, float size, float angle,
                            float r, float g, float b, float a) {
    float rr = size * 0.58f;
    float a0 = angle - 1.5708f;
    float a1 = angle + 0.5236f;
    float a2 = angle + 2.6180f;
    BatchTri(cx + cosf(a0) * rr, cy + sinf(a0) * rr,
             cx + cosf(a1) * rr, cy + sinf(a1) * rr,
             cx + cosf(a2) * rr, cy + sinf(a2) * rr,
             r, g, b, a);
}

void DrawApproachOrb(float x, float y) {
    drawRectCol3(x - 18.0f, y - 18.0f, 36.0f, 36.0f, UiCol::APPROACH_ORB_OUTER, 0.30f);
    drawRectCol3(x - 14.0f, y - 14.0f, 28.0f, 28.0f, UiCol::APPROACH_ORB_INNER, 1.0f);
}

void DrawAppWindow(float wx, float wy, float w, float h, const wchar_t* title, float tb) {
    BatchFlush(); glDisable(GL_BLEND);
    drawRect(wx, wy, w, h, 0.05f, 0.05f, 0.09f, 0.88f);
    BatchFlush(); glEnable(GL_BLEND);
    drawRect(wx, wy, w, tb, 0.45f, 0.18f, 0.70f, 1.0f);
    drawNeonBorder(wx, wy, w, h, 0.6f, 0.3f, 0.95f);
    BatchFlush();
    if (title) {
        float tScale = (tb <= 16.0f) ? 0.38f : 0.5f;
        float ty = wy + (tb - 11.0f * tScale) * 0.5f;
        if (ty < wy + 1.0f) ty = wy + 1.0f;
        g_TextS.Draw(title, wx + 6.0f, ty, tScale, 1.0f, 0.95f, 1.0f, 1.0f);
    }
    BatchFlush();
}

bool inWin(float x, float y, float rx, float ry, float rw, float rh, float margin) {
    return x >= rx - margin && x <= rx + rw + margin &&
           y >= ry - margin && y <= ry + rh + margin;
}

void drawBullet(const Bullet& b) {
    float r = 6.0f * b.sizeScale;
    if (b.rainMissile && g_RainMissileTex) {
        float z   = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
        float sx  = W2SX(b.x), sy = W2SY(b.y);
        float ang = std::atan2(b.dirX, -b.dirY);
        float hw  = 13.0f * z, hh = 17.0f * z;
        drawCircle(b.x - b.dirX * 10.0f, b.y - b.dirY * 10.0f, r * 1.3f,
                   1.0f, 0.6f, 0.2f, 0.5f);
        BatchFlush();
        DrawIconRot(g_RainMissileTex, sx, sy, hw, hh, ang,
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
    drawCircle(b.x, b.y, r, b.color.r, b.color.g, b.color.b, 1.0f);
}

void SpawnWormSplit(Monster* m, std::vector<Monster*>& born) {
    if (m->kind != MobKind::SPLITTER) return;
    extern PlayerStats g_Stats;
    int maxGen = g_Stats.splitterBoost ? 3 : 2;
    if (m->splitGen >= maxGen) return;
    for (int c = 0; c < 2; c++) {
        Monster* ch = new Monster(m->worldX + (c ? 28.0f : -28.0f), m->worldY,
                                  1.0f, 1.0f, false);
        ch->MakeKind(MobKind::SPLITTER, m->splitGen + 1, m->sizeScale * 0.7f);
        born.push_back(ch);
    }
}

void drawMob(const Monster* m) {
    MarkMobSeen(m->kind);
    if (m->kind == MobKind::DDOS) MarkMobSeenId(CM_DDOS);
    float base = (m->summoned ? 28.0f : 18.0f) * m->sizeScale;
    if (m->elite) {
        float gr, gg, gb;
        if (m->elite == 1)      { gr = 0.35f; gg = 0.9f;  gb = 1.0f; }
        else if (m->elite == 2) { gr = 1.0f;  gg = 0.85f; gb = 0.2f; }
        else                    { gr = 1.0f;  gg = 0.3f;  gb = 0.1f; }
        float pulse = (m->elite == 3)
                    ? (0.5f + 0.5f * sinf((float)glfwGetTime() * 10.0f)) : 0.55f;
        drawCircle(m->worldX, m->worldY, base * 1.85f, gr, gg, gb, 0.10f + 0.16f * pulse);
        drawCircle(m->worldX, m->worldY, base * 1.35f, gr, gg, gb, 0.14f + 0.14f * pulse);
    }
    if (m->kind == MobKind::SPAWNER) {
        // Hive: 6 bracket [ ] fragments, orbit expands during OPEN/SPAWN phase
        float x = m->worldX, y = m->worldY;
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        float t = (float)glfwGetTime();
        float ph = t * 0.35f;
        float expand = 1.0f + m->hiveOpenFactor * 1.1f;   // 1.0 → 2.1 when open
        float Ro = base * 1.55f * expand;
        float bh = base * 0.38f;
        float cl = base * (0.28f + m->hiveOpenFactor * 0.18f);  // caps grow inward when open
        // SPAWN phase: pulse glow on brackets
        float spawnFlash = (m->hivePhase == 2)
            ? (0.5f + 0.5f * sinf(t * 20.0f)) : 0.0f;
        for (int k = 0; k < 6; k++) {
            float a  = ph + (float)k * 1.0472f;
            float ox = x + cosf(a) * Ro;
            float oy = y + sinf(a) * Ro;
            float tx = -sinf(a), ty = cosf(a);
            float rx = -cosf(a), ry = -sinf(a);
            float bx1 = ox + tx*bh, by1 = oy + ty*bh;
            float bx2 = ox - tx*bh, by2 = oy - ty*bh;
            float coreA = 0.88f + spawnFlash * 0.12f;
            // back edge
            drawLineQuad(bx1,by1, bx2,by2, 3.5f, cr,cg,cb, 0.10f + spawnFlash*0.12f);
            drawLineQuad(bx1,by1, bx2,by2, 1.2f, cr,cg,cb, coreA);
            // end caps
            drawLineQuad(bx1,by1, bx1+rx*cl,by1+ry*cl, 3.5f, cr,cg,cb, 0.10f + spawnFlash*0.12f);
            drawLineQuad(bx1,by1, bx1+rx*cl,by1+ry*cl, 1.2f, cr,cg,cb, coreA);
            drawLineQuad(bx2,by2, bx2+rx*cl,by2+ry*cl, 3.5f, cr,cg,cb, 0.10f + spawnFlash*0.12f);
            drawLineQuad(bx2,by2, bx2+rx*cl,by2+ry*cl, 1.2f, cr,cg,cb, coreA);
        }
        // Core — red during SPAWN, normal otherwise
        float coreR = (m->hivePhase == 2) ? (0.9f + spawnFlash*0.1f) : 0.82f;
        float coreG = (m->hivePhase == 2) ? (0.3f - spawnFlash*0.2f) : 1.0f;
        float coreBl = (m->hivePhase == 2) ? 0.2f : 0.92f;
        drawDiamond(x, y, base*0.22f, coreR, coreG, coreBl, 0.95f);
    } else if (m->kind == MobKind::DDOS) {
        // Node: thin 1px diamond wireframe ◇, slow CW drift
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        float x = m->worldX, y = m->worldY;
        float phOff = (float)((size_t)m % 628) * 0.01f;
        float ph  = (float)glfwGetTime() * 0.524f + phOff + 0.785f;
        float R   = base * 0.95f;
        float vx[4], vy[4];
        for (int k = 0; k < 4; k++) {
            float a = ph + (float)k * 1.5708f;
            vx[k] = x + cosf(a) * R;
            vy[k] = y + sinf(a) * R;
        }
        for (int k = 0; k < 4; k++) {
            int kn = (k+1)%4;
            drawLineQuad(vx[k],vy[k], vx[kn],vy[kn], 0.9f, cr,cg,cb, 0.70f);
        }
        drawDiamond(x, y, base*0.08f, 0.80f, 0.08f, 0.08f, 0.90f);
    } else {
        // Rotor (NORMAL): 8-vertex star outline, fast CCW rotation
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        ApplyMobStyleTint(cr, cg, cb);
        float x = m->worldX, y = m->worldY;
        float phOff = (float)((size_t)m % 628) * 0.01f;
        float ph  = (float)glfwGetTime() * -5.236f + phOff;
        float R1  = base * 1.0f;   // outer tips
        float R2  = base * 0.38f;  // inner concave points
        float svx[8], svy[8];
        for (int k = 0; k < 8; k++) {
            float a = ph + (float)k * 0.7854f;   // 45° steps
            float r = (k % 2 == 0) ? R1 : R2;
            svx[k] = x + cosf(a) * r;
            svy[k] = y + sinf(a) * r;
        }
        for (int k = 0; k < 8; k++) {
            int kn = (k+1)%8;
            drawLineQuad(svx[k],svy[k], svx[kn],svy[kn], 3.5f, cr,cg,cb, 0.12f);
            drawLineQuad(svx[k],svy[k], svx[kn],svy[kn], 1.2f, cr,cg,cb, 0.88f);
        }
    }
    if (m->burnTimer > 0.0f) {
        float pulse = 0.35f + 0.25f * sinf((float)glfwGetTime() * 14.0f);
        drawCircle(m->worldX, m->worldY, base * 1.2f, 1.0f, 0.55f, 0.12f, pulse);
    }
}
