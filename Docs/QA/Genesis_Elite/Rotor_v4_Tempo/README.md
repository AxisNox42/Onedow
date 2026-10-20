# 로터 빈 대기 축소

2026-10-18. 로터 차례의 빈 대기만 4.6초에서 0.25초로 줄였다. SWARM의 생산 주기와 로터의 1초 준비, 0.3초 연사, 반동·회복은 유지한다.

연결: SWARM 마지막 투척 → 0.1초 투척 마무리 → 0.2초 회복 → 0.25초 외피 정리 → 1초 로터 준비 → 연속 출격. 고정 업데이트 경계에서 몇 프레임 더 걸릴 수 있다.

`GENESISCYCLE`로 SWARM 조준부터 5.20초까지 실제 업데이트한다. 1440×900, 0.08초 간격의 실제 캡처 66장이다. GIF 프레임은 각 80ms이며 합성 이동·배속 변경은 없다.

- `Cycle_Preview.gif`: SWARM부터 로터 연사·회복까지 전체 연결.
- `WaitToRotor_Closeup.gif`: SWARM 종료 이후 1.60–4.80초의 전환·준비·연사 확대.
- `Transition_ContactSheet.png`: 대표 시점.

로터 회귀는 0.25초 대기 동안 SWARM 스택·자유 회전·유닛 생성이 없고 바로 준비로 연결됨을 검사한다. 기존 로터 발사·충돌·취소·교대와 SWARM 투척 회귀도 실행한다.

결과: 두 회귀와 Windows x64 Release 빌드 통과. 실제 66프레임에서 짧은 외피 정리 뒤 준비·연사가 이어지는 것을 확인했다. 기존 `APIENTRY` 경고는 남아 있다.

```powershell
python scripts/preview_genesis_transition.py build/smoke/genesis_rotor_tempo_v4 Docs/QA/Genesis_Elite/Rotor_v4_Tempo --focus-end 4.8
```
