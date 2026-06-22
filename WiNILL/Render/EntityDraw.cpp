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
    if (!b.isEnemy) {
        float trailLen = b.speed * 0.020f;
        if (trailLen > 5.0f) {
            float tx = b.x - b.dirX * trailLen;
            float ty = b.y - b.dirY * trailLen;
            float px = -b.dirY * r, py = b.dirX * r;
            float v[6] = { b.x + px, b.y + py, b.x - px, b.y - py, tx, ty };
            BatchVerts(v, 3, b.color.r, b.color.g, b.color.b, 0.38f);
        }
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
    if      (m->kind == MobKind::DDOS)      MarkMobSeenId(CM_DDOS);
    else if (m->kind == MobKind::BADSECTOR) MarkMobSeenId(CM_BADSECTOR);
    else if (m->kind == MobKind::REGERROR)  MarkMobSeenId(CM_REGERROR);
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
    if (m->kind == MobKind::SPLITTER) {
        drawCircle(m->worldX, m->worldY, base, m->color.r, m->color.g, m->color.b, 1.0f);
        drawCircle(m->worldX, m->worldY, base*0.42f, 0.05f, 0.22f, 0.08f, 0.9f);
        if (m->burnTimer > 0.0f) {
            float pulse = 0.35f + 0.25f * sinf((float)glfwGetTime() * 14.0f);
            drawCircle(m->worldX, m->worldY, base * 1.25f, 1.0f, 0.55f, 0.12f, pulse);
        }
    } else if (m->kind == MobKind::BLINKER) {
        if (m->blinkWarn) {
            float a = 0.18f + 0.22f * (m->blinkWarnT / Monster::BLINK_WARN);
            drawDiamond(m->blinkTargetX, m->blinkTargetY, base*1.15f,
                        m->color.r, m->color.g, m->color.b, a);
        }
        drawDiamond(m->worldX, m->worldY, base, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(m->worldX, m->worldY, base*0.4f, 1.0f, 1.0f, 1.0f, 0.9f);
    } else if (m->kind == MobKind::CHARGER) {
        if (m->chargeState == 1) {
            float p = 0.5f + 0.5f * sinf((float)glfwGetTime() * 28.0f);
            drawTriangle(m->worldX, m->worldY, base * (1.5f + 0.4f * p),
                         1.0f, 0.85f, 0.25f, 0.35f);
        } else if (m->chargeState == 2) {
            drawCircle(m->worldX, m->worldY, base * 1.2f, 1.0f, 0.5f, 0.1f, 0.35f);
        }
        drawTriangle(m->worldX, m->worldY, base, m->color.r, m->color.g, m->color.b, 1.0f);
        drawTriangle(m->worldX, m->worldY, base*0.4f, 1.0f, 0.95f, 0.7f, 0.9f);
    } else if (m->kind == MobKind::WEAVER) {
        drawDiamond(m->worldX, m->worldY, base*0.95f, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(m->worldX, m->worldY, base*0.35f, 1.0f, 1.0f, 1.0f, 0.85f);
    } else if (m->kind == MobKind::BRUTE) {
        float x = m->worldX, y = m->worldY;
        drawDiamond(x, y, base*1.15f, m->color.r*0.55f, m->color.g*0.55f, m->color.b*0.55f, 1.0f);
        drawDiamond(x, y, base*0.85f, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(x, y, base*0.40f, 1.0f, 0.6f, 0.45f, 0.95f);
        float s = base*0.28f, o = base*0.50f;
        drawRect(x - o - s*0.5f, y - s*0.5f, s, s, 0.2f, 0.04f, 0.05f, 1.0f);
        drawRect(x + o - s*0.5f, y - s*0.5f, s, s, 0.2f, 0.04f, 0.05f, 1.0f);
        drawRect(x - s*0.5f, y - o - s*0.5f, s, s, 0.2f, 0.04f, 0.05f, 1.0f);
        drawRect(x - s*0.5f, y + o - s*0.5f, s, s, 0.2f, 0.04f, 0.05f, 1.0f);
    } else if (m->kind == MobKind::ORBITER) {
        float x = m->worldX, y = m->worldY;
        float bw = base*1.7f, th = base*0.42f;
        drawRect(x - bw*0.5f, y - th*0.5f, bw, th, m->color.r, m->color.g, m->color.b, 1.0f);
        drawRect(x - th*0.5f, y - bw*0.5f, th, bw, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(x, y, base*0.6f, 1.0f, 1.0f, 0.85f, 0.95f);
    } else if (m->kind == MobKind::SPAWNER) {
        float x = m->worldX, y = m->worldY;
        float ph = (float)glfwGetTime() * 1.2f;
        for (int k = 0; k < 3; k++) {
            float a = ph + (float)k * 2.0944f;
            drawTriangle(x + cosf(a) * base*1.35f, y + sinf(a) * base*1.35f,
                         base*0.45f, m->color.r*1.3f, m->color.g*1.2f, m->color.b*1.2f, 0.95f);
        }
        drawDiamond(x, y, base*1.05f, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(x, y, base*0.5f, 0.04f, 0.18f, 0.13f, 0.95f);
    } else if (m->kind == MobKind::SHIELDED) {
        float x = m->worldX, y = m->worldY;
        if (m->shieldActive) {
            float ang = atan2f(m->dashDirY, m->dashDirX);
            float pulse = 0.4f + 0.15f * sinf((float)glfwGetTime() * 8.0f);
            drawConeFan(x, y, base*2.0f, ang, 1.0f, 0.35f, 0.75f, 1.0f, pulse);
        }
        drawDiamond(x, y, base, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(x, y, base*0.4f, 1.0f, 1.0f, 1.0f, 0.85f);
    } else if (m->kind == MobKind::DDOS) {
        drawTriangle(m->worldX, m->worldY, base, m->color.r, m->color.g, m->color.b, 1.0f);
        drawTriangle(m->worldX, m->worldY, base*0.42f, 1.0f, 0.85f, 0.9f, 0.9f);
    } else if (m->kind == MobKind::BADSECTOR) {
        float x = m->worldX, y = m->worldY;
        for (int ring = 0; ring < 2; ring++) {
            float rr = base * (ring == 0 ? 1.0f : 0.5f);
            float cr = ring == 0 ? m->color.r : 0.1f;
            float cg = ring == 0 ? m->color.g : 0.05f;
            float cb = ring == 0 ? m->color.b : 0.2f;
            float vx[6], vy[6];
            for (int s = 0; s < 6; s++) {
                float a = (float)s * 1.0471976f + 0.5236f;
                vx[s] = x + cosf(a) * rr; vy[s] = y + sinf(a) * rr;
            }
            for (int s = 0; s < 6; s++) {
                int n = (s + 1) % 6;
                float v[6] = { x, y, vx[s], vy[s], vx[n], vy[n] };
                BatchVerts(v, 3, cr, cg, cb, 1.0f);
            }
        }
    } else if (m->kind == MobKind::REGERROR) {
        float x = m->worldX, y = m->worldY;
        float ww = 290.0f;
        drawRect(x - ww*0.5f, y - ww*0.5f, ww, ww, 0.5f, 0.1f, 0.1f, 0.09f);
        drawNeonBorder(x - ww*0.5f, y - ww*0.5f, ww, ww, 0.9f, 0.3f, 0.3f);
        float ph = (float)glfwGetTime() * 1.6f;
        for (int k = 0; k < 4; k++) {
            float a  = ph + (float)k * 1.5708f;
            float sx = x + cosf(a) * base * 1.5f;
            float sy = y + sinf(a) * base * 1.5f;
            float s  = base * 0.36f;
            drawRect(sx - s*0.5f, sy - s*0.5f, s, s, 1.0f, 0.55f, 0.2f, 0.95f);
        }
        float xr = ph * 0.6f;
        for (int d = 0; d < 2; d++) {
            float a = 0.7854f + (float)d * 1.5708f + xr;
            float dx = cosf(a), dy = sinf(a), px = -dy, py = dx;
            float L = base, T = base * 0.28f;
            float v1x=x+dx*L+px*T, v1y=y+dy*L+py*T, v2x=x+dx*L-px*T, v2y=y+dy*L-py*T;
            float v3x=x-dx*L+px*T, v3y=y-dy*L+py*T, v4x=x-dx*L-px*T, v4y=y-dy*L-py*T;
            float va[12]={v1x,v1y,v2x,v2y,v3x,v3y, v2x,v2y,v4x,v4y,v3x,v3y};
            BatchVerts(va, 6, m->color.r, m->color.g, m->color.b, 1.0f);
        }
        drawCircle(x, y, base*0.32f, 1.0f, 0.9f, 0.6f, 1.0f);
    } else {
        float cr = m->color.r, cg = m->color.g, cb = m->color.b;
        ApplyMobStyleTint(cr, cg, cb);
        drawTriangle(m->worldX, m->worldY, base, cr, cg, cb, 1.0f);
    }
    if (m->burnTimer > 0.0f && m->kind != MobKind::SPLITTER) {
        float pulse = 0.35f + 0.25f * sinf((float)glfwGetTime() * 14.0f);
        drawCircle(m->worldX, m->worldY, base * 1.2f, 1.0f, 0.55f, 0.12f, pulse);
    }
}
