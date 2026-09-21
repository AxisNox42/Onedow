#include "BlurShader.h"
#include "MainShader.h"   // CompileGlShader, g_BaseOrtho
#include "DrawPrim.h"     // BatchFlush, BindMainShader, g_MainVAO, g_VBO
#include <cstdio>
#include <cstring>
#include <algorithm>

static int    s_W = 1, s_H = 1;
static int    s_qW = 1, s_qH = 1;      // half-res dimensions
static int    s_captureW = 1;
static GLuint s_captureFbo = 0;
static GLuint s_captureTex = 0;        // source framebuffer snapshot
static GLuint s_fboA = 0, s_texA = 0;  // half-res ping-pong A (holds final result)
static GLuint s_fboB = 0, s_texB = 0;  // half-res ping-pong B
static GLuint s_blurProg = 0;
static GLuint s_blitProg = 0;
static bool   s_blurReady = false;
static bool   s_captureValid = false;

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

static bool MakeFBOTex(GLuint& fbo, GLuint& tex, int w, int h) {
    if (w <= 0 || h <= 0) return false;

    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (tex) glDeleteTextures(1, &tex);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    // Keep the render target format conservative and explicitly sized for
    // Intel/AMD drivers as well as NVIDIA.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, tex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "[BlurShader FBO FAIL] 0x%04X (%dx%d)\n",
                     (unsigned)status, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &tex);
        fbo = 0;
        tex = 0;
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

static GLuint LinkProg(const char* vsrc, const char* fsrc) {
    GLuint vs = CompileGlShader(GL_VERTEX_SHADER,   vsrc);
    GLuint fs = CompileGlShader(GL_FRAGMENT_SHADER, fsrc);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }

    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512] = {};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[BlurShader LINK FAIL] %s\n", log);
        glDeleteProgram(p);
        p = 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
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
    if (s_blurReady && s_W == screenW && s_H == screenH) return;
    GLint savedReadFbo = 0, savedDrawFbo = 0;
    GLint savedActiveTexture = GL_TEXTURE0;
    GLint savedTexture0 = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &savedReadFbo);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &savedDrawFbo);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &savedActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTexture0);

    s_captureW = std::max(1, screenW);
    s_W  = screenW > 0 ? screenW : 1;
    s_H  = screenH > 0 ? screenH : 1;
    s_qW = std::max(1, s_W / 2);
    s_qH = std::max(1, s_H / 2);
    s_blurReady = false;
    s_captureValid = false;

    const bool captureOK = MakeFBOTex(s_captureFbo, s_captureTex, s_W, s_H);
    const bool fboAOK = MakeFBOTex(s_fboA, s_texA, s_qW, s_qH);
    const bool fboBOK = MakeFBOTex(s_fboB, s_texB, s_qW, s_qH);

    if (!s_blurProg || !s_blitProg) {
        if (s_blurProg) glDeleteProgram(s_blurProg);
        if (s_blitProg) glDeleteProgram(s_blitProg);
        s_blurProg    = LinkProg(kBlurVS, kBlurFS);
        s_blitProg    = LinkProg(kBlitVS, kBlitFS);
    }

    if (s_blurProg) {
        s_blurTexLoc = glGetUniformLocation(s_blurProg, "uTex");
        s_blurDirLoc = glGetUniformLocation(s_blurProg, "uDir");
    }
    if (s_blitProg) {
        s_blitTexLoc  = glGetUniformLocation(s_blitProg, "uTex");
        s_blitProjLoc = glGetUniformLocation(s_blitProg, "uProj");
        s_blitAlpLoc  = glGetUniformLocation(s_blitProg, "uAlpha");
        s_blitTintLoc = glGetUniformLocation(s_blitProg, "uTint");
    }

    s_blurReady = captureOK && fboAOK && fboBOK
               && s_blurProg != 0 && s_blitProg != 0
               && s_blurTexLoc >= 0 && s_blurDirLoc >= 0
               && s_blitTexLoc >= 0 && s_blitProjLoc >= 0
               && s_blitAlpLoc >= 0 && s_blitTintLoc >= 0;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, savedReadFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, savedDrawFbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)savedTexture0);
    glActiveTexture((GLenum)savedActiveTexture);
}

void ResizeBlurSystem(int screenW, int screenH) {
    InitBlurSystem(screenW, screenH);
}

void CaptureBackdrop() {
    s_captureValid = false;
    if (!s_blurReady) return;
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
    GLint savedReadBuffer = GL_BACK;
    glGetIntegerv(GL_READ_BUFFER, &savedReadBuffer);

    GLint savedActiveTexture = GL_TEXTURE0;
    GLint savedTexture0 = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &savedActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTexture0);

    const GLboolean cullOn = glIsEnabled(GL_CULL_FACE);
    const GLboolean stencilOn = glIsEnabled(GL_STENCIL_TEST);
    const GLboolean rasterDiscardOn = glIsEnabled(GL_RASTERIZER_DISCARD);
    GLboolean colorMask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
    GLint savedSampler = 0;
    glGetIntegeri_v(GL_SAMPLER_BINDING, 0, &savedSampler);
    glBindSampler(0, 0);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_RASTERIZER_DISCARD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    const GLboolean blendOn = glIsEnabled(GL_BLEND);
    const GLboolean depthOn = glIsEnabled(GL_DEPTH_TEST);
    GLint blendSrcRgb = GL_SRC_ALPHA, blendDstRgb = GL_ONE_MINUS_SRC_ALPHA;
    GLint blendSrcAlpha = GL_ONE, blendDstAlpha = GL_ONE_MINUS_SRC_ALPHA;
    GLint blendEqRgb = GL_FUNC_ADD, blendEqAlpha = GL_FUNC_ADD;
    GLint depthWrite = GL_TRUE;
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &blendEqRgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &blendEqAlpha);
    glGetIntegerv(GL_DEPTH_WRITEMASK, &depthWrite);

    if (scissorOn) glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    if (depthOn) glDisable(GL_DEPTH_TEST);

    // ── 현재 프레임버퍼 → texture 복사 ───────────────────────────────────
    // Snapshot the actual draw target, not a potentially unrelated read FBO.
    // Resolve MSAA at full size first; downsampling happens in the shader.
    GLint sourceBuffer = GL_BACK;
    glGetIntegerv(GL_DRAW_BUFFER, &sourceBuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, savedDrawFbo);
    GLint sourceReadBuffer = GL_BACK;
    glGetIntegerv(GL_READ_BUFFER, &sourceReadBuffer);
    glReadBuffer((GLenum)sourceBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_captureFbo);
    while (glGetError() != GL_NO_ERROR) {}
    glBlitFramebuffer(savedVP[0], savedVP[1],
                      savedVP[0] + s_W, savedVP[1] + s_H,
                      0, 0, s_W, s_H, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    GLenum captureError = glGetError();
    if (captureError != GL_NO_ERROR) {
        // Single-sample fallback for default-framebuffer blit restrictions.
        glBindTexture(GL_TEXTURE_2D, s_captureTex);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                           savedVP[0], savedVP[1], s_W, s_H);
        captureError = glGetError();
    }
    glReadBuffer((GLenum)sourceReadBuffer);
    if (captureError != GL_NO_ERROR) {
        static bool reported = false;
        if (!reported) std::fprintf(stderr, "[BlurShader capture FAIL] 0x%04X\n",
                                    (unsigned)captureError);
        reported = true;
    }

    if (captureError == GL_NO_ERROR) {
        glUseProgram(s_blurProg);
        glUniform1i(s_blurTexLoc, 0);
        glActiveTexture(GL_TEXTURE0);

        // Pass 1 — capture texture → half-resolution target. Linear texture
        // filtering performs the downsample without compute shaders.
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_fboA);
        glViewport(0, 0, s_qW, s_qH);
        glBindTexture(GL_TEXTURE_2D, s_captureTex);
        glUniform2f(s_blurDirLoc, 2.0f / (float)s_captureW, 0.0f);
        DrawNdcQuad();

        // Pass 2~3 — separable vertical + horizontal Gaussian blur.
        const float kH = 8.0f / (float)s_qW;
        const float kV = 8.0f / (float)s_qH;
        glBindFramebuffer(GL_FRAMEBUFFER, s_fboB);
        glBindTexture(GL_TEXTURE_2D, s_texA);
        glUniform2f(s_blurDirLoc, 0.0f, kV);
        DrawNdcQuad();

        glBindFramebuffer(GL_FRAMEBUFFER, s_fboA);
        glBindTexture(GL_TEXTURE_2D, s_texB);
        glUniform2f(s_blurDirLoc, kH, 0.0f);
        DrawNdcQuad();
        s_captureValid = glGetError() == GL_NO_ERROR;
    }

    // ── GL 상태 복원 ───────────────────────────────────────────────────────
    glBindTexture(GL_TEXTURE_2D, (GLuint)savedTexture0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, savedReadFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, savedDrawFbo);
    glReadBuffer(savedReadBuffer);
    glViewport(savedVP[0], savedVP[1], savedVP[2], savedVP[3]);
    if (blendOn) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    glBlendFuncSeparate((GLenum)blendSrcRgb, (GLenum)blendDstRgb,
                        (GLenum)blendSrcAlpha, (GLenum)blendDstAlpha);
    glBlendEquationSeparate((GLenum)blendEqRgb, (GLenum)blendEqAlpha);
    if (depthOn) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    glDepthMask(depthWrite ? GL_TRUE : GL_FALSE);

    if (scissorOn) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    if (cullOn) glEnable(GL_CULL_FACE);
    if (stencilOn) glEnable(GL_STENCIL_TEST);
    if (rasterDiscardOn) glEnable(GL_RASTERIZER_DISCARD);
    glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    glBindSampler(0, (GLuint)savedSampler);
    glActiveTexture((GLenum)savedActiveTexture);

    // Keep the rest of the UI on the normal batch shader. The blur pass is
    // deliberately self-contained and does not depend on the active GPU.
    BindMainShader();
}

void DrawBlurPanel(float x, float y, float w, float h,
                   float alpha,
                   float tintR, float tintG, float tintB) {
    BatchFlush();

    if (!s_blurReady || !s_captureValid || !s_blitProg) {
        // A broken/unsupported FBO must never turn the panel black or poison
        // the following UI passes. Keep a readable translucent veil as a
        // graceful fallback on software renderers and old drivers.
        BindMainShader();
        drawRect(x, y, w, h,
                 std::max(0.0f, tintR), std::max(0.0f, tintG),
                 std::max(0.0f, tintB), alpha);
        return;
    }

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

    GLint savedActiveTexture = GL_TEXTURE0, savedTexture0 = 0, savedSampler = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &savedActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTexture0);
    glGetIntegeri_v(GL_SAMPLER_BINDING, 0, &savedSampler);
    glBindSampler(0, 0);
    glUseProgram(s_blitProg);
    glBindTexture(GL_TEXTURE_2D, s_texA);  // CaptureBackdrop의 최종 결과
    glUniform1i(s_blitTexLoc, 0);
    glUniformMatrix4fv(s_blitProjLoc, 1, GL_FALSE, g_BaseOrtho);
    glUniform1f(s_blitAlpLoc, alpha);
    glUniform3f(s_blitTintLoc, tintR, tintG, tintB);

    glBindVertexArray(g_MainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(blit), blit);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, (GLuint)savedTexture0);
    glBindSampler(0, (GLuint)savedSampler);
    glActiveTexture((GLenum)savedActiveTexture);
    BindMainShader();
}
