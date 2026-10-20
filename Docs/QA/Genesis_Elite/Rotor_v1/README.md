# 제네시스 후방 생산 로터 출격 QA

검증일: 2026-10-18

SWARM 투척은 유지하고 이전 협공만 제거·교체한다. 현재 스모크 단계는 `GENESISROTOR=<초>`다.

결과: Windows x64 Release 빌드와 아래 회귀가 통과했다. 1440×900의 81프레임 및 1280×720의 생산·반동·연사·회복 네 시점 스모크를 확인했다. 기존 `APIENTRY` 매크로 재정의 경고는 남아 있다.

- `scripts/test_genesis_rotor.ps1`: 6회 출격, 0.3초 간격, 매 발 현재 위치 조준, 후방 이송, 고속 선분 충돌, 발사 후 고정 방향, 착지 폭발 없음, 상한 15/130, 장전 유닛 격추, 사망 취소, 반동 경계, 긴 프레임의 연사 간격 보장, SWARM으로 복귀.
- `scripts/test_genesis_launcher.ps1`: 기존 SWARM 투척 회귀.
- `scripts/test_thrown_swarm_kill.ps1`: SWARM·ROTOR 모두 콤보·해당 도감만 기록하고 일반 처치 보상을 지급하지 않음.
- `scripts/test_quasar_fall.ps1`: 공통 업데이트 변경 후 퀘이사 회귀.

`Rotor_Preview.gif`와 `Motion_ContactSheet.png`는 실제 렌더러의 지정 시점 캡처를 모은다. 1440×900에서 0.00–3.20초를 0.04초 간격으로 재현하며 GIF 각 프레임은 40ms다. 중간 이동 프레임을 합성하지 않는다. 플레이어는 매 재현에서 같은 곡선으로 움직인다.

확인할 모션: 후방 생산 1외피·전방 가이드 2외피, 후방에서 코어를 지나는 가속 이송, 큰 사각 로터의 고속 출격, 본체 후퇴와 가이드 압축, 다음 발 전 복귀, 비행 감속·정리 뒤 일반 추적으로 이어지는 회전. 출격 후에도 사각 윤곽과 낮은 광번짐을 유지한다.

재현 예:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke_ui.ps1 -Steps 'GENESISROTOR=0.32;GENESISROTOR=0.44;GENESISROTOR=0.74;GENESISROTOR=2.2' -Tag rotor_review -Size 1440x900
python scripts/preview_genesis_rotor.py build/smoke/genesis_rotor_motion_v1 Docs/QA/Genesis_Elite/Rotor_v1
```

혼합 웨이브 난이도·큰 플레이어 빌드의 체감 회피 여유·실전 성능 측정은 별도 플레이 검증 대상이다. 이 자료의 재현 캡처 FPS 표시는 정상 실행 성능을 측정한 값이 아니다.
