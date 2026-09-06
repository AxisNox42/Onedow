#include "NebulaGlowShader.h"
#include "DrawPrim.h"
#include "MainShader.h"
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#endif

static GLuint g_NGProg   = 0;
static GLuint g_NGVAO    = 0;
static GLuint g_NGVBO    = 0;
static GLint  g_NGProj   = -1;
static GLint  g_NGCenter = -1;
static GLint  g_NGRadius = -1;
static GLint  g_NGColor  = -1;
static GLint  g_NGAlpha  = -1;
static GLint  g_NGRes    = -1;
static GLint  g_NGTime   = -1;
static GLint  g_NGRainbow= -1;
static float  g_NGResW   = 1920.0f;
static float  g_NGResH   = 1080.0f;

static const char* kNGVert =
    "#version 330 core\n"
    "layout(location=0) in vec2 aPos;\n"
    "uniform mat4 projection;\n"
    "void main() { gl_Position = projection * vec4(aPos, 0.0, 1.0); }\n";

static const char* kNGFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec2  u_center;\n"
    "uniform float u_radius;\n"
    "uniform vec3  u_color;\n"
    "uniform float u_alpha;\n"
    "uniform vec2  u_res;\n"
    "uniform float u_time;\n"
    "uniform float u_rainbow;\n"
    "void main() {\n"
    "    vec2 p = vec2(gl_FragCoord.x, u_res.y - gl_FragCoord.y);\n"
    "    float t = length(p - u_center) / max(u_radius, 1.0);\n"
    "    if (t >= 1.0) discard;\n"
    "    float a = pow(max(0.0, 1.0 - t), 1.8) * u_alpha;\n"
    "    if (a < 0.006) discard;\n"
    "    vec3 col = u_color;\n"
    "    if (u_rainbow > 0.5) {\n"
    "        col.r = 0.5 + 0.5 * sin(u_time * 2.0 + 0.000);\n"
    "        col.g = 0.5 + 0.5 * sin(u_time * 2.0 + 2.094);\n"
    "        col.b = 0.5 + 0.5 * sin(u_time * 2.0 + 4.189);\n"
    "    }\n"
    "    FragColor = vec4(col, a);\n"
    "}\n";

void InitNebulaGlowShader(int screenW, int screenH) {
    g_NGResW = (float)screenW;
    g_NGResH = (float)screenH;

    GLuint vs = CompileGlShader(GL_VERTEX_SHADER,   kNGVert);
    GLuint fs = CompileGlShader(GL_FRAGMENT_SHADER, kNGFrag);
    g_NGProg  = glCreateProgram();
    glAttachShader(g_NGProg, vs);
    glAttachShader(g_NGProg, fs);
    glLinkProgram(g_NGProg);
    { GLint ok = 0;
      glGetProgramiv(g_NGProg, GL_LINK_STATUS, &ok);
      if (!ok) {
          char log[1024] = {0};
          glGetProgramInfoLog(g_NGProg, sizeof(log), NULL, log);
          std::fprintf(stderr, "[NebulaGlowShader LINK FAIL] %s\n", log);
#ifdef _WIN32
          MessageBoxA(NULL, log, "NebulaGlowShader link failed", MB_OK | MB_ICONERROR);
#endif
      } }
    glDeleteShader(vs);
    glDeleteShader(fs);

    g_NGProj    = glGetUniformLocation(g_NGProg, "projection");
    g_NGCenter  = glGetUniformLocation(g_NGProg, "u_center");
    g_NGRadius  = glGetUniformLocation(g_NGProg, "u_radius");
    g_NGColor   = glGetUniformLocation(g_NGProg, "u_color");
    g_NGAlpha   = glGetUniformLocation(g_NGProg, "u_alpha");
    g_NGRes     = glGetUniformLocation(g_NGProg, "u_res");
    g_NGTime    = glGetUniformLocation(g_NGProg, "u_time");
    g_NGRainbow = glGetUniformLocation(g_NGProg, "u_rainbow");

    glGenVertexArrays(1, &g_NGVAO);
    glGenBuffers(1,      &g_NGVBO);
    glBindVertexArray(g_NGVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_NGVBO);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    BindMainShader();
}

void DrawNebulaGlow(float cx, float cy, float radius,
                    float r, float g, float b, float alpha,
                    float time, bool rainbow) {
    if (g_NGProg == 0 || alpha <= 0.001f || radius <= 0.5f) return;

    BatchFlush();

    float pad = radius * 1.02f;
    float x0 = cx - pad, y0 = cy - pad;
    float x1 = cx + pad, y1 = cy + pad;
    float v[12] = { x0,y0, x1,y0, x1,y1, x0,y0, x1,y1, x0,y1 };

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // 가산 합성 — 성운 글로우
    glUseProgram(g_NGProg);
    glUniformMatrix4fv(g_NGProj,   1, GL_FALSE, g_MainOrtho);
    glUniform2f (g_NGCenter, cx, cy);
    glUniform1f (g_NGRadius, radius);
    glUniform3f (g_NGColor,  r, g, b);
    glUniform1f (g_NGAlpha,  alpha * g_BatchAlpha);
    glUniform2f (g_NGRes,    g_NGResW, g_NGResH);
    glUniform1f (g_NGTime,   time);
    glUniform1f (g_NGRainbow, rainbow ? 1.0f : 0.0f);

    glBindVertexArray(g_NGVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_NGVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   // 표준 블렌드 복원
    BindMainShader();
}
