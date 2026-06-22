// ─────────────────────────────────────────────────────────────
//  Audio — miniaudio 구현부 (이 .cpp 에서만 IMPLEMENTATION 정의)
// ─────────────────────────────────────────────────────────────
#define _CRT_SECURE_NO_WARNINGS
#define MINIAUDIO_IMPLEMENTATION
// 안 쓰는 백엔드/기능 비활성 → 컴파일 가볍게
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#include "miniaudio.h"
#include "Audio.h"
#include <cstring>
#include <cstdlib>
#include <string>
#include <chrono>
#ifdef _WIN32
#  include <windows.h>
#endif

namespace {
    ma_engine g_engine;
    bool g_inited  = false;
    bool g_enabled = true;

    std::string g_base;
    void ResolveBase() {
        // PlatformChdirToExeDir() 이후 상대경로(Resource/...) 사용
        g_base = "";
    }
    std::string FullPath(const char* rel) { return g_base + rel; }

    struct SfxSlot { ma_sound snd; bool ok = false; };
    SfxSlot g_sfx[(int)Audio::Sfx::COUNT];

    struct SfxDef { const char* path; float vol; unsigned minMs; bool pitchVary; };
    // 게인/쓰로틀/피치 테이블 — 도배·기계감 방지 (파일 볼륨 안 건드림)
    //   minMs   : 최소 재생 간격(ms). 도배 방지 (연사/대량처치)
    //   pitchVary: 매번 피치 ±10% 랜덤 → 반복돼도 기계 같지 않게
    const SfxDef SFX_DEFS[(int)Audio::Sfx::COUNT] = {
        { "Resource/Audio/sfx_shoot.wav",         0.20f, 25,  true  },  // 발사 — 작게+피치변주+25ms 쓰로틀
        { "Resource/Audio/sfx_kill.wav",          0.30f, 55,  true  },  // 처치 — 대량처치 도배 방지(55ms)
        { "Resource/Audio/sfx_hurt.wav",          0.55f, 80,  false },
        { "Resource/Audio/sfx_death.wav",         0.85f, 0,   false },
    };
    unsigned long long g_sfxLastMs[(int)Audio::Sfx::COUNT] = {0};

    ma_sound g_bgm;
    bool     g_bgmActive = false;
    char     g_bgmPath[64] = "";

    void StartBgm(const char* path, float vol) {
        if (!g_inited || !g_enabled) return;
        if (g_bgmActive && std::strcmp(g_bgmPath, path) == 0) return;  // 이미 같은 곡
        if (g_bgmActive) { ma_sound_uninit(&g_bgm); g_bgmActive = false; g_bgmPath[0] = 0; }
        std::string full = FullPath(path);
        if (ma_sound_init_from_file(&g_engine, full.c_str(), MA_SOUND_FLAG_STREAM,
                                    NULL, NULL, &g_bgm) == MA_SUCCESS) {
            ma_sound_set_looping(&g_bgm, MA_TRUE);
            ma_sound_set_volume(&g_bgm, vol);
            ma_sound_start(&g_bgm);
            g_bgmActive = true;
            std::strncpy(g_bgmPath, path, 63);
            g_bgmPath[63] = 0;
        }
    }
}

void Audio::Init() {
    if (g_inited) return;
    if (ma_engine_init(NULL, &g_engine) != MA_SUCCESS) return;
    g_inited = true;
    ResolveBase();
    for (int i = 0; i < (int)Sfx::COUNT; i++) {
        std::string full = FullPath(SFX_DEFS[i].path);
        if (ma_sound_init_from_file(&g_engine, full.c_str(),
                MA_SOUND_FLAG_DECODE, NULL, NULL, &g_sfx[i].snd) == MA_SUCCESS) {
            ma_sound_set_volume(&g_sfx[i].snd, SFX_DEFS[i].vol);
            g_sfx[i].ok = true;
        }
    }
}

void Audio::Shutdown() {
    if (!g_inited) return;
    if (g_bgmActive) { ma_sound_uninit(&g_bgm); g_bgmActive = false; }
    for (int i = 0; i < (int)Sfx::COUNT; i++)
        if (g_sfx[i].ok) ma_sound_uninit(&g_sfx[i].snd);
    ma_engine_uninit(&g_engine);
    g_inited = false;
}

void Audio::PlaySfx(Sfx s) {
    if (!g_inited || !g_enabled) return;
    int i = (int)s;
    if (i < 0 || i >= (int)Sfx::COUNT || !g_sfx[i].ok) return;
    // 쓰로틀 — 도배 방지 (연사/대량처치)
    if (SFX_DEFS[i].minMs > 0) {
        unsigned long long now = 0;
#ifdef _WIN32
        now = GetTickCount64();
#else
        using clock = std::chrono::steady_clock;
        now = (unsigned long long)std::chrono::duration_cast<std::chrono::milliseconds>(
            clock::now().time_since_epoch()).count();
#endif
        if (now - g_sfxLastMs[i] < SFX_DEFS[i].minMs) return;
        g_sfxLastMs[i] = now;
    }
    // 피치 ±10% 랜덤 — 반복돼도 기계 같지 않게
    if (SFX_DEFS[i].pitchVary) {
        float p = 0.90f + (float)(std::rand() % 21) * 0.01f;   // 0.90~1.10
        ma_sound_set_pitch(&g_sfx[i].snd, p);
    }
    ma_sound_seek_to_pcm_frame(&g_sfx[i].snd, 0);   // 재트리거 = 처음부터
    ma_sound_start(&g_sfx[i].snd);
}

void Audio::PlayBgmMain() { StartBgm("Resource/Audio/bgm_main.mp3", 0.50f); }
void Audio::PlayBgmBoss() { StartBgm("Resource/Audio/bgm_boss.mp3", 0.55f); }

void Audio::StopBgm() {
    if (g_bgmActive) { ma_sound_uninit(&g_bgm); g_bgmActive = false; g_bgmPath[0] = 0; }
}

void Audio::SetEnabled(bool on) { g_enabled = on; if (!on) StopBgm(); }
bool Audio::IsEnabled() { return g_enabled; }

void Audio::SetVolume(float v) {
    if (!g_inited) return;
    if (v < 0.0f) v = 0.0f; if (v > 1.0f) v = 1.0f;
    ma_engine_set_volume(&g_engine, v);
}
