#pragma once
#include <glm/glm.hpp>
#include "Settings.h"

// 보스 pick: 0행(HANG) 1(미사용) 2리로드 ...
namespace BossDir {

inline int& RotIdx() {
    static int idx = 0;
    return idx;
}

inline void ResetRotation() { RotIdx() = 0; }

inline int RollScorePick() {
    static const int kRot[] = { 0, 2, 3, 5, 6 };
    return kRot[RotIdx()++ % 5];
}

inline const wchar_t* DisplayName(int pick) {
    switch (pick) {
    case 0: return L"HANG.exe";
    case 1: return L"UNKNOWN.sys";
    case 2: return L"VOLLEY.sys";
    case 3: return L"SPAM.dll";
    case 4: return L"POLYMORPH.vir";
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
    case 1: return 0.7f;
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
        L"",
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
        L"",
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
        L"",
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

} // namespace BossDir
