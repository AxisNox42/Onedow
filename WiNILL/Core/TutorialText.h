#pragma once
#include "Settings.h"

// 시작 전(READY) 화면 튜토리얼 — '/' 로 줄바꿈 (증강 설명과 동일 파서)
inline constexpr int TUTORIAL_BLOCK_COUNT = 6;

inline const wchar_t* kTutorialTitle[LANG_COUNT] = {
    L"플레이 가이드",
    L"How to Play",
    L"プレイガイド",
};

inline const wchar_t* kTutorialBlocks[LANG_COUNT][TUTORIAL_BLOCK_COUNT] = {
    /* KR */
    {
        L"가짜 OS 창이 전장입니다 / onedow.exe(당신) 창을 움직이며 프로세스를 막아내세요 / 창 밖은 시야 밖 — 보이는 창 안에서만 싸웁니다",
        L"WASD 이동 · 마우스 조준·사격 · SHIFT 대시(짧은 무적) / Q·E·R 액티브 스킬 — 증강으로 해금, 최대 3개",
        L"처치 → 경험치 → 레벨업 시 증강 3택1 (1·2·3 키 → Space 확정) / 디버프도 반드시 1개 — 대신 EXP·코인 보상↑",
        L"보스는 별도 .sys·.exe 창으로 등장 / 창 안 패턴을 읽고 체력바를 깎아 격파하세요",
        L"런 중 휴식마다 골드 상점(증강 구매) / 메뉴 상점·업적로 코인 영구 강화 · 도감에서 적·증강 해금",
        L"재료 3개 + Lv10+ → 조합 증강 등장 / ESC 일시정지 · 마우스 호버로 카드 상세 설명 확인",
    },
    /* EN */
    {
        L"Fake OS windows are the battlefield / Move onedow.exe and stop rogue processes / Fight only inside visible windows",
        L"WASD move · mouse aim·fire · SHIFT dash (brief i-frames) / Q·E·R actives from augments (max 3)",
        L"Kills → XP → level-up pick 1 of 3 (keys 1·2·3, Space confirm) / Debuffs are forced but grant more XP·coins",
        L"Bosses spawn in their own windows / Read patterns and burn their HP bars",
        L"Gold shop between waves / Meta shop & achievements for permanent upgrades · Codex unlocks info",
        L"3 combo ingredients + Lv10+ → combo augments / ESC pause · hover cards for full details",
    },
    /* JP */
    {
        L"偽OSウィンドウが戦場 / onedow.exeを動かしプロセスを阻止 / 見える窓の中だけが戦闘範囲",
        L"WASD移動 · マウス照準·射撃 · SHIFTダッシュ(短い無敵) / Q·E·Rアクティブは強化で解放(最大3)",
        L"撃破→経験値→レベルアップで3択1 (1·2·3→Space確定) / デバフは必須だがEXP·コイン増",
        L"ボスは別.sys·.exe窓で出現 / パターンを読んでHPを削る",
        L"ウェーブ間のゴールドショップ / メニューでコイン永久強化 · 図鑑で敵·強化解禁",
        L"素材3つ+Lv10+で組合強化 / ESC一時停止 · ホバーで詳細表示",
    },
};

inline const wchar_t* kTutorialControls[LANG_COUNT] = {
    L"WASD 이동  ·  마우스 사격  ·  SHIFT 대시  ·  Q / E / R 스킬  ·  ESC 일시정지",
    L"WASD  ·  Mouse fire  ·  SHIFT dash  ·  Q / E / R skills  ·  ESC pause",
    L"WASD  ·  マウス射撃  ·  SHIFTダッシュ  ·  Q/E/Rスキル  ·  ESC停止",
};

inline int LangIndexTutorial() {
    int li = (int)g_Language;
    if (li < 0 || li >= LANG_COUNT) li = 0;
    return li;
}

inline const wchar_t* TutorialTitle() { return kTutorialTitle[LangIndexTutorial()]; }
inline const wchar_t* TutorialBlock(int i) {
    if (i < 0 || i >= TUTORIAL_BLOCK_COUNT) return L"";
    return kTutorialBlocks[LangIndexTutorial()][i];
}
inline const wchar_t* TutorialControls() { return kTutorialControls[LangIndexTutorial()]; }
