# 보스 기획안: TESS.glitch 스킬 리워크

## 1. 방향 정리

외형은 현재 글리치 3D 큐브 방향을 유지한다.

공격은 다시 정리한다. 이전에 넣었던 `Axis Slice`, `Fold Zone`, `Screen Shear`, `Coordinate Snap`, `Null Frame`은 전부 제거한다. 3페이즈에서 열리는 `Dimension Lock`만 유지하고, 나머지는 새 스킬로 다시 구성한다.

TESS는 투사체를 쏘는 보스가 아니라, 짧게 예고된 공간 이상 현상을 읽고 이동하는 보스다.

## 2. 유지할 스킬

### Dimension Lock

3페이즈 전용 능력.

화면의 특정 축을 기준으로 안전 통로가 생기고, 통로 바깥쪽 영역이 위험해진다. 입력을 막거나 반전하지 않고, 플레이어가 자연스럽게 해당 통로 안에서 움직이게 만드는 압박 스킬이다.

규칙:
- 1~2페이즈에서는 사용하지 않는다.
- 3페이즈 진입 후 핵심 패턴으로 사용한다.
- 다른 스킬과 조합해도 안전 통로는 항상 눈에 보여야 한다.

## 3. 새 스킬 구성

### Core Pulse

큐브 중심 또는 플레이어 근처에서 원형 펄스가 발생한다.

변형:
- 링 형태: 원형 띠만 위험
- 디스크 형태: 원 내부가 위험

역할:
- 1페이즈 기본기
- 테세랙트가 공간을 두드리는 느낌을 줌
- 투사체 없이도 이동을 유도함

### Quadrant Crash

화면 4분면 중 하나가 깨진다.

예고된 사분면 전체가 짧게 위험해진다. 플레이어가 서 있는 위치와 반대 방향 또는 주변 사분면을 우선 선택해서 억까를 줄인다.

역할:
- 큰 그림으로 위치를 바꾸게 만드는 스킬
- 패턴이 단순해서 1페이즈 학습용으로 좋음

### Gravity Well

플레이어 주변에 작은 중력장 구역이 생긴다.

예고 후 일정 시간 동안 구역 안에 있으면 낮은 지속 피해를 받는다. 실제 이동 조작을 끌어당기지는 않는다.

역할:
- 한 지점에 오래 머무는 플레이를 방지
- 2페이즈부터 다른 스킬과 조합 가능

### Mirror Fault

플레이어 근처와 화면 반대편에 서로 연결된 두 개의 위험 구역이 생긴다.

두 지점 사이에는 얇은 글리치 선이 그려져, 화면이 접힌다는 느낌을 준다.

역할:
- 2페이즈 핵심 스킬
- 한 구역만 보고 피하면 반대편 구역에 걸릴 수 있게 만든다.
- 하지만 위험 구역은 원형이라 판정이 읽기 쉬워야 한다.

## 4. 페이즈 설계

### Phase 1: Core Pulse

사용 스킬:
- Core Pulse
- Quadrant Crash
- Gravity Well 단독

목표:
- 투사체 없는 공간 판정 보스라는 룰을 학습시킨다.
- 예고 시간을 길게 둔다.
- 한 번에 하나의 정보만 읽게 한다.

### Phase 2: Mirror Fault

사용 스킬:
- Mirror Fault
- Core Pulse
- Quadrant Crash
- Gravity Well

목표:
- 화면 반대편까지 확인하게 만든다.
- 2개 스킬 조합을 시작한다.
- 그래도 Dimension Lock은 아직 쓰지 않는다.

### Phase 3: Dimension Lock

사용 스킬:
- Dimension Lock
- Core Pulse
- Mirror Fault
- Gravity Well
- Quadrant Crash

목표:
- Dimension Lock으로 안전 통로를 만든 뒤, 다른 공간 스킬을 섞어 압박한다.
- 이동 제한 느낌은 주되 조작 방해는 하지 않는다.
- 최종 페이즈답게 빠르게 연속 조합한다.

추천 조합:
- Dimension Lock -> Core Pulse -> Mirror Fault
- Dimension Lock -> Quadrant Crash -> Gravity Well
- Gravity Well 2개 -> Quadrant Crash
- Mirror Fault 2회 -> Core Pulse

## 5. 난이도 기준

쉬움:
- 예고 시간을 길게 둔다.
- Gravity Well 크기를 줄인다.
- Dimension Lock 통로를 넓힌다.

보통:
- 기본값 사용
- 2페이즈부터 조합 패턴 사용

어려움:
- 예고 시간을 약간 줄인다.
- Mirror Fault와 Gravity Well 조합 빈도를 높인다.
- 3페이즈 Dimension Lock 통로를 좁힌다.

## 6. 구현 우선순위

1. 기존 Axis/Fold/Shear/Snap/NullFrame 제거
2. Core Pulse 구현
3. Quadrant Crash 구현
4. Gravity Well 구현
5. Mirror Fault 구현
6. Dimension Lock 유지 및 3페이즈 전용화
7. 페이즈별 로테이션 재구성

## 7. 한 줄 요약

`TESS.glitch`는 3페이즈의 `Dimension Lock`을 핵심으로 두고, 투사체 대신 원형 펄스, 사분면 붕괴, 중력장, 미러 결함으로 공간을 고장내는 보스다.
