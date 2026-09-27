#pragma once
#include "Settings.h"

// 튜토리얼 — '/' 로 줄바꿈 (증강 설명과 동일 파서)
inline constexpr int TUTORIAL_PAGE_COUNT = 8;

inline int LangIndexTutorial() {
    int li = (int)g_Language;
    if (li < 0 || li >= LANG_COUNT) li = 0;
    return li;
}

// ── READY 화면 (짧은 요약) ──
inline const wchar_t* kReadyTitle[LANG_COUNT] = {
    L"작전 개시",
    L"Mission Start",
    L"作戦開始",
};
inline const wchar_t* kReadyBrief[LANG_COUNT] = {
    L"별자리 영역 안에서 침입 신호를 막아내세요 / WASD·마우스·SHIFT·QER로 싸우고, 레벨업마다 증강을 고릅니다",
    L"Stop rogue signals inside the starfield / WASD·mouse·SHIFT·QER — pick augments each level-up",
    L"星座領域内で侵入信号を阻止 / WASD·マウス·SHIFT·QER — レベルアップで強化選択",
};
inline const wchar_t* kReadyHint[LANG_COUNT] = {
    L"자세한 설명 → 메인 메뉴 「튜토리얼」",
    L"Full guide → Main menu 「Tutorial」",
    L"詳細 → メインメニュー「チュートリアル」",
};

inline const wchar_t* ReadyTitle()  { return kReadyTitle[LangIndexTutorial()]; }
inline const wchar_t* ReadyBrief()  { return kReadyBrief[LangIndexTutorial()]; }
inline const wchar_t* ReadyHint()   { return kReadyHint[LangIndexTutorial()]; }

// ── 튜토리얼 창 (페이지별) ──
inline const wchar_t* kTutorialWinTitle[LANG_COUNT] = {
    L"ASTRAL FIELD GUIDE",
    L"ASTRAL FIELD GUIDE",
    L"ASTRAL FIELD GUIDE",
};

inline const wchar_t* kTutorialPageTitle[LANG_COUNT][TUTORIAL_PAGE_COUNT] = {
    /* KR */
    { L"1. 세계관", L"2. 조작", L"3. 증강", L"4. 디버프", L"5. 런 준비", L"6. 보스", L"7. 런 경제", L"8. 메타·팁" },
    /* EN */
    { L"1. World", L"2. Controls", L"3. Augments", L"4. Debuffs", L"5. Run setup", L"6. Bosses", L"7. Run economy", L"8. Meta & tips" },
    /* JP */
    { L"1. 世界観", L"2. 操作", L"3. 強化", L"4. デバフ", L"5. ラン準備", L"6. ボス", L"7. ラン経済", L"8. メタ·コツ" },
};

inline const wchar_t* kTutorialPageBody[LANG_COUNT][TUTORIAL_PAGE_COUNT] = {
    /* KR */
    {
        L"Onedow는 별빛 위에 펼쳐지는 투명한 천문 관측 게임입니다 / "
        L"별빛과 성운이 비치고, 게임은 항해 가능한 별자리 영역 안에서 펼쳐집니다 / "
        L"당신은 항성 함선 — 관측 영역을 이동하며 침입한 신호(적)를 막아냅니다 / "
        L"영역 밖은 시야 밖입니다. 보이는 별자리 영역 안에서만 전투·피격·이펙트가 일어납니다 / "
        L"목표: 웨이브를 버티고 보스를 격파한 뒤, 최대한 오래 생존하며 점수·코인을 모으세요",

        L"WASD — 항성 함선 이동 / 마우스 — 조준 방향, 좌클릭 유지 시 연사 / "
        L"SHIFT — 대시(짧은 무적·쿨다운). 위험한 탄막을 뚫을 때 사용 / "
        L"Q · E · R — 액티브 스킬 슬롯. 증강으로 해금되며 최대 3개까지 장착 / "
        L"ESC — 일시정지(재개·설정·메뉴) / Space — READY 화면에서 전투 시작, 일시정지 중 재개 / "
        L"마우스를 카드·아이콘 위에 올리면 상세 설명이 표시됩니다",

        L"적 처치 → 경험치(EXP) → 레벨업 시 증강 카드 3장 중 1장 선택 / "
        L"숫자키 1·2·3으로 후보를 고르고 Space로 확정합니다 / "
        L"증강은 공격·방어·이동·스킬·경제 등 빌드를 결정합니다. 중복·시너지를 고려하세요 / "
        L"일부 증강은 Q/E/R 액티브를 해금하거나 기존 스킬을 강화합니다 / "
        L"도감에서 이미 본 증강은 이름·효과를 미리 확인할 수 있습니다(미발견은 ???)",

        L"레벨업마다 증강과 함께 디버프도 반드시 1개 받습니다 — 피할 수 없습니다 / "
        L"대신 디버프 구간에서는 EXP·코인 보상이 올라가 리스크=리턴 구조입니다 / "
        L"디버프는 이동·사격·체력·쿨다운 등에 불리한 효과를 줍니다 / "
        L"증강으로 상쇄하거나, 패턴에 맞춰 플레이 스타일을 바꿔 대응하세요 / "
        L"모든 런은 NORMAL 수치로 시작하며, 시련이 추가 도전을 만듭니다",

        L"모든 런은 NORMAL로 시작합니다 / "
        L"이어서 랜덤 3종 중 시작 무기를 선택합니다 / "
        L"무기마다 사거리, 연사, 특수 탄환 등 스타일이 크게 달라집니다 / "
        L"선택 화면 설명을 읽고 플레이에 맞는 무기를 고르세요 / "
        L"크리에이티브 모드에서는 시작 조건과 시작 증강을 직접 설정할 수 있습니다",

        L"보스 VOLLEY, FORK가 각자의 성운 영역에 등장합니다 / "
        L"보스 영역 안에서만 패턴이 펼쳐지며, 체력바를 깎아 격파합니다 / "
        L"탄막, 소환, 장판 패턴을 읽고 SHIFT 대시와 필드 위치로 회피하세요 / "
        L"보스 처치 후 짧은 휴식(인터미션)이 열리고 오른쪽 구역에서 건너뛸 수 있습니다 / "
        L"보스마다 공략이 다릅니다 - 도감에서 정보를 해금하세요",

        L"런 중 획득한 스타더스트는 HUD에 표시되고 런 종료 시 정산됩니다 / "
        L"레벨업 증강 선택으로 빌드를 보완합니다 / "
        L"런이 끝나면 일부 성과가 코인으로 정산되어 메타 상점에 쓰입니다 / "
        L"메인 메뉴 상점 — 코인으로 영구 스탯·시작 증강 슬롯·테마 등을 강화 / "
        L"업적 달성 시 코인·해금 보상이 주어집니다",

        L"재료 증강 3종을 모으고 Lv10 이상이면 「조합 증강」이 등장합니다 / "
        L"조합은 강력하지만 재료를 차지하므로 계획적으로 모으세요 / "
        L"메인 메뉴 도감 — 적·증강·보스 정보 검색(위키 스타일) / "
        L"설정에서 언어·볼륨·관측 필드 동작을 바꿀 수 있습니다 / "
        L"패배 후에도 코인·도감·업적 진행은 유지됩니다. 반복 플레이로 빌드를 완성하세요",
    },
    /* EN */
    {
        L"Onedow is a transparent observatory spread across the starfield / "
        L"The cosmos shows through; combat happens inside navigable constellation zones / "
        L"You are the astral vessel — move through the field and stop invading signals (enemies) / "
        L"Outside the visible constellation zones is out of play: no combat, hits, or effects there / "
        L"Goal: survive waves, defeat bosses, and farm score·coins as long as you can",

        L"WASD — move your astral vessel / Mouse — aim; hold LMB to fire / "
        L"SHIFT — dash with brief i-frames and cooldown / "
        L"Q · E · R — active skills from augments (max 3 slots) / "
        L"ESC — pause menu / Space — start from READY, resume from pause / "
        L"Hover cards and icons for full descriptions",

        L"Kills → XP → on level-up pick 1 of 3 augment cards / "
        L"Keys 1·2·3 select; Space confirms / "
        L"Augments define your build: damage, defense, mobility, skills, economy / "
        L"Some unlock or upgrade Q/E/R actives / "
        L"Codex shows discovered augments in detail; unknown ones stay hidden",

        L"Every level-up also forces one debuff — cannot skip / "
        L"Debuff periods grant more XP and coins (risk vs reward) / "
        L"Debuffs hinder move speed, fire rate, HP, cooldowns, etc. / "
        L"Counter with augments or adapt your playstyle / "
        L"Every run uses NORMAL values; trials provide the extra challenge",

        L"Every run starts at NORMAL / "
        L"Then choose 1 of 3 random starting weapons / "
        L"Range, fire rate, and specials differ a lot per weapon / "
        L"Read the select-screen text and pick what fits your style / "
        L"Creative mode lets you set start conditions and starting augments",

        L"Bosses VOLLEY and FORK spawn in their own constellation zones / "
        L"Patterns play inside the boss field; burn the HP bar to win / "
        L"Dodge with SHIFT and field positioning / "
        L"After each boss: a short intermission; the right zone skips it / "
        L"Each boss plays differently - unlock tips in the Codex",

        L"Stardust earned mid-run is shown on the HUD and settled at run end / "
        L"Use level-up augment choices to patch your build / "
        L"End of run converts some progress into meta coins / "
        L"Main menu shop — permanent stats, extra start augments, themes / "
        L"Achievements grant coins and unlocks",

        L"Collect 3 combo ingredient augments + reach Lv10 → combo augment appears / "
        L"Combos are strong but cost ingredient slots — plan ahead / "
        L"Main menu Codex — searchable wiki for enemies, augments, bosses / "
        L"Settings: language, volume, observatory behavior / "
        L"Death keeps coins, codex, and achievements — iterate your build",
    },
    /* JP */
    {
        L"Onedowは星空に広がる透明な天文観測ゲームです / "
        L"星雲が透けて見え、戦闘は航行可能な星座領域で行われます / "
        L"あなたは星の船 — フィールドを移動し侵入信号(敵)を阻止します / "
        L"星座領域の外は戦闘範囲外 — 攻撃·被弾·演出は領域内のみ / "
        L"目標: ウェーブを凌ぎボスを倒し、できるだけ長くスコア·コインを稼ぐ",

        L"WASD — 星の船(プレイヤー)移動 / マウス — 照準、左クリック長押しで連射 / "
        L"SHIFT — 短い無敵付きダッシュ(クールダウンあり) / "
        L"Q · E · R — 強化で解放するアクティブスキル(最大3) / "
        L"ESC — 一時停止 / Space — READYから開始、停止中は再開 / "
        L"カード·アイコンにホバーで詳細表示",

        L"撃破→経験値→レベルアップで3枚から1枚選択 / "
        L"1·2·3で選択、Spaceで確定 / "
        L"強化は攻撃·防御·移動·スキル·経済などビルドを決めます / "
        L"一部はQ/E/Rアクティブの解放·強化になります / "
        L"図鑑で発見済み強化の説明を確認(未発見は???)",

        L"レベルアップごとにデバフも必ず1つ — 回避不可 / "
        L"その代わりEXP·コイン報酬が増えるリスク·リターン設計 / "
        L"デバフは移動·射撃·HP·クールダウンなどに不利 / "
        L"強化で相殺するかプレイを変えて対応 / "
        L"難易度が上がるほどデバフと敵密度が厳しくなります",

        L"ラン前: 難易度を選択 / "
        L"続けてランダム3択から開始武器を選ぶ / "
        L"武器ごとに射程·連射·特殊弾などスタイルが大きく異なる / "
        L"選択画面の説明を読み、自分に合う武器を選ぶ / "
        L"クリエイティブモードで開始条件·開始強化を設定可能",

        L"VOLLEY、FORK のボスが星座領域に出現 / "
        L"領域内でパターンが展開、HPバーを削って撃破 / "
        L"SHIFTダッシュとフィールド位置で回避 / "
        L"撃破後は短い休憩とランゴールドショップ / "
        L"ボスごとに攻略が異なる - 図鑑で情報解禁",

        L"ラン中のゴールドはウェーブ間の「ランショップ」で消費 / "
        L"強化を直接購入してビルドを補強 / "
        L"ラン終了後、一部がメタコインに換算 / "
        L"メニューショップ — 永久ステ·開始強化枠·テーマなど / "
        L"実績でコインと解禁報酬",

        L"素材強化3種+Lv10以上で「組合強化」出現 / "
        L"強力だが素材枠を使う — 計画的に収集 / "
        L"メニュー図鑑 — 敵·強化·ボスを検索(ウィキ風) / "
        L"設定で言語·音量·窓動作を変更 / "
        L"敗北後もコイン·図鑑·実績は保持 — 繰り返しビルドを完成させよう",
    },
};

inline const wchar_t* kTutorialControls[LANG_COUNT] = {
    L"WASD 이동  |  마우스 사격  |  SHIFT 대시  |  Q / E / R 스킬  |  ESC 일시정지",
    L"WASD  |  Mouse fire  |  SHIFT dash  |  Q / E / R skills  |  ESC pause",
    L"WASD  |  マウス射撃  |  SHIFTダッシュ  |  Q/E/Rスキル  |  ESC停止",
};

inline const wchar_t* TutorialWinTitle() { return kTutorialWinTitle[LangIndexTutorial()]; }
inline const wchar_t* TutorialPageTitle(int page) {
    if (page < 0 || page >= TUTORIAL_PAGE_COUNT) return L"";
    return kTutorialPageTitle[LangIndexTutorial()][page];
}
inline const wchar_t* TutorialPageBody(int page) {
    if (page < 0 || page >= TUTORIAL_PAGE_COUNT) return L"";
    return kTutorialPageBody[LangIndexTutorial()][page];
}
inline const wchar_t* TutorialControls() { return kTutorialControls[LangIndexTutorial()]; }

// 하위 호환 (READY 등에서 미사용)
inline constexpr int TUTORIAL_BLOCK_COUNT = TUTORIAL_PAGE_COUNT;
inline const wchar_t* TutorialTitle() { return kTutorialPageTitle[LangIndexTutorial()][0]; }
inline const wchar_t* TutorialBlock(int i) { return TutorialPageBody(i); }
