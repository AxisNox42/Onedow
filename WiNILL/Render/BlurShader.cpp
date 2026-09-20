#include "BlurShader.h"
#include "MainShader.h"   // CompileGlShader, g_BaseOrtho
#include "DrawPrim.h"     // BatchFlush, BindMainShader, g_MainVAO, g_VBO
#include <cstdio>
#include <cstring>
#include <algorithm>

static int    s_W = 1, s_H = 1;
static int    s_qW = 1, s_qH = 1;      // quarter-res dimensions
static GLuint s_fboA = 0, s_texA = 0;  // quarter-res ping-pong A (holds final result)
static GLuint s_fboB = 0, s_texB = 0;  // quarter-res ping-pong B
static GLuint s_blurProg = 0;
static GLuint s_blitProg = 0;

static GLint s_blurTexLoc  = -1;
static GLint s_blurDirLoc  = -1;
static GLint s_blitTexLoc  = -1;
static GLint s_blitProjLoc = -1;
static GLint s_blitAlpLoc  = -1;
static GLint s_blitTintLoc = -1;

// ── 셰이더 소스 ────────────────────────────────────────────────────────

static const char* kBlurVS = R"glsl(
#version 330 core
layout(location=0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)glsl";

// 9-tap 분리 가우시안 블러
static const char* kBlurFS = R"glsl(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
uniform vec2 uDir;
void main() {
    vec4 c = vec4(0.0);
    c += texture(uTex, vUV - uDir * 4.0) * 0.0162;
    c += texture(uTex, vUV - uDir * 3.0) * 0.0540;
    c += texture(uTex, vUV - uDir * 2.0) * 0.1216;
    c += texture(uTex, vUV - uDir      ) * 0.1945;
    c += texture(uTex, vUV             ) * 0.2270;
    c += texture(uTex, vUV + uDir      ) * 0.1945;
    c += texture(uTex, vUV + uDir * 2.0) * 0.1216;
    c += texture(uTex, vUV + uDir * 3.0) * 0.0540;
    c += texture(uTex, vUV + uDir * 4.0) * 0.0162;
    FragColor = c;
}
)glsl";

// 화면 좌표 쿼드, UV는 color 슬롯(.xy)에 패킹
static const char* kBlitVS = R"glsl(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec4 aColor;
uniform mat4 uProj;
out vec2 vUV;
void main() {
    vUV = aColor.xy;
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
}
)glsl";

// 블러 결과 + 틴트 (frosted glass)
static const char* kBlitFS = R"glsl(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
uniform float uAlpha;
uniform vec3  uTint;
void main() {
    vec3 c = texture(uTex, vUV).rgb;
    c = c * 0.45 + uTint;
    FragColor = vec4(c, uAlpha);
}
)glsl";

// ── 내부 헬퍼 ──────────────────────────────────────────────────────────

static void MakeFBOTex(GLuint& fbo, GLuint& tex, int w, int h) {
    if (tex) glDeleteTextures(1, &tex);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (fbo) glDeleteFramebuffers(1, &fbo);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static GLuint LinkProg(const char* vsrc, const char* fsrc) {
    GLuint vs = CompileGlShader(GL_VERTEX_SHADER,   vsrc);
    GLuint fs = CompileGlShader(GL_FRAGMENT_SHADER, fsrc);
    GLuint p  = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    { GLint ok = 0;
      glGetProgramiv(p, GL_LINK_STATUS, &ok);
      if (!ok) {
          char log[512] = {};
          glGetProgramInfoLog(p, sizeof(log), nullptr, log);
          std::fprintf(stderr, "[BlurShader LINK FAIL] %s\n", log);
      } }
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

static void DrawNdcQuad() {
    static const float kQ[36] = {
        -1.f,-1.f, 0,0,0,0,   1.f,-1.f, 0,0,0,0,   1.f, 1.f, 0,0,0,0,
        -1.f,-1.f, 0,0,0,0,   1.f, 1.f, 0,0,0,0,  -1.f, 1.f, 0,0,0,0,
    };
    glBindVertexArray(g_MainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(kQ), kQ);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// ── 공개 API ───────────────────────────────────────────────────────────

void InitBlurSystem(int screenW, int screenH) {
    s_W  = screenW > 0 ? screenW : 1;
    s_H  = screenH > 0 ? screenH : 1;
    s_qW = std::max(1, s_W / 2);
    s_qH = std::max(1, s_H / 2);

    MakeFBOTex(s_fboA, s_texA, s_qW, s_qH);
    MakeFBOTex(s_fboB, s_texB, s_qW, s_qH);

    if (!s_blurProg) {
        s_blurProg    = LinkProg(kBlurVS, kBlurFS);
        s_blurTexLoc  = glGetUniformLocation(s_blurProg, "uTex");
        s_blurDirLoc  = glGetUniformLocation(s_blurProg, "uDir");

        s_blitProg    = LinkProg(kBlitVS, kBlitFS);
        s_blitTexLoc  = glGetUniformLocation(s_blitProg, "uTex");
        s_blitProjLoc = glGetUniformLocation(s_blitProg, "uProj");
        s_blitAlpLoc  = glGetUniformLocation(s_blitProg, "uAlpha");
        s_blitTintLoc = glGetUniformLocation(s_blitProg, "uTint");
    }
}

void ResizeBlurSystem(int screenW, int screenH) {
    InitBlurSystem(screenW, screenH);
}

void CaptureBackdrop() {
    BatchFlush();

    // ── GL 상태 저장 (FBO 패스가 메인 렌더 상태를 오염시키지 않도록) ──────
    GLboolean scissorOn = glIsEnabled(GL_SCISSOR_TEST);
    GLint scissorBox[4] = {};
    glGetIntegerv(GL_SCISSOR_BOX, scissorBox);

    GLint savedVP[4] = {};
    glGetIntegerv(GL_VIEWPORT, savedVP);

    GLint savedReadFbo = 0, savedDrawFbo = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &savedReadFbo);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &savedDrawFbo);
    const GLboolean blendOn = glIsEnabled(GL_BLEND);
    const GLboolean depthOn = glIsEnabled(GL_DEPTH_TEST);

    if (scissorOn) glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    if (depthOn) glDisable(GL_DEPTH_TEST);

    // ── 현재 프레임버퍼 → capTex 복사 ────────────────────────────────────
    glBindFramebuffer(GL_READ_FRAMEBUFFER, savedReadFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_fboA);
    glViewport(0, 0, s_qW, s_qH);
    glBlitFramebuffer(0, 0, s_W, s_H,
                      0, 0, s_qW, s_qH,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // FBO 패스 세팅 — scissor 반드시 꺼야 전체 FBO를 채울 수 있음
    glUseProgram(s_blurProg);
    glUniform1i(s_blurTexLoc, 0);
    glActiveTexture(GL_TEXTURE0);

    // Pass 1 — H 다운샘플: capTex(full) → fboA(quarter)
    // 결과: texA

    // Pass 2~3 — 1× H+V 블러 (half-res ping-pong A↔B)
    // σ ≈ 27px (full-res): 배경 실루엣은 보이되 디테일은 뭉개지는 frosted glass
    const float kH = 8.0f / (float)s_qW;
    const float kV = 8.0f / (float)s_qH;
    for (int p = 0; p < 1; ++p) {
        glBindFramebuffer(GL_FRAMEBUFFER, s_fboB);
        glBindTexture(GL_TEXTURE_2D, s_texA);
        glUniform2f(s_blurDirLoc, 0.0f, kV);
        DrawNdcQuad();

        glBindFramebuffer(GL_FRAMEBUFFER, s_fboA);
        glBindTexture(GL_TEXTURE_2D, s_texB);
        glUniform2f(s_blurDirLoc, kH, 0.0f);
        DrawNdcQuad();
    }
    // 최종 블러 결과: s_texA

    // ── GL 상태 복원 ───────────────────────────────────────────────────────
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, savedReadFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, savedDrawFbo);
    glViewport(savedVP[0], savedVP[1], savedVP[2], savedVP[3]);
    if (blendOn) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
    if (depthOn) glEnable(GL_DEPTH_TEST);

    if (scissorOn) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
    }

    BindMainShader();
}

void DrawBlurPanel(float x, float y, float w, float h,
                   float alpha,
                   float tintR, float tintG, float tintB) {
    BatchFlush();

    // screen → texture UV (Y-flip: OpenGL FB의 y=0은 화면 하단)
    const float u0 = x / (float)s_W;
    const float u1 = (x + w) / (float)s_W;
    const float v0 = 1.0f - (y + h) / (float)s_H;
    const float v1 = 1.0f - y / (float)s_H;

    const float blit[36] = {
        x,   y,   u0, v1, 0, 0,
        x+w, y,   u1, v1, 0, 0,
        x+w, y+h, u1, v0, 0, 0,
        x,   y,   u0, v1, 0, 0,
        x+w, y+h, u1, v0, 0, 0,
        x,   y+h, u0, v0, 0, 0,
    };

    glUseProgram(s_blitProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_texA);  // CaptureBackdrop의 최종 결과
    glUniform1i(s_blitTexLoc, 0);
    glUniformMatrix4fv(s_blitProjLoc, 1, GL_FALSE, g_BaseOrtho);
    glUniform1f(s_blitAlpLoc, alpha);
    glUniform3f(s_blitTintLoc, tintR, tintG, tintB);

    glBindVertexArray(g_MainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(blit), blit);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);
    BindMainShader();
}
