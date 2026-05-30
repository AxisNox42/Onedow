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
    OPT_ON,
    OPT_OFF,

    _COUNT
};

// 7 언어 × StrId 개수만큼의 wchar_t* 테이블
// 인덱스: [(int)StrId][(int)Language]
//   Language 순서: KR, EN, ZH, RU, ES, PT, DE
inline const wchar_t* kStrings[(int)StrId::_COUNT][7] = {
    /* PRESS_SPACE_TO_START */ {
        L"스페이스바로 시작",
        L"Press Space to Start",
        L"按空格键开始",
        L"Нажмите Пробел чтобы начать",
        L"Pulsa Espacio para empezar",
        L"Pressione Espaço para começar",
        L"Drücke Leertaste zum Starten",
    },
    /* ESC_QUIT */ {
        L"ESC  종료",
        L"ESC  Quit",
        L"ESC  退出",
        L"ESC  Выход",
        L"ESC  Salir",
        L"ESC  Sair",
        L"ESC  Beenden",
    },
    /* PAUSED */ {
        L"일시 정지",
        L"Paused",
        L"已暂停",
        L"Пауза",
        L"En pausa",
        L"Pausado",
        L"Pausiert",
    },
    /* RESUME_OR_QUIT */ {
        L"스페이스바  재개    /    ESC  종료",
        L"Space  Resume    /    ESC  Quit",
        L"空格  继续    /    ESC  退出",
        L"Пробел  Продолжить    /    ESC  Выход",
        L"Espacio  Reanudar    /    ESC  Salir",
        L"Espaço  Continuar    /    ESC  Sair",
        L"Leertaste  Fortsetzen    /    ESC  Beenden",
    },
    /* GAMEOVER */ {
        L"게임 오버",
        L"Game Over",
        L"游戏结束",
        L"Игра окончена",
        L"Fin del juego",
        L"Fim de jogo",
        L"Spielende",
    },
    /* RESTART_ESC */ {
        L"ESC  재시작",
        L"ESC  Restart",
        L"ESC  重新开始",
        L"ESC  Перезапуск",
        L"ESC  Reiniciar",
        L"ESC  Reiniciar",
        L"ESC  Neustart",
    },
    /* CHOOSE_AUG */ {
        L"증강을 선택하세요",
        L"Choose an Augment",
        L"选择一个增益",
        L"Выберите усиление",
        L"Elige una mejora",
        L"Escolha um aumento",
        L"Wähle eine Verstärkung",
    },
    /* CHOOSE_DEBUFF */ {
        L"디버프를 선택하세요  (피할 수 없음)",
        L"Choose a Debuff  (unavoidable)",
        L"选择一个负面效果  (无法避免)",
        L"Выберите дебафф  (неизбежно)",
        L"Elige una desventaja  (inevitable)",
        L"Escolha uma desvantagem  (inevitável)",
        L"Wähle einen Debuff  (unvermeidlich)",
    },
    /* KEY_HINT_NO_HOVER */ {
        L"1  /  2  /  3  키로 카드 선택  ·  Space 확정",
        L"1  /  2  /  3  to highlight  ·  Space to confirm",
        L"1  /  2  /  3  选择卡片  ·  Space 确认",
        L"1  /  2  /  3  выбор карты  ·  Space подтвердить",
        L"1  /  2  /  3  resaltar  ·  Space confirmar",
        L"1  /  2  /  3  destacar  ·  Space confirmar",
        L"1  /  2  /  3  auswählen  ·  Space bestätigen",
    },
    /* KEY_HINT_HOVER */ {
        L"Space 로 확정",
        L"Press Space to confirm",
        L"按 Space 确认",
        L"Нажмите Space",
        L"Pulsa Space para confirmar",
        L"Pressione Space",
        L"Space zum Bestätigen",
    },
    /* OWNED_AUGS */ {
        L"보유 증강",
        L"Owned Augments",
        L"已有增益",
        L"Усиления",
        L"Mejoras",
        L"Aumentos",
        L"Verstärkungen",
    },
    /* SCORE */ {
        L"Score",
        L"Score",
        L"分数",
        L"Счёт",
        L"Puntos",
        L"Pontos",
        L"Punkte",
    },
    /* FPS */ {
        L"FPS",
        L"FPS",
        L"FPS",
        L"FPS",
        L"FPS",
        L"FPS",
        L"FPS",
    },
    /* LV_PREFIX */ {
        L"Lv.",
        L"Lv.",
        L"等级",
        L"Ур.",
        L"Nv.",
        L"Nv.",
        L"Stufe",
    },
    /* REACHED_LEVEL */ {
        L"도달 레벨",
        L"Reached Level",
        L"达到等级",
        L"Уровень",
        L"Nivel alcanzado",
        L"Nível alcançado",
        L"Erreichte Stufe",
    },
    /* KILL_COUNT */ {
        L"처치 수",
        L"Kills",
        L"击杀数",
        L"Убийства",
        L"Bajas",
        L"Abates",
        L"Abschüsse",
    },
    /* FINAL_SCORE */ {
        L"최종 점수",
        L"Final Score",
        L"最终分数",
        L"Итог",
        L"Puntuación final",
        L"Pontuação final",
        L"Endpunktzahl",
    },
    /* GAME_TITLE */ {
        L"WiNILL", L"WiNILL", L"WiNILL", L"WiNILL", L"WiNILL", L"WiNILL", L"WiNILL",
    },
    /* BTN_START */ {
        L"시작",  L"Start", L"开始", L"Старт", L"Empezar", L"Iniciar", L"Start",
    },
    /* BTN_SETTINGS */ {
        L"설정",  L"Settings", L"设置", L"Настройки",
        L"Ajustes", L"Ajustes", L"Einstellungen",
    },
    /* BTN_QUIT */ {
        L"종료",  L"Quit", L"退出", L"Выход", L"Salir", L"Sair", L"Beenden",
    },
    /* BTN_RESUME */ {
        L"재개",  L"Resume", L"继续", L"Продолжить",
        L"Reanudar", L"Continuar", L"Fortsetzen",
    },
    /* BTN_RESTART */ {
        L"다시하기", L"Restart", L"重新开始", L"Перезапуск",
        L"Reiniciar", L"Reiniciar", L"Neustart",
    },
    /* BTN_MAIN_MENU */ {
        L"메뉴로", L"Main Menu", L"主菜单", L"В меню",
        L"Menú",   L"Menu",      L"Hauptmenü",
    },
    /* BTN_BACK */ {
        L"뒤로",   L"Back", L"返回", L"Назад",
        L"Atrás", L"Voltar", L"Zurück",
    },
    /* DIFF_TITLE */ {
        L"난이도 선택", L"Choose Difficulty", L"选择难度", L"Сложность",
        L"Dificultad", L"Dificuldade", L"Schwierigkeit",
    },
    /* DIFF_EASY */ {
        L"쉬움", L"Easy", L"简单", L"Лёгкий",
        L"Fácil", L"Fácil", L"Leicht",
    },
    /* DIFF_NORMAL */ {
        L"보통", L"Normal", L"普通", L"Обычный",
        L"Normal", L"Normal", L"Normal",
    },
    /* DIFF_HARD */ {
        L"어려움", L"Hard", L"困难", L"Сложный",
        L"Difícil", L"Difícil", L"Schwer",
    },
    /* DIFF_EASY_DESC */ {
        L"원거리 몹 5초 지연  /  최대 2 마리",
        L"Ranged delay 5s  /  Max 2",
        L"远程怪延迟5秒  /  最大 2",
        L"Дальние +5с задержка  /  макс. 2",
        L"Retraso 5s  /  máx. 2",
        L"Atraso 5s  /  máx. 2",
        L"5s Verzögerung  /  Max 2",
    },
    /* DIFF_NORMAL_DESC */ {
        L"기본 설정",
        L"Default",
        L"默认",
        L"По умолчанию",
        L"Predeterminado",
        L"Padrão",
        L"Standard",
    },
    /* DIFF_HARD_DESC */ {
        L"즉시 등장  /  최대 8 마리  /  스폰 2배",
        L"Immediate  /  Max 8  /  2x spawn",
        L"立刻出现  /  最大 8  /  2倍刷新",
        L"Сразу  /  макс. 8  /  ×2 спавн",
        L"Inmediato  /  máx. 8  /  ×2",
        L"Imediato  /  máx. 8  /  ×2",
        L"Sofort  /  Max 8  /  ×2",
    },
    /* SET_TITLE */ {
        L"설정", L"Settings", L"设置", L"Настройки",
        L"Ajustes", L"Ajustes", L"Einstellungen",
    },
    /* SET_FPS */ {
        L"FPS 제한", L"FPS Cap", L"帧率上限", L"Лимит FPS",
        L"Límite FPS", L"Limite FPS", L"FPS-Limit",
    },
    /* SET_LANG */ {
        L"언어", L"Language", L"语言", L"Язык",
        L"Idioma", L"Idioma", L"Sprache",
    },
    /* SET_FONT_SIZE */ {
        L"폰트 크기", L"Font Size", L"字体大小", L"Размер шрифта",
        L"Tamaño de fuente", L"Tamanho da fonte", L"Schriftgröße",
    },
    /* SET_DISPLAY */ {
        L"디스플레이", L"Display", L"显示", L"Дисплей",
        L"Pantalla", L"Tela", L"Anzeige",
    },
    /* SET_UNLIMITED */ {
        L"무제한", L"Unlimited", L"无限制", L"Без лимита",
        L"Sin límite", L"Sem limite", L"Unbegrenzt",
    },
    /* SET_CROSSHAIR */ {
        L"크로스헤어", L"Crosshair", L"准星", L"Прицел",
        L"Mira", L"Mira", L"Fadenkreuz",
    },
    /* OPT_ON */ {
        L"켜기", L"ON", L"开启", L"Вкл",
        L"Sí", L"Sim", L"Ein",
    },
    /* OPT_OFF */ {
        L"끄기", L"OFF", L"关闭", L"Выкл",
        L"No", L"Não", L"Aus",
    },
};

inline const wchar_t* T(StrId id) {
    int li = (int)g_Language;
    if (li < 0 || li >= 7) li = 0;
    return kStrings[(int)id][li];
}
