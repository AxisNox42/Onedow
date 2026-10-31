#pragma once

// ── 게임 설정 (런타임 변경 가능, 추후 설정 메뉴) ──

// FPS 캡:
//   0  = VSync (모니터 주사율)
//   -1 = 무제한
//   양수 = 해당 FPS 로 캡
inline int g_FpsCap = 60;   // 첫 실행 기본 60fps (세이브 있으면 덮어씀)

// 언어 (한국어 / 영어 / 일본어)
enum class Language { KR, EN, JP };
inline Language g_Language = Language::KR;

// ASTERIA_SMOKE 환경변수로 실행한 스모크 캡처 세션. 세이브를 쓰지 않는다.
inline bool g_SmokeCapture = false;
// Smoke-only: augment slot to keyboard-focus once the selector accepts input
// (the selector resets focus on entry, so AUGSEL queues it here).
inline int g_SmokeAugFocus = -1;
// Smoke-only game-time scale (SLOW=x step), for frame-by-frame captures.
inline float g_SmokeTimeScale = 1.0f;
inline constexpr int LANG_COUNT = 3;

inline int LangIndex() {
    int li = (int)g_Language;
    if (li < 0 || li >= LANG_COUNT) li = 0;
    return li;
}

// 크로스헤어 표시 (기본 ON)
inline bool g_ShowCrosshair = true;

// 하단 작업표시줄 높이(px) — 풀스크린 오버레이 위에 작업표시줄이 떠 있을 때
//   체력바 등 하단 UI 가 가려지지 않도록 이만큼 위로 올린다. main 이 시작 시 계산.
inline int g_TaskbarH = 0;

// 화면 스케일 — 창/엔티티 크기를 해상도에 비례하게. 기준 높이(SCALE_REF_H) 대비.
//   작은 화면일수록 g_Scale<1 → 플레이어 창·원거리 몹 창 등이 비례 축소됨.
//   main 이 시작 시 화면 높이로 계산. (1.0 = 기준, 상한 1.0 / 하한 0.5)
inline float g_Scale = 1.0f;
inline constexpr float SCALE_REF_H = 2000.0f;   // 이 높이에서 g_Scale=1.0 (값↑일수록 전체적으로 작아짐)

// 인게임 작업표시줄 여백(px). Windows에서는 활성 창을 Shell fullscreen으로
// 표시해 작업표시줄을 숨기므로 0을 유지한다. 하단 HUD는 전체 화면을 사용한다.
inline constexpr float g_GameBarH = 0.0f;

// 플레이 영역 확장량(px, 각 변) — 폴리모프 페이즈2 줌아웃 시 보이는 영역이 넓어지므로
//   원거리 몹 등이 확장된 구역까지 돌아다니도록 main 이 매 프레임 설정. 0 = 확장 없음.
inline float g_ArenaExX = 0.0f;
inline float g_ArenaExY = 0.0f;

inline const char* LanguageFace(Language /*lang*/) {
    return "Microsoft YaHei UI";
}

// 비-Windows(macOS/Linux): TTF 폴백 체인 — Resource/Font/ 두 파일
inline int LanguageFontChain(Language /*lang*/, const char* out[2]) {
    out[0] = "Resource/Font/Jua-Regular.ttf";
    out[1] = "Resource/Font/KosugiMaru-Regular.ttf";
    return 2;
}

// PLAY chooses the difficulty; it also remains in saved run records.
enum class Difficulty { EASY, NORMAL, HARD, NIGHTMARE };
inline constexpr int DIFFICULTY_COUNT = 4;
inline Difficulty g_Difficulty = Difficulty::NORMAL;

// Run tuning shared by regular and ranged enemy spawns.
struct DifficultyParams {
    float spawnRateMultiplier;
    float enemyHpMultiplier;
    float waveSpawnCapMultiplier;
};
inline DifficultyParams GetDifficultyParams(Difficulty d) {
    switch (d) {
    case Difficulty::EASY:
        return { 0.85f, 0.90f, 0.85f };
    case Difficulty::NORMAL:
        return { 1.00f, 1.00f, 1.00f };
    case Difficulty::HARD:
        return { 1.15f, 1.15f, 1.15f };
    case Difficulty::NIGHTMARE:
        return { 1.30f, 1.30f, 1.30f };
    }
    return { 1.00f, 1.00f, 1.00f };
}

// ─── 시련 시스템 ─────────────────────────────────────────────────────────────
struct TrialDef {
    const wchar_t* id;
    const wchar_t* desc[3];   // [0]=KO [1]=EN [2]=JP
};
inline const TrialDef TRIAL_DEFS[] = {
    { L"OVERCLOCK",     { L"적 이동속도 +20%", L"Enemy speed +20%", L"敵の移動速度 +20%" } },
    { L"MEMORY_LEAK",   { L"최대 HP -25%", L"Max HP -25%", L"最大HP -25%" } },
    { L"TRIAL_SLOT_RETIRED", { L"", L"", L"" } },
    { L"TRIAL_SLOT_RETIRED", { L"", L"", L"" } },
    { L"PROCESS_LIMIT", { L"증강 선택지 2장", L"Only 2 aug choices", L"強化の選択肢 2枚" } },
    { L"LOW_BANDWIDTH", { L"스킬 쿨타임 +25%", L"Skill CD +25%", L"スキルCD +25%" } },
    { L"HARDENED",      { L"적 체력 +30%", L"Enemy HP +30%", L"敵HP +30%" } },
    { L"SURGE",         { L"적 속도·체력 +20%", L"Speed & HP +20%", L"敵の速度・HP +20%" } },
    { L"EARLY_RUSH",    { L"초반 원거리몹이 더 빨리 등장", L"Ranged mobs arrive earlier", L"序盤に遠距離敵が早く出現" } },
    { L"PACKET_STORM",  { L"초반 일반 몹 스폰 압박 증가", L"Early normal spawn pressure up", L"序盤の通常敵出現が増加" } },
    { L"COLD_BOOT",     { L"시작 최대 체력 감소", L"Lower starting max HP", L"開始時の最大HP減少" } },
    { L"MID_PRESSURE",   { L"중반 적 압박 +8%", L"Midgame enemy pressure +8%", L"中盤の敵圧力 +8%" } },
    // Reserved save slot kept at index 12 so older run records remain valid.
    { L"TRIAL_SLOT_RESERVED", { L"", L"", L"" } },
    { L"PROCESS_NOISE", { L"중반 원거리몹 상한 증가", L"Midgame ranged mob cap up", L"中盤の遠距離敵上限増加" } },
    { L"LATE_OVERRUN",  { L"후반 스폰 램프 강화", L"Late spawn ramp up", L"終盤の出現増加を強化" } },
    { L"HARDENED_CORE", { L"후반 몹 체력 램프 강화", L"Late enemy HP ramp up", L"終盤の敵HP上昇を強化" } },
    { L"SIGNAL_DRIFT",  { L"후반 원거리 압박 강화", L"Late ranged pressure up", L"終盤の遠距離圧力を強化" } },
    { L"TRIAL_SLOT_RETIRED", { L"", L"", L"" } },
    { L"TRIAL_SLOT_RETIRED", { L"", L"", L"" } },
    { L"TRIAL_SLOT_RETIRED", { L"", L"", L"" } },
};
inline constexpr int TRIAL_DEF_COUNT = 20;
inline constexpr int TRIAL_RESERVED_ID = 12;

// OVERCLOCK, MEMORY_LEAK, PROCESS_LIMIT, LOW_BANDWIDTH, HARDENED and SURGE
// have no gameplay effect implemented yet. Treat them as unavailable so they
// cannot be selected and never add the default +15% score bonus.
inline bool TrialDefUnimplemented(int id) {
    return id == 0 || id == 1 || (id >= 4 && id <= 7);
}

inline bool TrialDefRetired(int id) {
    return id < 0 || id >= TRIAL_DEF_COUNT || id == 2 || id == 3
        || id == TRIAL_RESERVED_ID || id >= 17 || TrialDefUnimplemented(id);
}

enum class TrialStage { EARLY, MID, LATE };

inline constexpr int TRIAL_SLOT_COUNT = 4;
inline int  g_TrialPool[TRIAL_SLOT_COUNT]     = { 8, 11, 14, TRIAL_RESERVED_ID };
inline bool g_TrialSelected[TRIAL_SLOT_COUNT] = { false, false, false, false };
inline bool g_TrialPoolReady                  = false;

inline TrialStage TrialStageForDef(int idx) {
    if (idx >= 8  && idx <= 10) return TrialStage::EARLY;
    if (idx == 11 || idx == 13) return TrialStage::MID;
    if (idx >= 14 && idx <= 16) return TrialStage::LATE;
    if (idx == 6 || idx == 7) return TrialStage::LATE;
    return TrialStage::EARLY;
}

inline float TrialScoreBonusForDef(int idx) {
    if (TrialDefRetired(idx)) return 0.0f;
    switch (idx) {
    case 8:  return 0.14f;
    case 9:  return 0.16f;
    case 10: return 0.18f;
    case 11: return 0.18f;
    case 13: return 0.17f;
    case 14: return 0.22f;
    case 15: return 0.24f;
    case 16: return 0.21f;
    default: return 0.15f;
    }
}

inline bool TrialActive(int defIdx) {
    if (TrialDefRetired(defIdx)) return false;
    for (int i = 0; i < TRIAL_SLOT_COUNT; ++i) {
        if (g_TrialSelected[i] && g_TrialPool[i] == defIdx)
            return true;
    }
    return false;
}

inline int TrialCount() {
    int count = 0;
    for (int i = 0; i < TRIAL_SLOT_COUNT; ++i)
        if (g_TrialSelected[i] && !TrialDefRetired(g_TrialPool[i])) ++count;
    return count;
}

inline float TrialScoreMult() {
    float mult = 1.0f;
    for (int i = 0; i < TRIAL_SLOT_COUNT; ++i) {
        if (g_TrialSelected[i] && !TrialDefRetired(g_TrialPool[i]))
            mult += TrialScoreBonusForDef(g_TrialPool[i]);
    }
    return mult;
}

inline void ResetTrials() {
    g_TrialPool[0] = 8;
    g_TrialPool[1] = 11;
    g_TrialPool[2] = 14;
    g_TrialPool[3] = TRIAL_RESERVED_ID;
    for (int i = 0; i < TRIAL_SLOT_COUNT; ++i)
        g_TrialSelected[i] = false;
    g_TrialPoolReady = false;
}

inline float TrialPlayerMaxHpMult() {
    float m = 1.0f;
    if (TrialActive(10)) m *= 0.82f;
    return m;
}

inline float TrialSpawnRateMult(long long score) {
    float m = 1.0f;
    if (TrialActive(9))  m *= 1.18f;
    if (TrialActive(11)) m *= 1.08f;
    if (TrialActive(14) && score >= 220000) m *= 1.24f;
    if (TrialActive(16) && score >= 320000) m *= 1.12f;
    return m;
}

inline float TrialEnemyHpMult(long long score) {
    float m = 1.0f;
    if (TrialActive(11)) m *= 1.08f;
    if (TrialActive(15) && score >= 220000) m *= 1.28f;
    return m;
}


inline float TrialRangedIntervalMult(long long score) {
    float m = 1.0f;
    if (TrialActive(8))  m *= 0.78f;
    if (TrialActive(13) && score >= 90000) m *= 0.86f;
    if (TrialActive(16) && score >= 280000) m *= 0.82f;
    return m;
}

inline int TrialRangedMaxBonus(long long score) {
    int bonus = 0;
    if (TrialActive(13) && score >= 90000) bonus += 2;
    if (TrialActive(16) && score >= 280000) bonus += 2;
    return bonus;
}

// 크리에이티브 모드 (런 설정 화면에서 토글)
//   ON: 게임 시작 시 score=100,000 (보스 즉시 등장)
//       RUNNING 중 F 키로 AUG_SELECT 즉시 열기
inline bool g_CreativeMode = false;
inline bool g_DebugMode = false;
// Title logo: 0 = vector constellation mark, 1 = handwritten "Asteria" (A),
// 2 = jagged "Asteria" (B). Chosen from the debug toolkit; not saved.
inline int g_TitleStyle = 1;

// Counts and the wallet never exceed this, so additions cannot overflow and
// every value fits the HUD (999,999,999,999,999).
inline constexpr long long SAVE_VALUE_MAX = 999999999999999LL;
inline long long ClampSaveValue(long long v) {
    return v < 0 ? 0 : v > SAVE_VALUE_MAX ? SAVE_VALUE_MAX : v;
}
inline void AddSaveValue(long long& total, long long amount) {
    total = ClampSaveValue(amount > 0 && total > SAVE_VALUE_MAX - amount
                               ? SAVE_VALUE_MAX : total + amount);
}

// A play that had debug mode on at any moment (F level-up, G godmode, the
// toolkit) records nothing: no best score, stardust, achievements or codex
// progress. Latched at play start and every in-play frame; cleared only by
// the next play's reset.
inline bool g_PlayDebugTainted = false;
inline bool PlayRecordsProgress() {
    return !g_CreativeMode && !g_PlayDebugTainted;
}

// Keep the debug switch visible for QA builds as well as Release packages.
// The switch remains opt-in and does not enable debug behavior by itself.
inline constexpr bool kDebugSettingsVisible = true;
inline bool g_BalanceTestMode = false;
// 크리에이티브 설정값 (CREATIVE_CONFIG 화면에서 조정)
inline long long g_CreativeStartScore = 0;       // 시작 점수
inline bool      g_CreativeGodmode    = false;   // G 키 무적 토글 (런타임)
inline bool      g_CreativeFreeGrab   = false;   // F 그랩 중 — 이 픽 뒤엔 디버프 페이지 스킵

// 손맛 표시 토글 (설정에서 ON/OFF)
inline bool g_ShowDamageNumbers = true;
inline bool g_ShowCombo         = true;   // 콤보 카운터 표시
inline int  g_SoundVol          = 100;    // 사운드 마스터 볼륨 (0=끄기 ~ 100)
inline int  g_BgmVol            = 100;    // BGM 채널 볼륨 (0~100, 0이면 꺼짐)
inline int  g_SfxVol            = 100;    // 효과음 채널 볼륨 (0~100, 0이면 꺼짐)
inline bool g_BgmEnabled        = true;   // g_BgmVol > 0 과 동기화 (구버전 저장 호환)
inline bool g_SfxEnabled        = true;
inline bool g_AudioMonoOutput   = false;
inline bool g_AudioEngineEnabled = true;

// 액티브 스킬 자동 사용 (C16) — ON 시 Q/E/R 스킬을 쿨다운 끝날 때마다 자동 발동.
//   기본 OFF(수동). 설정에서 토글.
inline bool g_AutoSkill         = false;

// CRT 셰이더 효과 (G) — 스캔라인 + 비네트 + 네온 글로우. 설정에서 토글.
// macOS: 투명 전체화면 합성 + 통합 GPU 부담 → 기본 OFF (세이브·macopt 마이그레이션으로도 적용)
#if defined(__APPLE__)
inline bool g_ShaderFx          = false;
#else
inline bool g_ShaderFx          = false;
#endif

// Strong readability overlay for out-of-game menu backgrounds.
inline bool g_StrongMenuDim     = false;

// Combat elements (bullets, HP/XP rings) sit straight on the desktop.
// Bright or busy wallpapers wash them out, so they always get a dark keyline.
// Smoke A/B checks can zero it (ASTERIA_SMOKE_KEYLINE=0).
// PLAY 패널 무기 별자리의 타원 궤도(끊어 그린 링) 표시. 디버그 툴킷에서 전환, 저장 안 함.
inline bool g_PlayConstellationOrbits = true;
inline float g_CombatKeylineScale = 1.0f;
inline float CombatKeylineStrength() {
    return g_CombatKeylineScale;
}

// 몹 외형 — CLASSIC=현재(강사님 OK), SOFT=채도↓·윤곽 부드럽게
enum class MobVisualStyle { CLASSIC, SOFT };
inline MobVisualStyle g_MobVisualStyle = MobVisualStyle::CLASSIC;

// 위성 VFX 밀도 — REDUCED=EMP·덫·패치 간격↑
// Keep the old save values: FULL=0, REDUCED=1; append the middle quality.
enum class VfxDensity { FULL = 0, REDUCED = 1, MEDIUM = 2 };
#if defined(__APPLE__)
inline VfxDensity g_VfxDensity = VfxDensity::REDUCED;
#else
inline VfxDensity g_VfxDensity = VfxDensity::FULL;
#endif
inline float VfxIntervalMult() {
    return g_VfxDensity == VfxDensity::REDUCED ? 1.45f
         : g_VfxDensity == VfxDensity::MEDIUM ? 1.20f : 1.0f;
}
inline int EnemyParticleCap() {
    return g_VfxDensity == VfxDensity::REDUCED ? 256
         : g_VfxDensity == VfxDensity::MEDIUM ? 512 : 1024;
}

// macOS 1회 성능 프로필 적용 여부 (asteria_save.cfg 의 macopt=1)
inline bool g_MacOptV1 = false;

// 원·부채꼴 등 다각형 분할 수 (VFX 밀도·플랫폼 반영)
inline int GfxCircleSegs() {
#if defined(__APPLE__)
    return g_VfxDensity == VfxDensity::REDUCED ? 6
         : g_VfxDensity == VfxDensity::MEDIUM ? 7 : 8;
#else
    return g_VfxDensity == VfxDensity::REDUCED ? 10
         : g_VfxDensity == VfxDensity::MEDIUM ? 11 : 12;
#endif
}
inline int GfxArcSegs(float mult = 1.5f) {
    int s = (int)(GfxCircleSegs() * mult + 0.5f);
    return s < 6 ? 6 : s;
}

// ── 코스메틱: OS 액센트 컬러 테마 (코인 상점에서 구매/장착, 저장됨) ──
//   플레이어 창 네온 보더/타이틀바 색을 바꾼다("데스크톱 테마" 커스터마이즈).
//   g_ThemeOwned 는 비트마스크(비트0=기본, 항상 보유), g_ThemeSel 은 선택 인덱스.
//   g_Accent* 는 선택 테마의 실제 RGB (ApplyAccentTheme 가 세팅, 렌더가 읽음).
inline int   g_ThemeOwned = 1;
inline int   g_ThemeSel   = 0;
inline float g_AccentR    = 0.30f;
inline float g_AccentG    = 1.0f;
inline float g_AccentB    = 1.0f;

// (추후 확장: 마스터 볼륨, 마우스 감도 등)
