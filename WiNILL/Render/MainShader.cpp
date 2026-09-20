#include "MainShader.h"
#include "DrawPrim.h"
#include "stb_image.h"
#include <cstdio>
#include <cstring>
#ifdef _WIN32
  #include <windows.h>
#endif

float  g_BaseOrtho[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
GLint  g_MainFxLoc  = -1;
GLint  g_MainResLoc = -1;

// ── 방사형 그라데이션 (PNG 텍스처 기반, 수학식 밴딩 없음) ──
static GLuint g_RadGradProg    = 0;
static GLuint g_RadGradTex     = 0;
static GLuint g_LinGradTex     = 0;
static GLint  g_RadGradProjLoc = -1;
static GLint  g_RadGradTexLoc  = -1;
static GLint  g_RadGradColLoc  = -1;
static GLint  g_RadGradAlpLoc  = -1;

static const char* kRadGradVS =
    "#version 330 core\n"
    "layout(location=0) in vec2 aPos;\n"
    "layout(location=1) in vec4 aColor;\n"
    "uniform mat4 projection;\n"
    "out vec2 vUV;\n"
    "void main() { vUV = aColor.xy; gl_Position = projection * vec4(aPos,0.0,1.0); }\n";

static const char* kRadGradFS =
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTex;\n"
    "uniform vec3  uColor;\n"
    "uniform float uAlpha;\n"
    "void main() {\n"
    "    float luma = texture(uTex, vUV).r;\n"
    "    float a = (1.0 - luma) * uAlpha;\n"
    "    FragColor = vec4(uColor, clamp(a, 0.0, 1.0));\n"
    "}\n";

static void LoadRadGradTexture() {
    int w = 0, h = 0, n = 0;
    unsigned char* d = stbi_load("Resource/bg_radial.png", &w, &h, &n, 1);
    if (!d) { std::fprintf(stderr,"[RadGrad] Resource/bg_radial.png not found\n"); return; }
    glGenTextures(1, &g_RadGradTex);
    glBindTexture(GL_TEXTURE_2D, g_RadGradTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, d);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(d);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void LoadLinGradTexture() {
    int w = 0, h = 0, n = 0;
    unsigned char* d = stbi_load("Resource/bg_linear.png", &w, &h, &n, 1);
    if (!d) { std::fprintf(stderr,"[LinGrad] Resource/bg_linear.png not found\n"); return; }
    glGenTextures(1, &g_LinGradTex);
    glBindTexture(GL_TEXTURE_2D, g_LinGradTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, d);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(d);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void InitRadialGradShader() {
    GLuint vs = CompileGlShader(GL_VERTEX_SHADER,   kRadGradVS);
    GLuint fs = CompileGlShader(GL_FRAGMENT_SHADER, kRadGradFS);
    g_RadGradProg = glCreateProgram();
    glAttachShader(g_RadGradProg, vs);
    glAttachShader(g_RadGradProg, fs);
    glLinkProgram(g_RadGradProg);
    { GLint lok = 0;
      glGetProgramiv(g_RadGradProg, GL_LINK_STATUS, &lok);
      if (!lok) {
          char log[512] = {};
          glGetProgramInfoLog(g_RadGradProg, sizeof(log), NULL, log);
          std::fprintf(stderr, "[RadGrad LINK FAIL] %s\n", log);
      } }
    glDeleteShader(vs);
    glDeleteShader(fs);
    g_RadGradProjLoc = glGetUniformLocation(g_RadGradProg, "projection");
    g_RadGradTexLoc  = glGetUniformLocation(g_RadGradProg, "uTex");
    g_RadGradColLoc  = glGetUniformLocation(g_RadGradProg, "uColor");
    g_RadGradAlpLoc  = glGetUniformLocation(g_RadGradProg, "uAlpha");
    LoadRadGradTexture();
    LoadLinGradTexture();
}

static const char* kMainVertSrc =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec4 aColor;\n"
    "uniform mat4 projection;\n"
    "out vec4 vColor;\n"
    "void main() { vColor = aColor; gl_Position = projection * vec4(aPos, 0.0, 1.0); }\n";

static const char* kMainFragSrc =
    "#version 330 core\n"
    "in vec4 vColor;\n"
    "out vec4 FragColor;\n"
    "uniform int  uFx;\n"
    "uniform vec2 uRes;\n"
    "void main() {\n"
    "    vec4 c = vColor;\n"
    "    if (uFx == 1) {\n"
    "        float luma = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"
    "        c.rgb = mix(vec3(luma), c.rgb, 1.16);\n"
    "        float lum = max(c.r, max(c.g, c.b));\n"
    "        c.rgb += c.rgb * smoothstep(0.55, 1.0, lum) * 0.18;\n"
    "        c.rgb = clamp(c.rgb, 0.0, 1.0);\n"
    "    } else if (uFx == 2) {\n"
    "        float lum = max(c.r, max(c.g, c.b));\n"
    "        c.rgb += c.rgb * smoothstep(0.38, 1.0, lum) * 0.70;\n"
    "        c.rgb = clamp(c.rgb, 0.0, 1.0);\n"
    "    }\n"
    "    FragColor = c;\n"
    "}\n";

GLuint CompileGlShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        std::fprintf(stderr, "[SHADER COMPILE FAIL] %s\n", log);
#ifdef _WIN32
        MessageBoxA(NULL, log, "Shader compile failed", MB_OK | MB_ICONERROR);
#endif
    }
    return s;
}

void InitMainShaderPipeline(int screenW, int screenH) {
    GLuint vs = CompileGlShader(GL_VERTEX_SHADER,   kMainVertSrc);
    GLuint fs = CompileGlShader(GL_FRAGMENT_SHADER, kMainFragSrc);
    GLuint shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    { GLint lok = 0;
      glGetProgramiv(shader, GL_LINK_STATUS, &lok);
      if (!lok) {
          char log[1024] = {0};
          glGetProgramInfoLog(shader, sizeof(log), NULL, log);
          std::fprintf(stderr, "[SHADER LINK FAIL] %s\n", log);
#ifdef _WIN32
          MessageBoxA(NULL, log, "Shader link failed", MB_OK | MB_ICONERROR);
#endif
      } }
    glDeleteShader(vs);
    glDeleteShader(fs);

    g_MainShader  = shader;
    g_MainProjLoc = glGetUniformLocation(shader, "projection");
    g_colorLoc    = glGetUniformLocation(shader, "color");
    g_MainFxLoc   = glGetUniformLocation(shader, "uFx");
    g_MainResLoc  = glGetUniformLocation(shader, "uRes");

    glUseProgram(shader);
    glUniform2f(g_MainResLoc, (float)screenW, (float)screenH);

    InitRadialGradShader();
}

void InitMainBatchGeometry(int screenW, int screenH) {
    g_BaseOrtho[0]  =  2.0f / (float)screenW;
    g_BaseOrtho[1]  =  0.0f;
    g_BaseOrtho[2]  =  0.0f;
    g_BaseOrtho[3]  =  0.0f;
    g_BaseOrtho[4]  =  0.0f;
    g_BaseOrtho[5]  = -2.0f / (float)screenH;
    g_BaseOrtho[6]  =  0.0f;
    g_BaseOrtho[7]  =  0.0f;
    g_BaseOrtho[8]  =  0.0f;
    g_BaseOrtho[9]  =  0.0f;
    g_BaseOrtho[10] = -1.0f;
    g_BaseOrtho[11] =  0.0f;
    g_BaseOrtho[12] = -1.0f;
    g_BaseOrtho[13] =  1.0f;
    g_BaseOrtho[14] =  0.0f;
    g_BaseOrtho[15] =  1.0f;
    memcpy(g_MainOrtho, g_BaseOrtho, sizeof(g_BaseOrtho));

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &g_VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, 65536 * sizeof(float), NULL, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    g_MainVAO = VAO;
    g_Batch.reserve(131072);
}

void DrawRadialGradient(float cx, float cy, float radius,
                        float r, float g, float b, float alpha) {
    DrawRadialGradientRect(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f,
                           r, g, b, alpha);
}

void DrawRadialGradientRect(float x, float y, float w, float h,
                            float r, float g, float b, float alpha) {
    if (alpha <= 0.0f || w <= 0.0f || h <= 0.0f || g_RadGradProg == 0 || g_RadGradTex == 0) return;
    BatchFlush();

    float x0 = x, y0 = y;
    float x1 = x + w, y1 = y + h;

    glUseProgram(g_RadGradProg);
    glBindVertexArray(g_MainVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_RadGradTex);
    glUniform1i(g_RadGradTexLoc, 0);
    glUniformMatrix4fv(g_RadGradProjLoc, 1, GL_FALSE, g_BaseOrtho);
    glUniform3f(g_RadGradColLoc, r, g, b);
    glUniform1f(g_RadGradAlpLoc, alpha);

    float verts[36] = {
        x0,y0, 0,0, 0,0,  x1,y0, 1,0, 0,0,  x1,y1, 1,1, 0,0,
        x0,y0, 0,0, 0,0,  x1,y1, 1,1, 0,0,  x0,y1, 0,1, 0,0
    };
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);
    BindMainShader();
}

void DrawLinearGradient(float x, float y, float w, float h,
                        float r, float g, float b, float alpha,
                        bool mirrorX) {
    if (alpha <= 0.0f || w <= 0.0f || h <= 0.0f || g_RadGradProg == 0 || g_LinGradTex == 0) return;
    BatchFlush();

    float x0 = x, y0 = y, x1 = x + w, y1 = y + h;

    glUseProgram(g_RadGradProg);
    glBindVertexArray(g_MainVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_LinGradTex);
    glUniform1i(g_RadGradTexLoc, 0);
    glUniformMatrix4fv(g_RadGradProjLoc, 1, GL_FALSE, g_BaseOrtho);
    glUniform3f(g_RadGradColLoc, r, g, b);
    glUniform1f(g_RadGradAlpLoc, alpha);

    const float u0 = mirrorX ? 1.0f : 0.0f;
    const float u1 = mirrorX ? 0.0f : 1.0f;
    float verts[36] = {
        x0,y0, u0,0, 0,0,  x1,y0, u1,0, 0,0,  x1,y1, u1,1, 0,0,
        x0,y0, u0,0, 0,0,  x1,y1, u1,1, 0,0,  x0,y1, u0,1, 0,0
    };
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);
    BindMainShader();
}

void DrawLinearGradientRibbon(float x, float y, float w, float h,
                              float cut, float r, float g, float b,
                              float alpha, bool mirrorX, bool mirrorY,
                              bool flatLeft) {
    if (alpha <= 0.0f || w <= 0.0f || h <= 0.0f
        || g_RadGradProg == 0 || g_LinGradTex == 0) return;
    BatchFlush();

    float cutPx = cut;
    if (cutPx < 0.0f) cutPx = 0.0f;
    if (cutPx > w * 0.28f) cutPx = w * 0.28f;

    // Mirror the actual silhouette as well as the gradient.  The previous
    // implementation only flipped UVs, so the button still had its slant on
    // the right edge even when callers requested a mirrored ribbon.
    // Keep both side edges parallel.  This is the actual 120-60-120-60
    // parallelogram silhouette: top/bottom are equal, and the two slanted
    // sides share the same offset.  The whole shape remains inside x..x+w.
    const float topLeft = mirrorX
        ? (flatLeft ? x : x + cutPx) : x;
    const float topRight = mirrorX ? x + w : x + w - cutPx;
    const float bottomLeft = mirrorX
        ? x : (flatLeft ? x : x + cutPx);
    const float bottomRight = mirrorX ? x + w - cutPx : x + w;
    const float y0 = y;
    const float y1 = y + h;
    const float u0 = mirrorX ? 1.0f : 0.0f;
    const float u1 = mirrorX ? 0.0f : 1.0f;
    const float v0 = mirrorY ? 1.0f : 0.0f;
    const float v1 = mirrorY ? 0.0f : 1.0f;

    glUseProgram(g_RadGradProg);
    glBindVertexArray(g_MainVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_LinGradTex);
    glUniform1i(g_RadGradTexLoc, 0);
    glUniformMatrix4fv(g_RadGradProjLoc, 1, GL_FALSE, g_BaseOrtho);
    glUniform3f(g_RadGradColLoc, r, g, b);
    glUniform1f(g_RadGradAlpLoc, alpha);

    // The paired slanted edges turn the rectangular texture into a true
    // directional parallelogram while preserving the full gradient range.
    float verts[36] = {
        topLeft,y0,  u0,v0, 0,0,  topRight,y0,   u1,v0, 0,0,  bottomRight,y1, u1,v1, 0,0,
        topLeft,y0,  u0,v0, 0,0,  bottomRight,y1, u1,v1, 0,0,  bottomLeft,y1,  u0,v1, 0,0
    };
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);
    BindMainShader();
}
