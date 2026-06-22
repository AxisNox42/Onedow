#pragma once
#include <glad/glad.h>
#include <cmath>
#include <vector>
#include <cstdint>
#include "Settings.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ─────────────────────────────────────────────────────────────
// 픽셀 좌표계용 도형 그리기 — 드로콜 배칭 버전
//   모든 drawX 는 즉시 그리지 않고 CPU 버퍼(g_Batch)에 삼각형을 쌓는다.
//   BatchFlush() 가 한 번에 업로드+드로 (정점당 [pos.xy, col.rgba] = 6 float).
//   상태(scissor/blend/ortho)나 셰이더(text/icon)가 바뀌기 직전에 반드시 Flush.
// ─────────────────────────────────────────────────────────────
enum class GfxPass : uint8_t { Main, Text, Icon };
inline GfxPass g_GfxPass = GfxPass::Main;

inline GLint  g_colorLoc = -1;   // (배칭 후 미사용 — 호환 위해 유지)
inline GLuint g_VBO      = 0;
inline size_t g_BatchVBOFloats = 65536;

inline GLuint g_MainShader  = 0;
inline GLuint g_MainVAO     = 0;
inline GLint  g_MainProjLoc = -1;
inline float  g_MainOrtho[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

inline std::vector<float> g_Batch;   // [x,y,r,g,b,a] × n

inline void BindMainShader() {
    if (g_MainShader == 0) return;
    g_GfxPass = GfxPass::Main;
    glUseProgram(g_MainShader);
    if (g_MainProjLoc >= 0)
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_MainOrtho);
    glBindVertexArray(g_MainVAO);
}

// 쌓인 삼각형을 한 번에 그린다. 메인 셰이더/VAO/ortho 를 재바인드하므로
//   text/icon 등 다른 셰이더가 바인드된 상태에서 불려도 안전.
inline void BatchFlush() {
    if (g_Batch.empty()) return;
    if (g_MainShader) {
        glUseProgram(g_MainShader);
        if (g_MainProjLoc >= 0)
            glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_MainOrtho);
        glBindVertexArray(g_MainVAO);
    }
    const GLsizeiptr nbytes = (GLsizeiptr)(g_Batch.size() * sizeof(float));
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    if (g_Batch.size() <= g_BatchVBOFloats)
        glBufferSubData(GL_ARRAY_BUFFER, 0, nbytes, g_Batch.data());
    else {
        glBufferData(GL_ARRAY_BUFFER, nbytes, g_Batch.data(), GL_DYNAMIC_DRAW);
        g_BatchVBOFloats = g_Batch.size();
    }
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(g_Batch.size() / 6));
    g_Batch.clear();
    g_GfxPass = GfxPass::Main;
}

inline void BatchVtx(float x, float y, float r, float g, float b, float a) {
    g_Batch.push_back(x); g_Batch.push_back(y);
    g_Batch.push_back(r); g_Batch.push_back(g);
    g_Batch.push_back(b); g_Batch.push_back(a);
}
inline void BatchTri(float x1, float y1, float x2, float y2, float x3, float y3,
                     float r, float g, float b, float a) {
    BatchVtx(x1, y1, r, g, b, a);
    BatchVtx(x2, y2, r, g, b, a);
    BatchVtx(x3, y3, r, g, b, a);
}
// 원시 정점 배열(x,y 쌍 n개, 삼각형 리스트)을 단색으로 배치에 추가
inline void BatchVerts(const float* v, int n, float r, float g, float b, float a) {
    for (int i = 0; i < n; i++) BatchVtx(v[i*2], v[i*2+1], r, g, b, a);
}

inline void drawRect(float x, float y, float w, float h,
                     float r, float g, float b, float a) {
    BatchTri(x, y, x+w, y, x+w, y+h, r, g, b, a);
    BatchTri(x, y, x+w, y+h, x, y+h, r, g, b, a);
}

inline void drawTriangle(float cx, float cy, float size,
                         float r, float g, float b, float a) {
    float hs = size * 0.5f;
    BatchTri(cx, cy - hs, cx - hs, cy + hs, cx + hs, cy + hs, r, g, b, a);
}

inline void drawCircle(float cx, float cy, float radius,
                       float r, float g, float b, float a) {
    const int SEG = GfxCircleSegs();
    float px = cx + radius, py = cy;   // theta=0
    for (int s = 1; s <= SEG; s++) {
        float th = (float)s / SEG * 2.0f * (float)M_PI;
        float nx = cx + radius * cosf(th), ny = cy + radius * sinf(th);
        BatchTri(cx, cy, px, py, nx, ny, r, g, b, a);
        px = nx; py = ny;
    }
}

// 부채꼴(파이 슬라이스) — angCenter±halfArc
inline void drawConeFan(float cx, float cy, float radius,
                        float angCenter, float halfArc,
                        float r, float g, float b, float a) {
    const int SEG = GfxArcSegs();
    float th0 = angCenter - halfArc;
    float px = cx + radius * cosf(th0), py = cy + radius * sinf(th0);
    for (int s = 1; s <= SEG; s++) {
        float t  = (float)s / SEG;
        float th = angCenter - halfArc + 2.0f * halfArc * t;
        float nx = cx + radius * cosf(th), ny = cy + radius * sinf(th);
        BatchTri(cx, cy, px, py, nx, ny, r, g, b, a);
        px = nx; py = ny;
    }
}

// Mercedes 로고 — 외곽 원(어두움) + 3-pointed star(밝음)
inline void drawMercedes(float cx, float cy, float size,
                         float r, float g, float b, float a) {
    const int SEG = GfxArcSegs(1.67f);
    float dr = r*0.55f, dg = g*0.55f, db = b*0.55f;
    float px = cx + size, py = cy;
    for (int s = 1; s <= SEG; s++) {
        float th = (float)s / SEG * 2.0f * (float)M_PI;
        float nx = cx + size * cosf(th), ny = cy + size * sinf(th);
        BatchTri(cx, cy, px, py, nx, ny, dr, dg, db, a);
        px = nx; py = ny;
    }
    for (int p = 0; p < 3; p++) {
        float angle = -(float)M_PI / 2.0f + (float)p * 2.0f * (float)M_PI / 3.0f;
        float tx = cx + cosf(angle) * size * 0.92f;
        float ty = cy + sinf(angle) * size * 0.92f;
        float baseW = size * 0.16f;
        float perpX = -sinf(angle), perpY = cosf(angle);
        BatchTri(tx, ty, cx + perpX*baseW, cy + perpY*baseW,
                 cx - perpX*baseW, cy - perpY*baseW, r, g, b, a);
    }
}

inline void drawPentagon(float cx, float cy, float size,
                         float r, float g, float b, float a) {
    const int N = 5;
    float hs = size * 0.5f;
    float th0 = -(float)M_PI / 2.0f;
    float px = cx + cosf(th0) * hs, py = cy + sinf(th0) * hs;
    for (int i = 1; i <= N; i++) {
        float th = -(float)M_PI / 2.0f + (float)i * 2.0f * (float)M_PI / N;
        float nx = cx + cosf(th) * hs, ny = cy + sinf(th) * hs;
        BatchTri(cx, cy, px, py, nx, ny, r, g, b, a);
        px = nx; py = ny;
    }
}

inline void drawDiamond(float cx, float cy, float size,
                        float r, float g, float b, float a) {
    float hs = size * 0.5f;
    float t = cy - hs, bo = cy + hs, le = cx - hs, ri = cx + hs;
    BatchTri(cx, cy, cx, t,  ri, cy, r, g, b, a);   // 상-우
    BatchTri(cx, cy, ri, cy, cx, bo, r, g, b, a);   // 우-하
    BatchTri(cx, cy, cx, bo, le, cy, r, g, b, a);   // 하-좌
    BatchTri(cx, cy, le, cy, cx, t,  r, g, b, a);   // 좌-상
}

// 사이버펑크 네온 터미널 보더 — 또렷한 라인 + 코너 브래킷
inline void drawNeonBorder(float x, float y, float w, float h,
                           float nr, float ng, float nb) {
    const float t = 1.5f;
    drawRect(x, y,       w, t, nr, ng, nb, 0.92f);
    drawRect(x, y+h-t,   w, t, nr, ng, nb, 0.92f);
    drawRect(x, y,       t, h, nr, ng, nb, 0.92f);
    drawRect(x+w-t, y,   t, h, nr, ng, nb, 0.92f);
    const float cl = 14.0f, ct = 2.5f;
    drawRect(x, y,            cl, ct, nr, ng, nb, 1.0f);  drawRect(x, y,            ct, cl, nr, ng, nb, 1.0f);
    drawRect(x+w-cl, y,       cl, ct, nr, ng, nb, 1.0f);  drawRect(x+w-ct, y,       ct, cl, nr, ng, nb, 1.0f);
    drawRect(x, y+h-ct,       cl, ct, nr, ng, nb, 1.0f);  drawRect(x, y+h-cl,       ct, cl, nr, ng, nb, 1.0f);
    drawRect(x+w-cl, y+h-ct,  cl, ct, nr, ng, nb, 1.0f);  drawRect(x+w-ct, y+h-cl,  ct, cl, nr, ng, nb, 1.0f);
}
