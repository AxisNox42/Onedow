#pragma once

// ── 게임 설정 (런타임 변경 가능, 추후 설정 메뉴) ──

// FPS 캡:
//   0  = VSync (모니터 주사율)
//   -1 = 무제한
//   양수 = 해당 FPS 로 캡
inline int g_FpsCap = 0;

// 언어 (한국어 / 영어 / 일본어)
enum class Language { KR, EN, JP };
inline Language g_Language = Language::KR;
inline constexpr int LANG_COUNT = 3;

// 크로스헤어 표시 (기본 ON)
inline bool g_ShowCrosshair = true;

// 모든 언어 공통 폰트 — Microsoft YaHei UI (Win10+ 기본 탑재)
//   한글/라틴은 GDI 폰트 링크(자동 폴백)로, 일본어 가나·한자도 시스템 폴백으로 표시
//   (Windows 전용 — GDI face 이름)
inline const char* LanguageFace(Language /*lang*/) {
    return "Microsoft YaHei UI";
}

// 비-Windows(macOS/Linux): 언어별 폰트 폴백 체인 (TTF 파일 경로)
//   앞쪽이 주 폰트, 없는 글리프는 다음 폰트로 폴백
//   ※ 일본어(가나)를 맥에서 표시하려면 Resource/Font/NotoSansJP-Regular.ttf 가 필요.
//     (없으면 그 슬롯은 자동 스킵 — 빌드/실행엔 문제없고 일본어만 안 보임)
inline int LanguageFontChain(Language /*lang*/, const char* out[3]) {
    // 한/일/영 항상 동시 로드 (언어 무관 동일 체인) — 글리프 단위 폴백
    out[0] = "Font/Jua-Regular.ttf";                       // 한글
    out[1] = "Font/KosugiMaru-Regular.ttf";                // 일본어
    out[2] = "Resource/Font/Oswald-VariableFont_wght.ttf"; // 라틴
    return 3;
}

// 난이도 (게임 시작 시 적용)
enum class Difficulty { EASY, NORMAL, HARD };
inline Difficulty g_Difficulty = Difficulty::NORMAL;

// 난이도 별 몹 설정 헬퍼
struct DifficultyParams {
    float rangedSpawnInitialDelay; // 시작 시 spawn timer 오프셋
    float rangedSpawnInterval;     // 원거리 spawn 주기 (초)
    int   rangedMaxBase;           // 기본 max 마릿수
    // 자폭병
    float bomberStartTime;         // 게임 시작 후 자폭병 첫 등장 시간 (초). 1e9 = 안 나옴
    float bomberInterval;          // 자폭병 spawn 주기 (초)
    // 보스 HP (검객 등)
    float bossHp;
};
inline DifficultyParams GetDifficultyParams(Difficulty d) {
    switch (d) {
    case Difficulty::EASY:
        return { -5.0f, 5.0f, 2,   1e9f,  1e9f,  6000.0f };  // 자폭병 X · 보스 스펙업
    case Difficulty::NORMAL:
        return {  0.0f, 5.0f, 5,   30.0f, 5.0f, 15000.0f };
    case Difficulty::HARD:
        return {  4.9f, 2.5f, 8,   20.0f, 4.0f, 24000.0f };
    }
    return { 0.0f, 5.0f, 5, 1e9f, 1e9f, 15000.0f };
}

// 크리에이티브 모드 (난이도 선택 화면에서 토글)
//   ON: 게임 시작 시 score=100,000 (보스 즉시 등장)
//       RUNNING 중 F 키로 AUG_SELECT 즉시 열기
//       DEBUFF_SELECT 항상 스킵
inline bool g_CreativeMode = false;
// 크리에이티브 설정값 (CREATIVE_CONFIG 화면에서 조정)
inline long long g_CreativeStartScore = 0;       // 시작 점수
inline int       g_CreativeBossPick   = -1;      // -1=없음, 0슬라임 1글리치 2리로드 3폴리모프
inline int       g_CreativeStartAugs  = 0;       // 시작 시 무료 증강 픽 횟수
inline bool      g_CreativeGodmode    = false;   // G 키 무적 토글 (런타임)
inline bool      g_CreativeFreeGrab   = false;   // F 그랩 중 — 이 픽 뒤엔 디버프 페이지 스킵

// 손맛 표시 토글 (설정에서 ON/OFF)
inline bool g_ShowDamageNumbers = true;   // 데미지 숫자(딜 계산) 표시
inline bool g_ShowCombo         = true;   // 콤보 카운터 표시

// (추후 확장: 마스터 볼륨, 마우스 감도 등)
