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
#include "EmbeddedResource.h"
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
    bool g_bgmEnabled = true;
    bool g_sfxEnabled = true;
    bool g_monoOutput = false;
    float g_masterVolume = 1.0f;

    std::string g_base;
    void ResolveBase() {
        // PlatformChdirToExeDir() 이후 상대경로(Resource/...) 사용
        g_base = "";
    }
    std::string FullPath(const char* rel) { return g_base + rel; }

    struct SfxSlot {
        ma_sound snd;
        ma_decoder decoder;
        bool decoderActive = false;
        bool ok = false;
    };
    SfxSlot g_sfx[(int)Audio::Sfx::COUNT];

    struct SfxDef { const char* path; float vol; unsigned minMs; bool pitchVary; };
    // 게인/쓰로틀/피치 테이블 — 도배·기계감 방지 (파일 볼륨 안 건드림)
    //   minMs   : 최소 재생 간격(ms). 도배 방지 (연사/대량처치)
    //   pitchVary: 매번 피치 ±10% 랜덤 → 반복돼도 기계 같지 않게
    const SfxDef SFX_DEFS[(int)Audio::Sfx::COUNT] = {
        { "Resource/Audio/sfx_shoot.wav", 0.20f, 25, true  },
        { "Resource/Audio/sfx_kill.wav",  0.30f, 55, true  },
    };
    unsigned long long g_sfxLastMs[(int)Audio::Sfx::COUNT] = {0};

    ma_sound g_bgm;
    ma_decoder g_bgmDecoder;
    bool     g_bgmDecoderActive = false;
    bool     g_bgmActive = false;
    char     g_bgmPath[64] = "";

    void ReleaseSoundObjects() {
        if (g_bgmActive) { ma_sound_uninit(&g_bgm); g_bgmActive = false; }
        if (g_bgmDecoderActive) {
            ma_decoder_uninit(&g_bgmDecoder);
            g_bgmDecoderActive = false;
        }
        g_bgmPath[0] = 0;
        for (int i = 0; i < (int)Audio::Sfx::COUNT; ++i) {
            if (g_sfx[i].ok) {
                ma_sound_uninit(&g_sfx[i].snd);
                g_sfx[i].ok = false;
            }
            if (g_sfx[i].decoderActive) {
                ma_decoder_uninit(&g_sfx[i].decoder);
                g_sfx[i].decoderActive = false;
            }
        }
    }

    bool InitializeEngine(bool mono) {
        ma_engine_config config = ma_engine_config_init();
        config.channels = mono ? 1 : 2;
        return ma_engine_init(&config, &g_engine) == MA_SUCCESS;
    }

    void LoadSoundEffects() {
        for (int i = 0; i < (int)Audio::Sfx::COUNT; ++i) {
            std::string full = FullPath(SFX_DEFS[i].path);
            const char* embeddedName = nullptr;
            switch ((Audio::Sfx)i) {
            case Audio::Sfx::Shoot: embeddedName = "AUDIO_SFX_SHOOT"; break;
            case Audio::Sfx::Kill:  embeddedName = "AUDIO_SFX_KILL"; break;
            default: break;
            }
            EmbeddedResourceView embedded;
            if (embeddedName && LoadEmbeddedResource(embeddedName, embedded) &&
                ma_decoder_init_memory(embedded.data, (size_t)embedded.size,
                                       nullptr, &g_sfx[i].decoder) == MA_SUCCESS) {
                g_sfx[i].decoderActive = true;
                g_sfx[i].ok = ma_sound_init_from_data_source(
                    &g_engine, &g_sfx[i].decoder, MA_SOUND_FLAG_DECODE,
                    nullptr, &g_sfx[i].snd) == MA_SUCCESS;
                if (!g_sfx[i].ok) {
                    ma_decoder_uninit(&g_sfx[i].decoder);
                    g_sfx[i].decoderActive = false;
                }
            }
            if (!g_sfx[i].ok &&
                ma_sound_init_from_file(&g_engine, full.c_str(),
                    MA_SOUND_FLAG_DECODE, nullptr, nullptr,
                    &g_sfx[i].snd) == MA_SUCCESS)
                g_sfx[i].ok = true;
            if (g_sfx[i].ok)
                ma_sound_set_volume(&g_sfx[i].snd, SFX_DEFS[i].vol);
        }
    }

    void StartBgm(const char* path, float vol) {
        if (!g_inited || !g_enabled || !g_bgmEnabled) return;
        if (g_bgmActive && std::strcmp(g_bgmPath, path) == 0) return;  // 이미 같은 곡
        if (g_bgmActive) {
            ma_sound_uninit(&g_bgm);
            g_bgmActive = false;
        }
        if (g_bgmDecoderActive) {
            ma_decoder_uninit(&g_bgmDecoder);
            g_bgmDecoderActive = false;
        }
        g_bgmPath[0] = 0;

        EmbeddedResourceView embedded;
        if (LoadEmbeddedResource("AUDIO_BGM_MAIN", embedded) &&
            ma_decoder_init_memory(embedded.data, (size_t)embedded.size,
                                   nullptr, &g_bgmDecoder) == MA_SUCCESS) {
            g_bgmDecoderActive = true;
            if (ma_sound_init_from_data_source(&g_engine, &g_bgmDecoder,
                                               MA_SOUND_FLAG_STREAM, nullptr,
                                               &g_bgm) == MA_SUCCESS) {
                ma_sound_set_looping(&g_bgm, MA_TRUE);
                ma_sound_set_volume(&g_bgm, vol);
                ma_sound_start(&g_bgm);
                g_bgmActive = true;
                std::strncpy(g_bgmPath, path, 63);
                g_bgmPath[63] = 0;
                return;
            }
            ma_decoder_uninit(&g_bgmDecoder);
            g_bgmDecoderActive = false;
        }

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

void Audio::Init(bool monoOutput) {
    if (g_inited) return;
    g_monoOutput = monoOutput;
    if (!InitializeEngine(g_monoOutput)) {
        if (!g_monoOutput || !InitializeEngine(false)) return;
        g_monoOutput = false;
    }
    g_inited = true;
    ResolveBase();
    for (int i = 0; i < (int)Sfx::COUNT; i++) {
        std::string full = FullPath(SFX_DEFS[i].path);
        const char* embeddedName = nullptr;
        switch ((Sfx)i) {
        case Sfx::Shoot: embeddedName = "AUDIO_SFX_SHOOT"; break;
        case Sfx::Kill:  embeddedName = "AUDIO_SFX_KILL";  break;
        default: break;
        }

        EmbeddedResourceView embedded;
        if (embeddedName && LoadEmbeddedResource(embeddedName, embedded) &&
            ma_decoder_init_memory(embedded.data, (size_t)embedded.size,
                                   nullptr, &g_sfx[i].decoder) == MA_SUCCESS) {
            g_sfx[i].decoderActive = true;
            g_sfx[i].ok =
                ma_sound_init_from_data_source(&g_engine, &g_sfx[i].decoder,
                                               MA_SOUND_FLAG_DECODE, nullptr,
                                               &g_sfx[i].snd) == MA_SUCCESS;
            if (!g_sfx[i].ok) {
                ma_decoder_uninit(&g_sfx[i].decoder);
                g_sfx[i].decoderActive = false;
            }
        }
        if (!g_sfx[i].ok &&
            ma_sound_init_from_file(&g_engine, full.c_str(),
                                    MA_SOUND_FLAG_DECODE, NULL, NULL,
                                    &g_sfx[i].snd) == MA_SUCCESS) {
            g_sfx[i].ok = true;
        }
        if (g_sfx[i].ok)
            ma_sound_set_volume(&g_sfx[i].snd, SFX_DEFS[i].vol);
    }
}

void Audio::Shutdown() {
    if (!g_inited) return;
    ReleaseSoundObjects();
    ma_engine_uninit(&g_engine);
    g_inited = false;
}

void Audio::PlaySfx(Sfx s) {
    if (!g_inited || !g_enabled || !g_sfxEnabled) return;
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

void Audio::StopBgm() {
    if (g_bgmActive) { ma_sound_uninit(&g_bgm); g_bgmActive = false; }
    if (g_bgmDecoderActive) {
        ma_decoder_uninit(&g_bgmDecoder);
        g_bgmDecoderActive = false;
    }
    g_bgmPath[0] = 0;
}

void Audio::SetEnabled(bool on) {
    if (g_enabled == on) return;
    g_enabled = on;
    if (!on) {
        StopBgm();
        if (g_inited) ma_engine_stop(&g_engine);
    } else if (g_inited) {
        ma_engine_start(&g_engine);
    }
}
bool Audio::IsEnabled() { return g_enabled; }

void Audio::SetBgmEnabled(bool on) {
    g_bgmEnabled = on;
    if (!on) StopBgm();
}

void Audio::SetSfxEnabled(bool on) {
    if (g_sfxEnabled == on) return;
    g_sfxEnabled = on;
    if (!on && g_inited) {
        for (int i = 0; i < (int)Sfx::COUNT; ++i)
            if (g_sfx[i].ok) ma_sound_stop(&g_sfx[i].snd);
    }
}

bool Audio::SetMonoOutput(bool mono) {
    if (mono == g_monoOutput) return true;
    if (!g_inited) {
        g_monoOutput = mono;
        return true;
    }

    const bool resumeBgm = g_bgmActive;
    char bgmPath[sizeof(g_bgmPath)] = {};
    std::strncpy(bgmPath, g_bgmPath, sizeof(bgmPath) - 1);
    const bool previousMono = g_monoOutput;
    ReleaseSoundObjects();
    ma_engine_uninit(&g_engine);
    g_inited = false;

    if (!InitializeEngine(mono)) {
        if (!InitializeEngine(previousMono)) return false;
        g_monoOutput = previousMono;
    } else {
        g_monoOutput = mono;
    }
    g_inited = true;
    LoadSoundEffects();
    ma_engine_set_volume(&g_engine, g_masterVolume);
    if (resumeBgm && bgmPath[0] && g_enabled && g_bgmEnabled)
        StartBgm(bgmPath, 0.50f);
    if (!g_enabled) ma_engine_stop(&g_engine);
    return g_monoOutput == mono;
}

bool Audio::IsMonoOutput() { return g_monoOutput; }

void Audio::SetVolume(float v) {
    if (v < 0.0f) v = 0.0f; if (v > 1.0f) v = 1.0f;
    g_masterVolume = v;
    if (g_inited) ma_engine_set_volume(&g_engine, v);
}
