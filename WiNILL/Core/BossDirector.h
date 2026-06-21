#pragma once
#include <glm/glm.hpp>
#include "Settings.h"

// 보스 pick: 2 VOLLEY 4 GLITCH 7 C2 8 FORK 9 RITE
namespace BossDir {

inline int& RotIdx() {
    static int idx = 0;
    return idx;
}

inline void ResetRotation() { RotIdx() = 0; }

inline int RollScorePick() {
    static const int kRot[] = { 4, 2, 7, 8, 9 };
    return kRot[RotIdx()++ % 5];
}

// ── ACT 테마 (보스 예고 ~ 처치까지) ─────────────────────────
inline int  g_ActBossPick  = -1;   // -1 = 워밍업 ACT
inline int  g_ActClears    = 0;    // 이번 런 보스 클리어 수
inline bool g_ActEndless   = false;

inline void ResetAct() {
    g_ActBossPick = -1;
    g_ActClears   = 0;
    g_ActEndless  = false;
}

inline void SetActTheme(int pick) { g_ActBossPick = pick; }

inline void OnBossDefeated() {
    ++g_ActClears;
    if (g_ActClears >= 5) g_ActEndless = true;
}

struct ActRules {
    float intensityCap;    // score/100k 상한
    float hpIntensityCap;
    float spawnMult;       // rampSpawn 배율
    float speedMult;       // mobSpdRamp 배율
    int   eliteBias;       // elitePct 가산
    int   varietyBias;     // varietyPct 가산
};

inline ActRules WarmupRules() {
    return { 0.55f, 1.2f, 0.88f, 0.92f, 0, 0 };
}

inline ActRules EndlessRules() {
    return { 6.0f, 18.0f, 1.08f, 1.0f, 8, 6 };
}

inline ActRules RulesForPick(int pick) {
    switch (pick) {
    case 1:  return { 2.8f, 4.8f, 1.04f, 1.02f, 2, 3 };   // UNKNOWN — 기술·필드 pin
    case 2:  return { 3.0f, 5.0f, 1.05f, 1.06f, 4, 2 };   // VOLLEY — 탄막·원거리
    case 4:  return { 2.5f, 4.5f, 1.0f,  1.0f,  8, 4 };   // GLITCH — 엘리트
    case 7:  return { 3.5f, 6.0f, 1.18f, 1.0f,  3, 8 };   // C2 — swarm
    case 8:  return { 4.0f, 7.0f, 1.22f, 1.04f, 2, 10 };  // FORK — 물량
    case 9:  return { 4.5f, 8.0f, 1.12f, 1.08f, 6, 6 };   // RITE — 특수몹
    default: return { 2.0f, 3.0f, 1.0f,  1.0f,  2, 2 };
    }
}

inline ActRules GetActRules() {
    if (g_ActEndless) return EndlessRules();
    if (g_ActBossPick < 0) return WarmupRules();
    return RulesForPick(g_ActBossPick);
}

inline int GetActNumber() {
    if (g_ActEndless) return 6;
    if (g_ActBossPick < 0) return 0;
    return g_ActClears + 1;
}

inline const wchar_t* DisplayName(int pick) {
    switch (pick) {
    case 0: return L"HANG.exe";
    case 1: return L"UNKNOWN.sys";
    case 2: return L"VOLLEY.sys";
    case 3: return L"SPAM.dll";
    case 4: return L"GLITCH.exe";
    case 5: return L"KERNEL.sys";
    case 6: return L"FIREWALL.sys";
    case 7: return L"C2_RELAY.sys";
    case 8: return L"FORK.worm";
    case 9: return L"RITE.CORE";
    default: return L"UNKNOWN.sys";
    }
}

inline float HpMul(int pick) {
    switch (pick) {
    case 1: return 0.70f;
    case 2: return 1.08f;
    case 3: return 0.65f;
    case 5: return 0.45f;
    case 6: return 0.7f;
    case 7: return 0.75f;
    case 8: return 0.7f;
    case 9: return 0.72f;
    default: return 1.0f;
    }
}

inline glm::vec3 WarnColor(int pick) {
    switch (pick) {
    case 0: return { 0.65f, 0.68f, 0.74f };
    case 1: return { 0.95f, 0.2f,  0.6f  };
    case 2: return { 1.0f,  0.55f, 0.2f  };
    case 3: return { 1.0f,  0.4f,  0.8f  };
    case 4: return { 0.6f,  0.25f, 1.0f  };
    case 5: return { 1.0f,  0.65f, 0.25f };
    case 6: return { 1.0f,  0.45f, 0.2f  };
    case 7: return { 0.25f, 0.92f, 0.48f };
    case 8: return { 0.35f, 0.88f, 0.95f };
    case 9: return { 0.85f, 0.45f, 0.95f };
    default: return { 0.6f, 0.25f, 1.0f };
    }
}

inline const wchar_t* Tagline(int pick) {
    static const wchar_t* KR[10] = {
        L"응답 없음 — 멈추면 LAG 장판",
        L"게임창 끝까지 연사 — pull.lane으로 회수 경로 읽기",
        L"연발 포격 — 사거리 전조 표시",
        L"팝업 광고 폭주",
        L"실행 파일 형태 변조",
        L"커널 패닉 직전",
        L"인바운드 차단 — 아웃바운드 허용",
        L"C2 터미널 — 맵 전역 Agent 교전",
        L"프로세스 포크 — 연쇄 분열",
        L"기둥 의식 — Q/E/R 봉인 · 의식망",
    };
    static const wchar_t* EN[10] = {
        L"Not responding — freeze then lag",
        L"Edge pin — recall path cuts",
        L"Volley fire — telegraphed danger zones",
        L"Popup ad flood",
        L"Executable morphing",
        L"Kernel panic imminent",
        L"Inbound blocked — outbound open",
        L"C2 terminal — zombie host relay",
        L"Fork bomb — chained child processes",
        L"Pylon rite — skill bind · ritual web",
    };
    static const wchar_t* JP[10] = {
        L"応答なし — 停止後LAG",
        L"窓縁固定 — 回収経路が刃",
        L"連発砲撃 — 射程予告表示",
        L"ポップアップ広告暴走",
        L"実行ファイル変形",
        L"カーネルパニック直前",
        L"インバウンド遮断",
        L"C2端末 — ゾンビホスト中継",
        L"フォーク爆弾 — 子プロセス連鎖",
        L"柱の儀式 — スキル封印 · 儀式網",
    };
    if (pick < 0 || pick > 9) return L"";
    int li = LangIndex();
    if (li == 2) return JP[pick];
    if (li == 1) return EN[pick];
    return KR[pick];
}

// ACT별 스폰 bias — main 에서 kind 롤 시 참조
inline bool ActBiasSplitter()  { return g_ActBossPick == 8; }
inline bool ActBiasSpawner()   { return g_ActBossPick == 7 || g_ActBossPick == 8; }
inline bool ActBiasRanged()    { return g_ActBossPick == 2; }
inline bool ActBiasShielded()  { return g_ActBossPick == 9; }

inline const wchar_t* ActLabel() {
    static const wchar_t* W0[3] = { L"ACT 0 · BOOT", L"ACT 0 · BOOT", L"ACT 0 · 起動" };
    static const wchar_t* W6[3] = { L"ACT ∞ · ENDLESS", L"ACT ∞ · ENDLESS", L"ACT ∞ · 無限" };
    int li = LangIndex();
    if (g_ActEndless) return W6[li < 3 ? li : 0];
    if (g_ActBossPick < 0) return W0[li < 3 ? li : 0];
    return DisplayName(g_ActBossPick);
}

} // namespace BossDir
