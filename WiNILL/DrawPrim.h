#pragma once
#include <glad/glad.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ─────────────────────────────────────────────────────────────
// 픽셀 좌표계용 즉석 도형 그리기 헬퍼
//   - g_VBO / g_colorLoc 는 메인이 init 후 세팅 (호출 전에 반드시 값 있어야)
//   - 호출하는 쪽에서 적절한 shader/VAO 가 bind 되어 있어야 함
// ─────────────────────────────────────────────────────────────
inline GLint  g_colorLoc = -1;
inline GLuint g_VBO      = 0;

// 메인 쉐이더 (TextRenderer 가 shader 를 swap 한 후 rebind 용)
inline GLuint g_MainShader  = 0;
inline GLuint g_MainVAO     = 0;
inline GLint  g_MainProjLoc = -1;
inline float  g_MainOrtho[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

// 메뉴/버튼 등 텍스트와 섞어 그릴 때 매 drawRect 전에 호출
inline void BindMainShader() {
    if (g_MainShader == 0) return;
    glUseProgram(g_MainShader);
    if (g_MainProjLoc >= 0)
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_MainOrtho);
    glBindVertexArray(g_MainVAO);
}

inline void drawRect(float x, float y, float w, float h,
                     float r, float g, float b, float a) {
    float v[] = {
        x,   y,    x+w, y,    x+w, y+h,
        x,   y,    x+w, y+h,  x,   y+h
    };
    glUniform4f(g_colorLoc, r, g, b, a);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

inline void drawTriangle(float cx, float cy, float size,
                         float r, float g, float b, float a) {
    float hs = size * 0.5f;
    float v[] = {
        cx,      cy - hs,
        cx - hs, cy + hs,
        cx + hs, cy + hs
    };
    glUniform4f(g_colorLoc, r, g, b, a);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

inline void drawCircle(float cx, float cy, float radius,
                       float r, float g, float b, float a) {
    const int SEG = 20;
    float v[(SEG + 2) * 2];
    int i = 0;
    v[i++] = cx; v[i++] = cy;
    for (int s = 0; s <= SEG; s++) {
        float theta = (float)s / SEG * 2.0f * (float)M_PI;
        v[i++] = cx + radius * cosf(theta);
        v[i++] = cy + radius * sinf(theta);
    }
    glUniform4f(g_colorLoc, r, g, b, a);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLE_FAN, 0, SEG + 2);
}

// Mercedes-Benz 로고 — 외곽 원 + 안에 3-pointed star (위·좌하·우하)
// size = 외곽 원 반지름
inline void drawMercedes(float cx, float cy, float size,
                         float r, float g, float b, float a) {
    // 외곽 원 (어두운 톤)
    glUniform4f(g_colorLoc, r * 0.55f, g * 0.55f, b * 0.55f, a);
    {
        const int SEG = 28;
        float v[(SEG + 2) * 2];
        int i = 0; v[i++] = cx; v[i++] = cy;
        for (int s = 0; s <= SEG; s++) {
            float theta = (float)s / SEG * 2.0f * (float)M_PI;
            v[i++] = cx + size * cosf(theta);
            v[i++] = cy + size * sinf(theta);
        }
        glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
        glDrawArrays(GL_TRIANGLE_FAN, 0, SEG + 2);
    }
    // 3-pointed star (밝은 톤, 좁고 긴 삼각형 3개)
    glUniform4f(g_colorLoc, r, g, b, a);
    for (int p = 0; p < 3; p++) {
        float angle = -(float)M_PI / 2.0f + (float)p * 2.0f * (float)M_PI / 3.0f;
        // 끝점 (원 가까이)
        float tx = cx + cosf(angle) * size * 0.92f;
        float ty = cy + sinf(angle) * size * 0.92f;
        // 베이스 (중심에서 perpendicular)
        float baseW = size * 0.16f;
        float perpX = -sinf(angle), perpY = cosf(angle);
        float b1x = cx + perpX * baseW, b1y = cy + perpY * baseW;
        float b2x = cx - perpX * baseW, b2y = cy - perpY * baseW;
        float v[6] = { tx, ty, b1x, b1y, b2x, b2y };
        glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

inline void drawPentagon(float cx, float cy, float size,
                         float r, float g, float b, float a) {
    const int N = 5;
    float hs = size * 0.5f;
    float v[(N + 2) * 2]; // center + N + close
    v[0] = cx; v[1] = cy;
    for (int i = 0; i <= N; i++) {
        float theta = -(float)M_PI / 2.0f + (float)i * 2.0f * (float)M_PI / N; // 위쪽이 꼭짓점
        v[2 + i*2]     = cx + cosf(theta) * hs;
        v[2 + i*2 + 1] = cy + sinf(theta) * hs;
    }
    glUniform4f(g_colorLoc, r, g, b, a);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLE_FAN, 0, N + 2);
}

inline void drawDiamond(float cx, float cy, float size,
                        float r, float g, float b, float a) {
    float hs = size * 0.5f;
    float v[] = {
        cx,      cy,        // center
        cx,      cy - hs,   // top
        cx + hs, cy,        // right
        cx,      cy + hs,   // bottom
        cx - hs, cy,        // left
        cx,      cy - hs    // close fan
    };
    glUniform4f(g_colorLoc, r, g, b, a);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
}
