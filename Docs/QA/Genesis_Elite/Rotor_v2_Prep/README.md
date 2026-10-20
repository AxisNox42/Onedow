# 제네시스 준비 모션 v2

2026-10-18. 준비 0.4초를 1.0초로 늘리고 힘 모으기·가이드 정렬·준비 자세 유지·빠른 후방 이송으로 나눴다. 조준 갱신, 총 여섯 발, 0.3초 연사 간격, 강한 반동과 SWARM 투척은 유지한다.

실제 1440×900 캡처 96장: 0.00–3.80초, 0.04초 간격. GIF는 프레임당 40ms로 실제 속도이며 합성 이동·슬로모션을 추가하지 않는다.

- `Rotor_Preview.gif`: 전체 패턴.
- `Prep_Closeup.gif`: 준비 시작부터 1.24초까지 본체·생산부·포구 확대.
- `Prep_BeforeAfter.gif`: 같은 시점의 이전 0.4초 준비(왼쪽)와 현재 1.0초 준비(오른쪽). 두 버전 모두 실제 캡처이며 타임라인을 맞췄다.
- `Motion_ContactSheet.png`: 대표 시점.

준비 구간: 0.00–0.12초 힘 모으기, 0.12–0.70초 가이드 정렬, 0.70–0.88초 준비 자세 유지, 0.88–1.00초 이송. 전체 준비 동안 현재 플레이어 위치를 따라 조준한다.

검증: `test_genesis_rotor.ps1`(1초 전 미발사, 자세 완성·유지, 힘 모으기 종료, 기존 발사·충돌·교대·취소 검사), `test_genesis_launcher.ps1` 통과. Windows x64 Release 빌드 성공. 기존 `APIENTRY` 경고는 남아 있다.

재현:

```powershell
python scripts/preview_genesis_rotor.py build/smoke/genesis_rotor_prep_v2 Docs/QA/Genesis_Elite/Rotor_v2_Prep --baseline build/smoke/genesis_rotor_motion_v1
```
