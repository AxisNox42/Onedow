#pragma once
// ─────────────────────────────────────────────────────────────
//  Audio — miniaudio 기반 사운드 시스템 (가벼운 선언부)
//   · Sounds/ 폴더에서 로드. 파일이 없으면 조용히 무시(크래시 X).
//   · 파일을 하나씩 떨궈 넣으면 그때부터 자동으로 소리남.
//   · 볼륨은 Audio.cpp 의 게인 테이블에서 코드로 조절(파일 안 건드림).
// ─────────────────────────────────────────────────────────────
namespace Audio {

enum class Sfx {
    Shoot,    // 발사 (sfx_shoot.wav)
    Kill,     // 적 처치 (sfx_kill.wav)
    Hurt,     // 플레이어 피격 (sfx_hurt.wav)
    Death,    // 플레이어 사망 (sfx_death.wav)
    Phase2,   // 보스 페이즈2 전환 (sfx_glitch_phase2.wav)
    COUNT
};

void Init();              // 엔진 + SFX 프리로드
void Shutdown();
void PlaySfx(Sfx s);      // 원샷 (없으면 무음)
void PlayBgmMain();       // bgm_main.mp3 루프 (이미 재생 중이면 무시)
void PlayBgmBoss();       // bgm_boss.mp3 루프
void StopBgm();
void SetEnabled(bool on); // 마스터 ON/OFF (OFF 시 BGM 정지)
bool IsEnabled();

} // namespace Audio
