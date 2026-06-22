#pragma once
// ─────────────────────────────────────────────────────────────
//  Audio — miniaudio 기반 사운드 시스템 (가벼운 선언부)
//   · Sounds/ 폴더에서 로드. 파일이 없으면 조용히 무시(크래시 X).
//   · 파일을 하나씩 떨궈 넣으면 그때부터 자동으로 소리남.
//   · 볼륨은 Audio.cpp 의 게인 테이블에서 코드로 조절(파일 안 건드림).
// ─────────────────────────────────────────────────────────────
namespace Audio {

enum class Sfx {
    Shoot,    // sfx_shoot.wav
    Kill,     // sfx_kill.wav
    COUNT
};

void Init();
void Shutdown();
void PlaySfx(Sfx s);
void PlayBgmMain();       // bgm_main.mp3 루프
void StopBgm();
void SetEnabled(bool on); // 마스터 ON/OFF (OFF 시 BGM 정지)
bool IsEnabled();
void SetVolume(float v);  // 마스터 볼륨 0.0~1.0

} // namespace Audio
