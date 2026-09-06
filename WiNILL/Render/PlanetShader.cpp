#include "PlanetShader.h"
#include "DrawPrim.h"
#include "MainShader.h"
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#endif

static GLuint g_PProg   = 0;
static GLuint g_PVAO    = 0;
static GLuint g_PVBO    = 0;
static GLint  g_PProj   = -1;
static GLint  g_PCenter = -1;
static GLint  g_PRadius = -1;
static GLint  g_PColor  = -1;
static GLint  g_PGlow   = -1;
static GLint  g_PAlpha  = -1;
static GLint  g_PRes    = -1;
static GLint  g_PRing   = -1;
static float  g_PResW   = 1920.0f;
static float  g_PResH   = 1080.0f;

static const char* kPVert =
    "#version 330 core\n"
    "layout(location=0) in vec2 aPos;\n"
    "uniform mat4 projection;\n"
    "void main() { gl_Position = projection * vec4(aPos, 0.0, 1.0); }\n";

static const char* kPFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec2  u_center;\n"
    "uniform float u_radius;\n"
    "uniform vec3  u_color;\n"
    "uniform float u_glow;\n"
    "uniform float u_alpha;\n"
    "uniform vec2  u_res;\n"
    "uniform float u_ring;\n"
    "void main() {\n"
    "    // gl_FragCoord is Y-up; flip to match Y-down pixel coords\n"
    "    vec2 p = vec2(gl_FragCoord.x, u_res.y - gl_FragCoord.y);\n"
    "    float d = length(p - u_center);\n"
    "    float edge = max(u_radius * 0.016, 1.2);\n"
    "    // core disc\n"
    "    float core = 1.0 - smoothstep(u_radius - edge, u_radius + edge, d);\n"
    "    // rim highlight (bright ring near planet edge)\n"
    "    float rim = smoothstep(u_radius * 0.55, u_radius * 0.88, d)\n"
    "              * (1.0 - smoothstep(u_radius * 0.88, u_radius, d));\n"
    "    rim *= u_ring;\n"
    "    // glow halo (exponential falloff beyond edge)\n"
    "    float glowD = max(d - u_radius, 0.0);\n"
    "    float halo  = exp(-glowD / max(u_radius * 0.38, 1.0)) * u_glow;\n"
    "    float a = clamp(core + halo * (1.0 - core * 0.7), 0.0, 1.0) * u_alpha;\n"
    "    if (a < 0.008) discard;\n"
    "    vec3  col = u_color * (core * (1.0 + rim * 0.9) + halo * 0.55);\n"
    "    FragColor = vec4(clamp(col, 0.0, 2.0), a);\n"
    "}\n";

void InitPlanetShader(int screenW, int screenH) {
    g_PResW = (float)screenW;
    g_PResH = (float)screenH;

    GLuint vs = CompileGlShader(GL_VERTEX_SHADER,   kPVert);
    GLuint fs = CompileGlShader(GL_FRAGMENT_SHADER, kPFrag);
    g_PProg   = glCreateProgram();
    glAttachShader(g_PProg, vs);
    glAttachShader(g_PProg, fs);
    glLinkProgram(g_PProg);
    { GLint ok = 0;
      glGetProgramiv(g_PProg, GL_LINK_STATUS, &ok);
      if (!ok) {
          char log[1024] = {0};
          glGetProgramInfoLog(g_PProg, sizeof(log), NULL, log);
          std::fprintf(stderr, "[PlanetShader LINK FAIL] %s\n", log);
#ifdef _WIN32
          MessageBoxA(NULL, log, "PlanetShader link failed", MB_OK | MB_ICONERROR);
#endif
      } }
    glDeleteShader(vs);
    glDeleteShader(fs);

    g_PProj   = glGetUniformLocation(g_PProg, "projection");
    g_PCenter = glGetUniformLocation(g_PProg, "u_center");
    g_PRadius = glGetUniformLocation(g_PProg, "u_radius");
    g_PColor  = glGetUniformLocation(g_PProg, "u_color");
    g_PGlow   = glGetUniformLocation(g_PProg, "u_glow");
    g_PAlpha  = glGetUniformLocation(g_PProg, "u_alpha");
    g_PRes    = glGetUniformLocation(g_PProg, "u_res");
    g_PRing   = glGetUniformLocation(g_PProg, "u_ring");

    // 별도 VAO/VBO (pos xy only, 6 verts = 1 quad)
    glGenVertexArrays(1, &g_PVAO);
    glGenBuffers(1,      &g_PVBO);
    glBindVertexArray(g_PVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_PVBO);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    BindMainShader();   // 초기화 후 메인 셰이더 복원
}

void DrawPlanetSDF(float cx, float cy, float radius,
                   float r, float g, float b,
                   float glow, float alpha, float ring) {
    if (g_PProg == 0 || alpha <= 0.001f || radius <= 0.5f) return;

    BatchFlush();   // 메인 배치 먼저 비움

    float pad = radius * (1.0f + glow * 1.6f + 0.25f);
    float x0 = cx - pad, y0 = cy - pad;
    float x1 = cx + pad, y1 = cy + pad;
    float v[12] = { x0,y0, x1,y0, x1,y1, x0,y0, x1,y1, x0,y1 };

    glUseProgram(g_PProg);
    glUniformMatrix4fv(g_PProj,   1, GL_FALSE, g_MainOrtho);
    glUniform2f (g_PCenter, cx, cy);
    glUniform1f (g_PRadius, radius);
    glUniform3f (g_PColor,  r, g, b);
    glUniform1f (g_PGlow,   glow);
    glUniform1f (g_PAlpha,  alpha * g_BatchAlpha);
    glUniform2f (g_PRes,    g_PResW, g_PResH);
    glUniform1f (g_PRing,   ring);

    glBindVertexArray(g_PVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_PVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    BindMainShader();   // 메인 셰이더/VAO 복원
}
