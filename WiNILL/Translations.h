#pragma once
#include "Settings.h"

// ─────────────────────────────────────────────────────────────
// i18n — UI 라벨 번역 테이블
//   g_Language 에 따라 적절한 wchar_t* 반환
//   증강 이름/설명은 Augment.h 에서 별도 관리 (현재 한국어만, TODO)
// ─────────────────────────────────────────────────────────────

enum class StrId {
    PRESS_SPACE_TO_START,
    ESC_QUIT,
    PAUSED,
    RESUME_OR_QUIT,
    GAMEOVER,
    RESTART_ESC,
    CHOOSE_AUG,
    CHOOSE_DEBUFF,
    KEY_HINT_NO_HOVER,
    KEY_HINT_HOVER,
    OWNED_AUGS,
    SCORE,
    FPS,
    LV_PREFIX,
    REACHED_LEVEL,
    KILL_COUNT,
    FINAL_SCORE,

    // ── 메뉴 ──
    GAME_TITLE,
    BTN_START,
    BTN_SETTINGS,
    BTN_SHOP,
    BTN_QUIT,
    BTN_RESUME,
    BTN_RESTART,
    BTN_MAIN_MENU,
    BTN_BACK,
    // ── 난이도 ──
    DIFF_TITLE,
    DIFF_EASY,
    DIFF_NORMAL,
    DIFF_HARD,
    DIFF_EASY_DESC,
    DIFF_NORMAL_DESC,
    DIFF_HARD_DESC,
    // ── 설정 ──
    SET_TITLE,
    SET_FPS,
    SET_LANG,
    SET_FONT_SIZE,
    SET_DISPLAY,
    SET_UNLIMITED,
    SET_CROSSHAIR,
    SET_DMGNUM,
    SET_COMBO,
    OPT_ON,
    OPT_OFF,

    // ── 크리에이티브 모드 ──
    CREATIVE_ON,
    CREATIVE_OFF,
    CREATIVE_DESC_ON,
    CREATIVE_DESC_OFF,

    _COUNT
};

// 3 언어 × StrId 개수만큼의 wchar_t* 테이블
// 인덱스: [(int)StrId][(int)Language]
//   Language 순서: KR, EN, JP
inline const wchar_t* kStrings[(int)StrId::_COUNT][LANG_COUNT] = {
    /* PRESS_SPACE_TO_START */ {
        L"스페이스바로 시작",
        L"Press Space to Start",
        L"スペースキーでスタート",
    },
    /* ESC_QUIT */ {
        L"ESC  종료",
        L"ESC  Quit",
        L"ESC  終了",
    },
    /* PAUSED */ {
        L"일시 정지",
        L"Paused",
        L"一時停止",
    },
    /* RESUME_OR_QUIT */ {
        L"스페이스바  재개    /    ESC  종료",
        L"Space  Resume    /    ESC  Quit",
        L"スペース  再開    /    ESC  終了",
    },
    /* GAMEOVER */ {
        L"게임 오버",
        L"Game Over",
        L"ゲームオーバー",
    },
    /* RESTART_ESC */ {
        L"ESC  재시작",
        L"ESC  Restart",
        L"ESC  リスタート",
    },
    /* CHOOSE_AUG */ {
        L"증강을 선택하세요",
        L"Choose an Augment",
        L"強化を選択してください",
    },
    /* CHOOSE_DEBUFF */ {
        L"디버프를 선택하세요  (피할 수 없음)",
        L"Choose a Debuff  (unavoidable)",
        L"デバフを選択してください  (回避不可)",
    },
    /* KEY_HINT_NO_HOVER */ {
        L"1  /  2  /  3  키로 카드 선택  ·  Space 확정",
        L"1  /  2  /  3  to highlight  ·  Space to confirm",
        L"1  /  2  /  3  でカード選択  ·  Space で確定",
    },
    /* KEY_HINT_HOVER */ {
        L"Space 로 확정",
        L"Press Space to confirm",
        L"Space で確定",
    },
    /* OWNED_AUGS */ {
        L"보유 증강",
        L"Owned Augments",
        L"所持強化",
    },
    /* SCORE */ {
        L"Score",
        L"Score",
        L"スコア",
    },
    /* FPS */ {
        L"FPS",
        L"FPS",
        L"FPS",
    },
    /* LV_PREFIX */ {
        L"Lv.",
        L"Lv.",
        L"Lv.",
    },
    /* REACHED_LEVEL */ {
        L"도달 레벨",
        L"Reached Level",
        L"到達レベル",
    },
    /* KILL_COUNT */ {
        L"처치 수",
        L"Kills",
        L"撃破数",
    },
    /* FINAL_SCORE */ {
        L"최종 점수",
        L"Final Score",
        L"最終スコア",
    },
    /* GAME_TITLE */ {
        L"Onedow", L"Onedow", L"Onedow",
    },
    /* BTN_START */ {
        L"시작", L"Start", L"スタート",
    },
    /* BTN_SETTINGS */ {
        L"설정", L"Settings", L"設定",
    },
    /* BTN_SHOP */ {
        L"상점", L"Shop", L"ショップ",
    },
    /* BTN_QUIT */ {
        L"종료", L"Quit", L"終了",
    },
    /* BTN_RESUME */ {
        L"재개", L"Resume", L"再開",
    },
    /* BTN_RESTART */ {
        L"다시하기", L"Restart", L"リスタート",
    },
    /* BTN_MAIN_MENU */ {
        L"메뉴로", L"Main Menu", L"メニューへ",
    },
    /* BTN_BACK */ {
        L"뒤로", L"Back", L"戻る",
    },
    /* DIFF_TITLE */ {
        L"난이도 선택", L"Choose Difficulty", L"難易度を選択",
    },
    /* DIFF_EASY */ {
        L"쉬움", L"Easy", L"イージー",
    },
    /* DIFF_NORMAL */ {
        L"보통", L"Normal", L"ノーマル",
    },
    /* DIFF_HARD */ {
        L"어려움", L"Hard", L"ハード",
    },
    /* DIFF_EASY_DESC */ {
        L"원거리 몹 5초 지연  /  최대 2 마리",
        L"Ranged delay 5s  /  Max 2",
        L"遠距離敵 5秒遅延  /  最大 2体",
    },
    /* DIFF_NORMAL_DESC */ {
        L"기본 설정",
        L"Default",
        L"標準設定",
    },
    /* DIFF_HARD_DESC */ {
        L"즉시 등장  /  최대 8 마리  /  스폰 2배",
        L"Immediate  /  Max 8  /  2x spawn",
        L"即出現  /  最大 8体  /  スポーン2倍",
    },
    /* SET_TITLE */ {
        L"설정", L"Settings", L"設定",
    },
    /* SET_FPS */ {
        L"FPS 제한", L"FPS Cap", L"FPS 制限",
    },
    /* SET_LANG */ {
        L"언어", L"Language", L"言語",
    },
    /* SET_FONT_SIZE */ {
        L"폰트 크기", L"Font Size", L"フォントサイズ",
    },
    /* SET_DISPLAY */ {
        L"디스플레이", L"Display", L"ディスプレイ",
    },
    /* SET_UNLIMITED */ {
        L"무제한", L"Unlimited", L"無制限",
    },
    /* SET_CROSSHAIR */ {
        L"크로스헤어", L"Crosshair", L"照準",
    },
    /* SET_DMGNUM */ {
        L"데미지 숫자(딜 계산)", L"Damage Numbers", L"ダメージ表示",
    },
    /* SET_COMBO */ {
        L"콤보 표시", L"Combo Counter", L"コンボ表示",
    },
    /* OPT_ON */ {
        L"켜기", L"ON", L"オン",
    },
    /* OPT_OFF */ {
        L"끄기", L"OFF", L"オフ",
    },
    /* CREATIVE_ON */ {
        L"★ 크리에이티브  [ON]", L"★ Creative  [ON]", L"★ クリエイティブ  [ON]",
    },
    /* CREATIVE_OFF */ {
        L"☆ 크리에이티브  [OFF]", L"☆ Creative  [OFF]", L"☆ クリエイティブ  [OFF]",
    },
    /* CREATIVE_DESC_ON */ {
        L"시작 시 10만점 · F키로 증강 획득 · 디버프 없음",
        L"Start at 100k · F to grab augments · no debuffs",
        L"開始時10万点 · Fキーで強化獲得 · デバフ無し",
    },
    /* CREATIVE_DESC_OFF */ {
        L"10만점부터 시작 (보스 즉시) + F키 증강",
        L"Start at 100k (boss now) + F-key augments",
        L"10万点開始 (ボス即時) + Fキー強化",
    },
};

inline const wchar_t* T(StrId id) {
    int li = (int)g_Language;
    if (li < 0 || li >= LANG_COUNT) li = 0;
    return kStrings[(int)id][li];
}
