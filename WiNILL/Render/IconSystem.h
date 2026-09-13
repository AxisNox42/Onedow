#pragma once
// ─────────────────────────────────────────────────────────────
// 아이콘(픽토그램) 시스템 — game-icons.net 흰색 PNG 를 텍스처로 로드,
//   AugType → 텍스처 매핑, 등급색 틴트로 그리는 drawIcon 헬퍼.
//   파일명 = AugType 이넘명 (예: Icons/DMG_UP.png). 흰색+투명배경 권장.
// ─────────────────────────────────────────────────────────────
#include <glad/glad.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include "stb_image.h"
#include "Augment.h"
#include "DrawPrim.h"   // g_MainOrtho
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>   // 리소스(RCDATA)에서 아이콘 로드
#endif

// ── AugType → 파일명(이넘명) ──────────────────────────────
inline const char* IconNameForAug(AugType t) {
    switch (t) {
    // 일반
    case AugType::DMG_UP:        return "DMG_UP";
    case AugType::RATE_UP:       return "RATE_UP";
    case AugType::SPD_UP:        return "SPD_UP";
    case AugType::MOVE_UP:       return "MOVE_UP";
    case AugType::VISION_UP:     return "VISION_UP";
    case AugType::REGEN_UP:      return "REGEN_UP";
    // 희귀
    case AugType::GLASS_CANNON:  return "GLASS_CANNON";
    case AugType::LIGHT_AMMO:    return "LIGHT_AMMO";
    case AugType::LIGHT_STEP:    return "LIGHT_STEP";
    case AugType::GUN_RUNNER:    return "GUN_RUNNER";
    case AugType::BULLET_RAIN:   return "BULLET_RAIN";
    case AugType::CRIT:          return "CRIT";
    case AugType::LIFESTEAL:     return "LIFESTEAL";
    case AugType::BERSERK:       return "BERSERK";
    // 에픽
    case AugType::VAMPIRE:       return "VAMPIRE";
    case AugType::BROKEN_SIGHT:  return "BROKEN_SIGHT";
    case AugType::BAYONET:       return "BAYONET";
    case AugType::MINIATURIZE:   return "MINIATURIZE";
    case AugType::GIGANTIFY:     return "GIGANTIFY";
    case AugType::PIERCE:        return "PIERCE";
    case AugType::TWIN:          return "TWIN";
    case AugType::CHAKRAM:       return "CHAKRAM";
    case AugType::BULLET_RAIN_2: return "BULLET_RAIN_2";
    case AugType::CHAKRAM_2:     return "CHAKRAM_2";
    case AugType::DRONE:         return "DRONE";
    case AugType::MINIGUN:       return "MINIGUN";
    case AugType::HACK_RANGED:   return "HACK_RANGED";
    case AugType::PROB_CHAIN:    return "PROB_CHAIN";
    case AugType::DEATH_BLAST:   return "DEATH_BLAST";
    case AugType::SKILL_CLOSE:   return "SKILL_CLOSE";
    case AugType::SKILL_OVERCLOCK: return "SKILL_OVERCLOCK";
    // 전설
    case AugType::RANDOM_AUG:    return "RANDOM_AUG";
    case AugType::SOUL_HARVEST:  return "SOUL_HARVEST";
    case AugType::MK2:           return "MK2";
    case AugType::HACK_BOMBER:   return "HACK_BOMBER";
    case AugType::CHAIN:         return "CHAIN";
    case AugType::SKILL_TIMESTOP:return "SKILL_TIMESTOP";
    case AugType::BULLET_RAIN_3: return "BULLET_RAIN_3";
    case AugType::DRONE_2:       return "DRONE_2";
    case AugType::CHAKRAM_3:     return "CHAKRAM_3";
    // 디버프
    case AugType::D_RMOB_MAX:    return "D_RMOB_MAX";
    case AugType::D_RMOB_HP:     return "D_RMOB_HP";
    case AugType::D_RMOB_DELAY:  return "D_RMOB_DELAY";
    case AugType::D_MOB_SPAWN:   return "D_MOB_SPAWN";
    case AugType::D_APPROACH:    return "D_APPROACH";
    case AugType::D_MOB_SPEED:   return "D_MOB_SPEED";
    case AugType::D_GLASS_HEART: return "D_GLASS_HEART";
    case AugType::D_BULLET_STUCK:return "D_BULLET_STUCK";
    case AugType::D_DRUNK:       return "D_DRUNK";
    case AugType::D_BOMBER_BLAST:return "D_BOMBER_BLAST";
    case AugType::D_BOMBER_BUFF: return "D_BOMBER_BUFF";
    case AugType::D_BOMBER_SPEED:return "D_BOMBER_SPEED";
    case AugType::D_MOB_HP:      return "D_MOB_HP";
    case AugType::D_SLOW_MOVE:   return "D_SLOW_MOVE";
    case AugType::D_SPLITTER:    return "D_SPLITTER";
    case AugType::D_BLINKER:     return "D_BLINKER";
    case AugType::D_ORBITER:     return "D_ORBITER";
    case AugType::D_SPAWNER:     return "D_SPAWNER";
    case AugType::D_SHIELDED:    return "D_SHIELDED";
    case AugType::D_BLEED:       return "D_BLEED";
    case AugType::D_WEAKEN:      return "D_WEAKEN";
    // 특수
    case AugType::S_CHAOS:       return "S_CHAOS";
    case AugType::S_PANDORA:     return "S_PANDORA";
    // 조합
    case AugType::CB_EXECUTIONER: return "CB_EXECUTIONER";
    case AugType::CB_BLOODLORD:   return "CB_BLOODLORD";
    case AugType::CB_PIERCE_TWIN: return "CB_PIERCE_TWIN";
    case AugType::CB_STORMCALLER: return "CB_STORMCALLER";
    case AugType::LASER:         return "LASER_1";
    case AugType::LASER_2:       return "LASER_2";
    case AugType::MELEE_WIDE:    return "MELEE_WIDE";
    case AugType::BLADE_WIND:    return "BLADE_WIND";
    case AugType::POWER_DRAW:    return "POWER_DRAW";
    case AugType::MULTISHOT:     return "MULTISHOT";
    case AugType::SNIPER_AMPLIFIER:return "SNIPER_AMPLIFIER";
    case AugType::HP_UP:           return "REGEN_UP";
    case AugType::FIREWALL:        return "MK2";
    case AugType::REGEN_2:         return "REGEN_UP";
    default:                     return nullptr;
    }
}

// (int)AugType 로 인덱싱 (이넘이 0부터 연속). Augment.h 의 AUG_TYPE_SLOTS 와 동기화.
inline GLuint g_IconTex[AUG_TYPE_SLOTS] = { 0 };
inline char   g_IconBaseDir[260] = "Icons";   // 런타임에 실제 폴더로 확정

inline GLuint g_IconProg = 0, g_IconVAO = 0, g_IconVBO = 0;
inline GLint  g_IconProjLoc = -1, g_IconTintLoc = -1;
inline GLint  g_IconDarkenLoc = -1;
inline GLuint g_IconBatchProg = 0, g_IconBatchVAO = 0, g_IconBatchVBO = 0;
inline GLint  g_IconBatchProjLoc = -1;
inline size_t g_IconBatchCapacityFloats = 0;

struct IconBatchQuad {
    float x, y, w, h;
    float r, g, b, a;
};

// ── GL 파이프라인(텍스처 사각형 + 틴트) ───────────────────
inline GLuint IconCompile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}
inline void InitIconGL() {
    static const char* VS =
        "#version 330 core\n"
        "layout(location=0) in vec2 aPos;\n"
        "layout(location=1) in vec2 aUV;\n"
        "uniform mat4 proj;\n"
        "out vec2 uv;\n"
        "void main(){ gl_Position=proj*vec4(aPos,0,1); uv=aUV; }\n";
    static const char* FS =
        "#version 330 core\n"
        "in vec2 uv;\n"
        "uniform sampler2D tex;\n"
        "uniform vec4 tint;\n"
        "uniform float darkenMode;\n"
        "out vec4 fragColor;\n"
        "void main(){\n"
        "  vec4 t = texture(tex, uv);\n"
        "  float a = t.a * tint.a;\n"
        "  if (a < 0.002) discard;\n"
        "  if (darkenMode > 0.5) {\n"
        "    float darkenA = a * 0.42;\n"
        "    vec3 darkenColor = mix(vec3(1.0), tint.rgb * t.rgb, darkenA);\n"
        "    fragColor = vec4(darkenColor, darkenA);\n"
        "  } else {\n"
        "    fragColor = vec4(tint.rgb * t.rgb, a);\n"
        "  }\n"
        "}\n";
    GLuint v = IconCompile(GL_VERTEX_SHADER, VS);
    GLuint f = IconCompile(GL_FRAGMENT_SHADER, FS);
    g_IconProg = glCreateProgram();
    glAttachShader(g_IconProg, v); glAttachShader(g_IconProg, f);
    glLinkProgram(g_IconProg);
    glDeleteShader(v); glDeleteShader(f);
    g_IconProjLoc = glGetUniformLocation(g_IconProg, "proj");
    g_IconTintLoc = glGetUniformLocation(g_IconProg, "tint");
    g_IconDarkenLoc = glGetUniformLocation(g_IconProg, "darkenMode");
    glUseProgram(g_IconProg);
    glUniform1i(glGetUniformLocation(g_IconProg, "tex"), 0);
    glUseProgram(0);

    glGenVertexArrays(1, &g_IconVAO);
    glGenBuffers(1, &g_IconVBO);
    glBindVertexArray(g_IconVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_IconVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    static const char* BatchVS =
        "#version 330 core\n"
        "layout(location=0) in vec2 aPos;\n"
        "layout(location=1) in vec2 aUV;\n"
        "layout(location=2) in vec4 aColor;\n"
        "uniform mat4 proj;\n"
        "out vec2 uv;\n"
        "out vec4 color;\n"
        "void main(){ gl_Position=proj*vec4(aPos,0,1); uv=aUV; color=aColor; }\n";
    static const char* BatchFS =
        "#version 330 core\n"
        "in vec2 uv;\n"
        "in vec4 color;\n"
        "uniform sampler2D tex;\n"
        "out vec4 fragColor;\n"
        "void main(){\n"
        "  vec4 t=texture(tex,uv);\n"
        "  float a=color.a*t.a;\n"
        "  if(a<0.002) discard;\n"
        "  fragColor=vec4(color.rgb*t.rgb,a);\n"
        "}\n";
    GLuint bv = IconCompile(GL_VERTEX_SHADER, BatchVS);
    GLuint bf = IconCompile(GL_FRAGMENT_SHADER, BatchFS);
    g_IconBatchProg = glCreateProgram();
    glAttachShader(g_IconBatchProg, bv); glAttachShader(g_IconBatchProg, bf);
    glLinkProgram(g_IconBatchProg);
    glDeleteShader(bv); glDeleteShader(bf);
    g_IconBatchProjLoc = glGetUniformLocation(g_IconBatchProg, "proj");
    glUseProgram(g_IconBatchProg);
    glUniform1i(glGetUniformLocation(g_IconBatchProg, "tex"), 0);
    glUseProgram(0);

    glGenVertexArrays(1, &g_IconBatchVAO);
    glGenBuffers(1, &g_IconBatchVBO);
    glBindVertexArray(g_IconBatchVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_IconBatchVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 8 * sizeof(float), nullptr, GL_STREAM_DRAW);
    g_IconBatchCapacityFloats = 6u * 8u;
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(4*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

inline void DrawIconBatch(GLuint tex, const std::vector<IconBatchQuad>& quads,
                          bool additive = false) {
    if (!tex || !g_IconBatchProg || quads.empty()) return;

    static std::vector<float> vertices;
    vertices.resize(quads.size() * 6u * 8u);
    size_t cursor = 0;
    auto vertex = [&](float x, float y, float u, float v,
                      const IconBatchQuad& q) {
        vertices[cursor++] = x;
        vertices[cursor++] = y;
        vertices[cursor++] = u;
        vertices[cursor++] = v;
        vertices[cursor++] = q.r;
        vertices[cursor++] = q.g;
        vertices[cursor++] = q.b;
        vertices[cursor++] = q.a;
    };
    for (const auto& q : quads) {
        vertex(q.x,       q.y,       0.0f, 0.0f, q);
        vertex(q.x + q.w, q.y,       1.0f, 0.0f, q);
        vertex(q.x + q.w, q.y + q.h, 1.0f, 1.0f, q);
        vertex(q.x,       q.y,       0.0f, 0.0f, q);
        vertex(q.x + q.w, q.y + q.h, 1.0f, 1.0f, q);
        vertex(q.x,       q.y + q.h, 0.0f, 1.0f, q);
    }

    BatchFlush();
    g_GfxPass = GfxPass::Icon;
    glUseProgram(g_IconBatchProg);
    glUniformMatrix4fv(g_IconBatchProjLoc, 1, GL_FALSE, g_MainOrtho);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindVertexArray(g_IconBatchVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_IconBatchVBO);
    const size_t needed = vertices.size();
    const GLsizeiptr bytes = (GLsizeiptr)(needed * sizeof(float));
    if (needed > g_IconBatchCapacityFloats) {
        glBufferData(GL_ARRAY_BUFFER, bytes, vertices.data(), GL_STREAM_DRAW);
        g_IconBatchCapacityFloats = needed;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, vertices.data());
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(needed / 8u));
    // Geometry and UI passes expect regular alpha blending.
    if (additive)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// RGBA8 픽셀 → GL 텍스처
inline GLuint IconTexFromRGBA(unsigned char* d, int w, int h) {
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, d);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}

// Large soft masks are normally rendered far below their source resolution.
// Mip levels keep those samples cache-friendly without changing screen size.
inline void IconEnableMipmaps(GLuint tex) {
    if (!tex) return;
    glBindTexture(GL_TEXTURE_2D, tex);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}
inline GLuint IconLoadMem(const unsigned char* buf, int len) {
    int w, h, n;
    unsigned char* d = stbi_load_from_memory(buf, len, &w, &h, &n, 4);
    if (!d) return 0;
    GLuint t = IconTexFromRGBA(d, w, h);
    stbi_image_free(d);
    return t;
}
inline GLuint IconLoadFile(const char* path) {
    int w, h, n;
    unsigned char* d = stbi_load(path, &w, &h, &n, 4);
    if (!d) return 0;
    GLuint t = IconTexFromRGBA(d, w, h);
    stbi_image_free(d);
    return t;
}
inline GLuint IconLoadFileAlphaBoost(const char* path, float boost) {
    int w, h, n;
    unsigned char* d = stbi_load(path, &w, &h, &n, 4);
    if (!d) return 0;
    const int pixels = w * h;
    for (int i = 0; i < pixels; ++i) {
        const int a = (int)((float)d[i * 4 + 3] * boost);
        d[i * 4 + 3] = (unsigned char)(a > 255 ? 255 : a);
    }
    GLuint t = IconTexFromRGBA(d, w, h);
    stbi_image_free(d);
    return t;
}
#ifdef _WIN32
// 임베디드 리소스 "ICON_<name>" (RCDATA) → 텍스처
inline GLuint IconLoadResourceName(const char* resName) {
    HMODULE hm = GetModuleHandleW(NULL);
    HRSRC   hr = FindResourceA(hm, resName, MAKEINTRESOURCEA(10));  // 10 = RT_RCDATA
    if (!hr) return 0;
    HGLOBAL hg = LoadResource(hm, hr);
    if (!hg) return 0;
    const void* p = LockResource(hg);
    DWORD sz = SizeofResource(hm, hr);
    if (!p || sz == 0) return 0;
    return IconLoadMem((const unsigned char*)p, (int)sz);
}
inline GLuint IconLoadResourceAlphaBoost(const char* resName, float boost) {
    HMODULE hm = GetModuleHandleW(NULL);
    HRSRC hr = FindResourceA(hm, resName, MAKEINTRESOURCEA(10));
    if (!hr) return 0;
    HGLOBAL hg = LoadResource(hm, hr);
    const void* p = hg ? LockResource(hg) : nullptr;
    DWORD sz = hg ? SizeofResource(hm, hr) : 0;
    if (!p || sz == 0) return 0;
    int w, h, n;
    unsigned char* d = stbi_load_from_memory((const unsigned char*)p, (int)sz, &w, &h, &n, 4);
    if (!d) return 0;
    const int pixels = w * h;
    for (int i = 0; i < pixels; ++i) {
        const int a = (int)((float)d[i * 4 + 3] * boost);
        d[i * 4 + 3] = (unsigned char)(a > 255 ? 255 : a);
    }
    GLuint t = IconTexFromRGBA(d, w, h);
    stbi_image_free(d);
    return t;
}
#endif

// 이넘명("DMG_UP") → 텍스처. 리소스 우선, 없으면 파일(Icons/) 폴백.
inline GLuint IconLoad(const char* name) {
    if (!name || !name[0]) return 0;
#ifdef _WIN32
    char res[96];
    std::snprintf(res, sizeof(res), "ICON_%s", name);
    GLuint t = IconLoadResourceName(res);
    if (t) return t;
#endif
    char path[320];
    std::snprintf(path, sizeof(path), "%s/%s.png", g_IconBaseDir, name);
    return IconLoadFile(path);
}

// 아이콘 폴더 확정 — 실행 위치(exe) 기준 후보 경로 중 DMG_UP.png 가 있는 곳
inline void ResolveIconDir() {
    const char* cands[] = {
        "Icons", "WiNILL/Icons", "../../WiNILL/Icons", "../../../WiNILL/Icons",
        "../WiNILL/Icons"
    };
    char probe[300];
    for (const char* base : cands) {
        std::snprintf(probe, sizeof(probe), "%s/DMG_UP.png", base);
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif
        FILE* f = std::fopen(probe, "rb");
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
        if (f) { std::fclose(f);
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif
            std::strncpy(g_IconBaseDir, base, sizeof(g_IconBaseDir) - 1);
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
            g_IconBaseDir[sizeof(g_IconBaseDir) - 1] = 0;
            return;
        }
    }
}

// 직업 아이콘 — 인덱스 = JobId 순서 (Achievements.h 의 enum 과 동일 순서로 유지)
//   0:NONE(없음) 1:ASSASSIN 2:BERSERKER 3:BOMBARDIER 4:VAMPIRE 5:SWORDSMAN 6:ARCHER
inline GLuint g_JobIconTex[8] = { 0 };
inline GLuint g_ConstellationLineTex = 0;
inline GLuint g_ConstellationCircleTex = 0;
inline GLuint g_InGameCircleTex = 0;
inline GLuint g_ConfigPanelTex = 0;
inline GLuint g_LeftGradientTex = 0;
inline constexpr float kInGameCircleAlphaBoost = 1.7f;
inline const char* const g_JobIconNames[7] = {
    "",                // JOB_NONE — 아이콘 없음
    "JOB_ASSASSIN", "JOB_BERSERKER", "JOB_BOMBARDIER",
    "JOB_VAMPIRE", "JOB_SWORDSMAN", "JOB_ARCHER"
};

// 모든 증강/직업 아이콘 로드 — 임베디드 리소스 우선(파일 폴백). 없으면 0 → 미표시.
inline void LoadIcons() {
    ResolveIconDir();   // 파일 폴백 경로 확정 (리소스 없을 때만 사용)
    for (int i = 0; i < AUG_TOTAL; i++) {
        AugType t = ALL_AUGS[i].type;
        const char* nm = IconNameForAug(t);
        if (!nm) continue;
        int idx = (int)t;
        if (idx < 0 || idx >= AUG_TYPE_SLOTS) continue;
        if (g_IconTex[idx]) continue;
        g_IconTex[idx] = IconLoad(nm);
    }
    for (int j = 1; j < 7; j++)
        g_JobIconTex[j] = IconLoad(g_JobIconNames[j]);

    // White alpha masks used by constellation renderers. Prefer the embedded
    // copies so every supported launch directory produces identical visuals.
    const char* linePaths[] = { "Resource/Icons/LineTexture.png", "../Resource/Icons/LineTexture.png", "Icons/LineTexture.png" };
    const char* circlePaths[] = { "Resource/Icons/CircleTexture.png", "../Resource/Icons/CircleTexture.png", "Icons/CircleTexture.png" };
    const char* inGameCirclePaths[] = { "Resource/Icons/InGameCircleTexture.png", "../Resource/Icons/InGameCircleTexture.png", "Icons/InGameCircleTexture.png" };
    const char* panelPaths[] = { "Resource/Icons/PanelTexture.png", "../Resource/Icons/PanelTexture.png", "Icons/PanelTexture.png" };
    const char* leftGradientPaths[] = { "Resource/Icons/LeftGradient.png", "../Resource/Icons/LeftGradient.png", "Icons/LeftGradient.png" };
#ifdef _WIN32
    g_ConstellationLineTex = IconLoadResourceAlphaBoost("ICON_CONSTELLATION_LINE", 1.0f);
    g_ConstellationCircleTex = IconLoadResourceAlphaBoost("ICON_CONSTELLATION_CIRCLE", 1.0f);
    g_InGameCircleTex = IconLoadResourceAlphaBoost("ICON_INGAME_CIRCLE", kInGameCircleAlphaBoost);
    g_ConfigPanelTex = IconLoadResourceAlphaBoost("ICON_CONFIG_PANEL", 4.0f);
    g_LeftGradientTex = IconLoadResourceAlphaBoost("ICON_LEFT_GRADIENT", 1.0f);
#endif
    if (!g_ConstellationLineTex) {
        for (const char* path : linePaths) {
            g_ConstellationLineTex = IconLoadFile(path);
            if (g_ConstellationLineTex) break;
        }
    }
    if (!g_ConstellationCircleTex) {
        for (const char* path : circlePaths) {
            g_ConstellationCircleTex = IconLoadFile(path);
            if (g_ConstellationCircleTex) break;
        }
    }
    if (!g_InGameCircleTex) {
        for (const char* path : inGameCirclePaths) {
            g_InGameCircleTex = IconLoadFileAlphaBoost(path, kInGameCircleAlphaBoost);
            if (g_InGameCircleTex) break;
        }
    }
    if (!g_ConfigPanelTex) {
        for (const char* path : panelPaths) {
            g_ConfigPanelTex = IconLoadFileAlphaBoost(path, 4.0f);
            if (g_ConfigPanelTex) break;
        }
    }
    if (!g_LeftGradientTex) {
        for (const char* path : leftGradientPaths) {
            g_LeftGradientTex = IconLoadFileAlphaBoost(path, 1.0f);
            if (g_LeftGradientTex) break;
        }
    }

    IconEnableMipmaps(g_ConstellationCircleTex);
    IconEnableMipmaps(g_InGameCircleTex);
}

inline GLuint JobIcon(int jobId) {
    if (jobId <= 0 || jobId >= 8) return 0;
    return g_JobIconTex[jobId];
}

// AugType 텍스처 (없으면 티어 폴백)
inline GLuint IconFor(AugType t) {
    int idx = (int)t;
    if (idx >= 0 && idx < AUG_TYPE_SLOTS && g_IconTex[idx]) return g_IconTex[idx];
    switch (t) {  // 티어 II/III → 기본 티어 그림 재사용
    case AugType::BULLET_RAIN_2:
    case AugType::BULLET_RAIN_3:
    case AugType::BULLET_RAIN_ETERNAL: return g_IconTex[(int)AugType::BULLET_RAIN];
    case AugType::CHAKRAM_2:
    case AugType::CHAKRAM_3:     return g_IconTex[(int)AugType::CHAKRAM];
    case AugType::DRONE_2:       return g_IconTex[(int)AugType::DRONE];
    // 티어 II → 기본 티어 그림 재사용 (별도 아이콘 없는 것만)
    case AugType::PIERCE_2:      return g_IconTex[(int)AugType::PIERCE];
    case AugType::PIERCE_RAILSLUG: return g_IconTex[(int)AugType::PIERCE];
    case AugType::TWIN_2:        return g_IconTex[(int)AugType::TWIN];
    case AugType::DRONE_HIVE:    return g_IconTex[(int)AugType::DRONE];
    case AugType::LASER_CONVERGE:return g_IconTex[(int)AugType::LASER];
    // 프로세스류 디버프 확장 — 전용 아이콘 없으면 유사 디버프 아이콘 재사용
    case AugType::D_MOB_PACK:    return g_IconTex[(int)AugType::D_MOB_SPAWN];
    case AugType::D_MOB_ELITE:   return g_IconTex[(int)AugType::D_MOB_HP];
    case AugType::D_MOB_FRENZY:  return g_IconTex[(int)AugType::D_MOB_SPEED];
    case AugType::D_SCHEDULER:    return g_IconTex[(int)AugType::D_MOB_HP];
    case AugType::D_TROJAN_BOOST: return g_IconTex[(int)AugType::D_BLINKER];
    case AugType::D_CRASHER_BOOST:return g_IconTex[(int)AugType::D_MOB_SPEED];
    case AugType::D_BADSECTOR:    return g_IconTex[(int)AugType::D_SLOW_MOVE];
    case AugType::D_REGERROR:     return g_IconTex[(int)AugType::D_MOB_HP];
    case AugType::D_DDOS:         return g_IconTex[(int)AugType::D_MOB_SPAWN];
    case AugType::D_WEAVER_BOOST: return g_IconTex[(int)AugType::D_MOB_SPEED];
    case AugType::D_BRUTE_BOOST:  return g_IconTex[(int)AugType::D_MOB_HP];
    case AugType::LIFESTEAL_2:    return g_IconTex[(int)AugType::LIFESTEAL];
    case AugType::CHAIN_2:        return g_IconTex[(int)AugType::CHAIN];
    case AugType::CHAKRAM_SINGULARITY: return g_IconTex[(int)AugType::CHAKRAM];
    case AugType::HE_SHELLS:      return g_IconTex[(int)AugType::CANNON];
    case AugType::SKILL_FOCUS:    return g_IconTex[(int)AugType::SNIPER];
    case AugType::SKILL_DASH_UP:  return g_IconTex[(int)AugType::LIGHT_STEP];
    case AugType::SMG_COMPRESSOR: return g_IconTex[(int)AugType::MINIGUN];
    case AugType::RIFLE_STABILITY:return g_IconTex[(int)AugType::BROKEN_SIGHT];
    case AugType::SNIPER_AMPLIFIER:return g_IconTex[(int)AugType::SNIPER];
    case AugType::HP_UP:           return g_IconTex[(int)AugType::REGEN_UP];
    case AugType::FIREWALL:        return g_IconTex[(int)AugType::MK2];
    case AugType::REGEN_2:         return g_IconTex[(int)AugType::REGEN_UP];
    case AugType::MINIGUN_2:       return g_IconTex[(int)AugType::MINIGUN];
    case AugType::MINIGUN_CYCLONE: return g_IconTex[(int)AugType::MINIGUN];
    case AugType::DEATH_BLAST_2:   return g_IconTex[(int)AugType::DEATH_BLAST];
    case AugType::HACK_FIREWALL:   return g_IconTex[(int)AugType::HACK_RANGED];
    case AugType::REVOLVER_SILVER: return g_IconTex[(int)AugType::REVOLVER_OVERLOAD];
    case AugType::HE_SHELLS_2:     return g_IconTex[(int)AugType::HE_SHELLS];
    case AugType::D_SPLITTER_BOOST:return g_IconTex[(int)AugType::D_SPLITTER];
    default:                     break;
    }
    // 조합 증강 — 전용 아이콘(CB_*.png) 없으면 레시피 첫 재료 아이콘 재사용
    for (int c = 0; c < COMBO_COUNT; c++)
        if (COMBO_DEFS[c].result == t)
            return IconFor(COMBO_DEFS[c].reqs[0]);
    return 0;
}

// 화면(y-down ortho) 좌표에 틴트 적용해 아이콘 그리기
inline void DrawIcon(GLuint tex, float x, float y, float w, float h,
                     float r, float g, float b, float a) {
    if (!tex || !g_IconProg) return;
    if (g_GfxPass != GfxPass::Icon)
        BatchFlush();
    g_GfxPass = GfxPass::Icon;
    glUseProgram(g_IconProg);
    glUniformMatrix4fv(g_IconProjLoc, 1, GL_FALSE, g_MainOrtho);
    glUniform4f(g_IconTintLoc, r, g, b, a);
    if (g_IconDarkenLoc >= 0) glUniform1f(g_IconDarkenLoc, 0.0f);
    float vv[] = {
        x,     y,     0.0f, 0.0f,
        x + w, y,     1.0f, 0.0f,
        x + w, y + h, 1.0f, 1.0f,
        x,     y,     0.0f, 0.0f,
        x + w, y + h, 1.0f, 1.0f,
        x,     y + h, 0.0f, 1.0f,
    };
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindVertexArray(g_IconVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_IconVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vv), vv);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// CircleTexture contrast pass. The fragment shader maps transparent pixels
// toward white, then GL_MIN keeps the darker RGB value already on screen.
// This lets a strong black field remain dark when another CircleTexture
// overlaps it without changing the normal icon/text alpha blend path.
inline void DrawIconDarken(GLuint tex, float x, float y, float w, float h,
                           float r, float g, float b, float a) {
    if (!tex || !g_IconProg) return;
    if (g_GfxPass != GfxPass::Icon)
        BatchFlush();
    g_GfxPass = GfxPass::Icon;
    glUseProgram(g_IconProg);
    glUniformMatrix4fv(g_IconProjLoc, 1, GL_FALSE, g_MainOrtho);
    glUniform4f(g_IconTintLoc, r, g, b, a);
    if (g_IconDarkenLoc >= 0) glUniform1f(g_IconDarkenLoc, 1.0f);
    float vv[] = {
        x,     y,     0.0f, 0.0f,
        x + w, y,     1.0f, 0.0f,
        x + w, y + h, 1.0f, 1.0f,
        x,     y,     0.0f, 0.0f,
        x + w, y + h, 1.0f, 1.0f,
        x,     y + h, 0.0f, 1.0f,
    };
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindVertexArray(g_IconVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_IconVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vv), vv);
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_MIN, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (g_IconDarkenLoc >= 0) glUniform1f(g_IconDarkenLoc, 0.0f);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// 중심 기준 회전 텍스처 쿼드 (탄환 세례 미사일 스프라이트 등). 화면(y-down) 좌표.
inline void DrawIconRot(GLuint tex, float cx, float cy, float halfW, float halfH,
                        float angle, float r, float g, float b, float a) {
    if (!tex || !g_IconProg) return;
    if (g_GfxPass != GfxPass::Icon)
        BatchFlush();
    g_GfxPass = GfxPass::Icon;
    glUseProgram(g_IconProg);
    glUniformMatrix4fv(g_IconProjLoc, 1, GL_FALSE, g_MainOrtho);
    glUniform4f(g_IconTintLoc, r, g, b, a);
    if (g_IconDarkenLoc >= 0) glUniform1f(g_IconDarkenLoc, 0.0f);
    float ca = std::cos(angle), sa = std::sin(angle);
    auto rot = [&](float ox, float oy, float& X, float& Y){ X = cx + ox*ca - oy*sa; Y = cy + ox*sa + oy*ca; };
    float x0,y0,x1,y1,x2,y2,x3,y3;
    rot(-halfW,-halfH,x0,y0); rot(halfW,-halfH,x1,y1);
    rot(halfW, halfH,x2,y2); rot(-halfW, halfH,x3,y3);
    float vv[] = { x0,y0,0,0,  x1,y1,1,0,  x2,y2,1,1,   x0,y0,0,0,  x2,y2,1,1,  x3,y3,0,1 };
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindVertexArray(g_IconVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_IconVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vv), vv);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ── 탄환 세례 미사일 스프라이트 ──
//   원본 PNG 가 흰 배경+검정 로켓+하단 출처표기라, 그대로는 못 씀.
//   → 하단 출처표기 크롭 + 휘도 반전(흰배경 투명/검정 로켓 불투명) + RGB 흰색(틴트 가능)
inline GLuint g_RainMissileTex = 0;
inline GLuint MakeRocketSprite(unsigned char* d, int w, int h) {
    int cropH = (int)(h * 0.80f); if (cropH < 1) cropH = h;   // 하단 20%(출처표기) 제거
    for (int i = 0; i < w * cropH; i++) {
        unsigned char* p = d + i * 4;
        int lum = (p[0]*299 + p[1]*587 + p[2]*114) / 1000;    // 0=검정 255=흰
        // 알파 = min(기존 알파, 255-휘도). 흰 배경/투명 배경 둘 다 안전하게 제거:
        //   흰 불투명배경(a=255,lum255)→0, 검정로켓(a=255,lum0)→255,
        //   투명배경(a=0)→0 (기존알파 0 우선 → 검정-투명 배경이 불투명 사각형 되던 버그 fix)
        int na = 255 - lum; if ((int)p[3] < na) na = p[3];
        p[0] = 255; p[1] = 255; p[2] = 255;
        p[3] = (unsigned char)na;
    }
    return IconTexFromRGBA(d, w, cropH);
}
inline void InitRainMissileTex() {
#ifdef _WIN32
    HMODULE hm = GetModuleHandleW(NULL);
    HRSRC hr = FindResourceA(hm, "ICON_RAIN_MISSILE", MAKEINTRESOURCEA(10));
    if (hr) {
        HGLOBAL hg = LoadResource(hm, hr); const void* pp = LockResource(hg);
        DWORD sz = SizeofResource(hm, hr);
        if (pp && sz) {
            int w,h,n; unsigned char* d = stbi_load_from_memory((const unsigned char*)pp,(int)sz,&w,&h,&n,4);
            if (d) { g_RainMissileTex = MakeRocketSprite(d,w,h); stbi_image_free(d); return; }
        }
    }
#endif
    char path[320]; std::snprintf(path, sizeof(path), "%s/BULLET_RAIN.png", g_IconBaseDir);
    int w,h,n; unsigned char* d = stbi_load(path,&w,&h,&n,4);
    if (d) { g_RainMissileTex = MakeRocketSprite(d,w,h); stbi_image_free(d); }
}

inline void DrawAugIcon(AugType t, float x, float y, float sz,
                        float r, float g, float b, float a) {
    DrawIcon(IconFor(t), x, y, sz, sz, r, g, b, a);
}
