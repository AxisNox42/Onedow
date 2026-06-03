#pragma once
// ─────────────────────────────────────────────────────────────
// 아이콘(픽토그램) 시스템 — game-icons.net 흰색 PNG 를 텍스처로 로드,
//   AugType → 텍스처 매핑, 등급색 틴트로 그리는 drawIcon 헬퍼.
//   파일명 = AugType 이넘명 (예: Icons/DMG_UP.png). 흰색+투명배경 권장.
// ─────────────────────────────────────────────────────────────
#include <glad/glad.h>
#include <cstdio>
#include <cstring>
#include "stb_image.h"
#include "Augment.h"
#include "DrawPrim.h"   // g_MainOrtho

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
    case AugType::D_BLEED:       return "D_BLEED";
    case AugType::D_WEAKEN:      return "D_WEAKEN";
    // 특수
    case AugType::S_CHAOS:       return "S_CHAOS";
    case AugType::S_PANDORA:     return "S_PANDORA";
    default:                     return nullptr;
    }
}

// (int)AugType 로 인덱싱 (이넘이 0부터 연속). 여유 있게 잡음.
inline GLuint g_IconTex[96] = { 0 };
inline char   g_IconBaseDir[260] = "Icons";   // 런타임에 실제 폴더로 확정

inline GLuint g_IconProg = 0, g_IconVAO = 0, g_IconVBO = 0;
inline GLint  g_IconProjLoc = -1, g_IconTintLoc = -1;

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
        "out vec4 fragColor;\n"
        "void main(){\n"
        "  vec4 t = texture(tex, uv);\n"
        "  fragColor = vec4(tint.rgb * t.rgb, t.a * tint.a);\n"
        "}\n";
    GLuint v = IconCompile(GL_VERTEX_SHADER, VS);
    GLuint f = IconCompile(GL_FRAGMENT_SHADER, FS);
    g_IconProg = glCreateProgram();
    glAttachShader(g_IconProg, v); glAttachShader(g_IconProg, f);
    glLinkProgram(g_IconProg);
    glDeleteShader(v); glDeleteShader(f);
    g_IconProjLoc = glGetUniformLocation(g_IconProg, "proj");
    g_IconTintLoc = glGetUniformLocation(g_IconProg, "tint");
    glUseProgram(g_IconProg);
    glUniform1i(glGetUniformLocation(g_IconProg, "tex"), 0);
    glUseProgram(0);

    glGenVertexArrays(1, &g_IconVAO);
    glGenBuffers(1, &g_IconVBO);
    glBindVertexArray(g_IconVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_IconVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

inline GLuint IconLoadPNG(const char* path) {
    int w, h, n;
    unsigned char* d = stbi_load(path, &w, &h, &n, 4);
    if (!d) return 0;
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, d);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(d);
    return t;
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
inline const char* const g_JobIconNames[7] = {
    "",                // JOB_NONE — 아이콘 없음
    "JOB_ASSASSIN", "JOB_BERSERKER", "JOB_BOMBARDIER",
    "JOB_VAMPIRE", "JOB_SWORDSMAN", "JOB_ARCHER"
};

// 모든 증강/직업 아이콘 로드 (없는 파일은 그냥 0 → 폴백/미표시)
inline void LoadIcons() {
    ResolveIconDir();
    for (int i = 0; i < AUG_TOTAL; i++) {
        AugType t = ALL_AUGS[i].type;
        const char* nm = IconNameForAug(t);
        if (!nm) continue;
        int idx = (int)t;
        if (idx < 0 || idx >= 96) continue;
        if (g_IconTex[idx]) continue;
        char path[320];
        std::snprintf(path, sizeof(path), "%s/%s.png", g_IconBaseDir, nm);
        g_IconTex[idx] = IconLoadPNG(path);
    }
    for (int j = 1; j < 7; j++) {
        char path[320];
        std::snprintf(path, sizeof(path), "%s/%s.png", g_IconBaseDir, g_JobIconNames[j]);
        g_JobIconTex[j] = IconLoadPNG(path);
    }
}

inline GLuint JobIcon(int jobId) {
    if (jobId <= 0 || jobId >= 8) return 0;
    return g_JobIconTex[jobId];
}

// AugType 텍스처 (없으면 티어 폴백)
inline GLuint IconFor(AugType t) {
    int idx = (int)t;
    if (idx >= 0 && idx < 96 && g_IconTex[idx]) return g_IconTex[idx];
    switch (t) {  // 티어 II/III → 기본 티어 그림 재사용
    case AugType::BULLET_RAIN_2:
    case AugType::BULLET_RAIN_3: return g_IconTex[(int)AugType::BULLET_RAIN];
    case AugType::CHAKRAM_2:
    case AugType::CHAKRAM_3:     return g_IconTex[(int)AugType::CHAKRAM];
    case AugType::DRONE_2:       return g_IconTex[(int)AugType::DRONE];
    default:                     return 0;
    }
}

// 화면(y-down ortho) 좌표에 틴트 적용해 아이콘 그리기
inline void DrawIcon(GLuint tex, float x, float y, float w, float h,
                     float r, float g, float b, float a) {
    if (!tex || !g_IconProg) return;
    glUseProgram(g_IconProg);
    glUniformMatrix4fv(g_IconProjLoc, 1, GL_FALSE, g_MainOrtho);
    glUniform4f(g_IconTintLoc, r, g, b, a);
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

inline void DrawAugIcon(AugType t, float x, float y, float sz,
                        float r, float g, float b, float a) {
    DrawIcon(IconFor(t), x, y, sz, sz, r, g, b, a);
}
