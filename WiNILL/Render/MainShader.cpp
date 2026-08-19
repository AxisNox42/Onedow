#include "MainShader.h"
#include "DrawPrim.h"
#include <cstdio>
#include <cstring>
#ifdef _WIN32
  #include <windows.h>
#endif

float  g_BaseOrtho[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
GLint  g_MainFxLoc  = -1;
GLint  g_MainResLoc = -1;

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
    "        vec2 uv = gl_FragCoord.xy / max(uRes, vec2(1.0));\n"
    "        float luma = dot(c.rgb, vec3(0.299, 0.587, 0.114));\n"
    "        c.rgb = mix(vec3(luma), c.rgb, 1.30);\n"
    "        vec3 topTint = vec3(0.92, 1.00, 1.06);\n"
    "        vec3 botTint = vec3(1.04, 0.94, 1.06);\n"
    "        c.rgb *= mix(botTint, topTint, uv.y);\n"
    "        float lum = max(c.r, max(c.g, c.b));\n"
    "        c.rgb += c.rgb * smoothstep(0.55, 1.0, lum) * 0.35;\n"
    "        c.rgb *= 0.94 + 0.06 * (0.5 + 0.5 * sin(gl_FragCoord.y * 3.14159));\n"
    "        vec2 d = uv - vec2(0.5);\n"
    "        c.rgb *= (1.0 - dot(d, d) * 0.50);\n"
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
    glBufferData(GL_ARRAY_BUFFER, 65536 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    g_MainVAO = VAO;
    g_Batch.reserve(131072);
}
