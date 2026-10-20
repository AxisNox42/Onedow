# SWARM 충전 → 로터 전환 오류 수정

2026-10-18. 로터 차례의 공통 대기까지 실제 재현해 확인한다. 이전 `GENESISROTOR` fixture는 대기를 건너뛰어 잘못된 SWARM 충전 모션을 드러내지 못했다.

원인: 다음 공격을 SWARM 3스택이 가득 찬 뒤에 분기하고, 대기 렌더링도 공통 SWARM 충전/회전을 사용했다.

수정: 로터 차례의 대기·표시를 분리한다. 4.6초 동안 충전량은 0이며 빈 후방 생산부를 유지한다. 외피의 SWARM 자유 회전을 제거하고, 직전 투척의 반응을 0.25초 동안 정리한다. 대기 완료는 시간으로 판정하며 같은 빈 자세에서 로터 준비로 연결한다. SWARM 차례의 기존 충전·투척은 그대로 둔다.

- `GENESISCYCLE=<초>`: 실제 SWARM 조준 시작부터 전체 교대를 업데이트한다.
- 1440×900, 0.00–10.00초를 0.08초 간격으로 캡처한다. 126개 실제 프레임, GIF 각 80ms. 합성 이동·슬로모션 없음.
- `Cycle_Preview.gif`: 전체 교대.
- `WaitToRotor_Closeup.gif`: 직전 SWARM 종료부터 로터 대기·준비·첫 출격까지 확대.
- `Transition_ContactSheet.png`: 실제 단계별 대표 시점.

전용 회귀는 로터 대기 4.6초 동안 충전량 0·외피 회전 0·유닛 생성 없음, 대기→준비 경계의 같은 빈 자세를 검사한다. 기존 로터 발사·충돌·취소·교대 및 SWARM 투척 회귀도 통과했다.

Windows x64 Release 빌드 성공. 실제 126프레임을 확인해 SWARM 종료 후 대기 구간에 삼각 SWARM 생산이 없고, 빈 후방 외피에서 로터 생산·출격으로 이어지는 것을 확인했다. 기존 `APIENTRY` 경고는 남아 있다.

```powershell
python scripts/preview_genesis_transition.py build/smoke/genesis_rotor_cycle_v3 Docs/QA/Genesis_Elite/Rotor_v3_Transition
```
