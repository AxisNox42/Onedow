# 난이도 선택 UI 리디자인 계획서

작성일: 2026-08-19

## 목표

메인 메뉴의 OS/브라우저 위장 컨셉을 유지하면서, 난이도 선택 화면을 단순한 버튼 선택이 아니라 "이번 런의 설정을 조립하는 화면"처럼 보이게 만든다.

단, 화면의 중심은 반드시 난이도 선택이어야 한다. 무기 선택, 시련 선택, 점수 배율은 보조 정보로 배치한다.

## 핵심 방향

- 메인 메뉴와 같은 `DrawMenuBackground` 기반의 파티클, 스캔라인, 비네트 분위기를 유지한다.
- 화면 전체를 게임 UI가 아니라 `onedow.exe` 내부 설정 창처럼 보이게 한다.
- 제목은 `RUN CONFIG`로 두되, 첫 번째 섹션은 `DIFFICULTY`로 크게 잡는다.
- Easy / Normal / Hard는 한눈에 비교 가능한 3개 대형 카드로 보여준다.
- 현재 선택된 난이도는 색, 테두리, 하단 상태 문구까지 함께 바뀌어야 한다.
- Trial과 Loadout은 현재 구조를 유지하되, 난이도보다 시각적 위계를 낮춘다.

## 화면 구조

```text
MAIN_MENU
  -> RUN CONFIG
      [DIFFICULTY]
        EASY     NORMAL     HARD

      [LOADOUT]
        Rifle    Revolver   Shotgun   SMG

      [TRIAL]
        Trial A  Trial B    Trial C

      SCORE RATE / BACK / REROLL / EXECUTE
```

## 레이아웃

### 1. 브라우저 창

- 기존 `SceneFlowWindow` 스타일을 유지한다.
- URL 표기는 `onedow.exe/runconfig.odw` 또는 `onedow.exe/difficulty.odw`가 적합하다.
- 패널 크기는 고정값만 쓰지 말고 화면에 맞춰 줄인다.

권장값:

```cpp
FW = min(FLOW_PANEL_W, sw * 0.92f)
FH = min(FLOW_PANEL_H, sh * 0.88f)
```

작은 해상도에서는 카드 크기와 간격도 같은 비율로 줄인다.

### 2. 상단 영역

- 중앙: `RUN CONFIG`
- 우측: `COIN 0000`
- 좌측 또는 제목 아래: 현재 선택 요약

예시:

```text
PROFILE: NORMAL / RIFLE / 2 TRIALS
```

이 요약은 플레이어가 `EXECUTE`를 누르기 직전에 현재 설정을 다시 확인하는 역할을 한다.

## 난이도 카드

난이도 선택은 화면에서 가장 커야 한다.

### EASY

역할: 입문, 첫 클리어, 패턴 학습

- 색상: 녹색 + 연한 cyan
- 설명: `pattern assist / lower pressure`
- 점수 배율: x0.85 또는 x0.90
- 일부 보스 패턴 제외 가능
- 적 탄속, 적 피해량, 보스 패턴 빈도 감소

### NORMAL

역할: 기본 경험

- 색상: cyan + 흰색
- 설명: `standard onedow protocol`
- 점수 배율: x1.00
- 모든 기본 패턴 사용
- 현재 밸런스의 기준점

### HARD

역할: 숙련자용 압박

- 색상: 붉은색 + 주황색
- 설명: `unstable rules / full pressure`
- 점수 배율: x1.20 또는 x1.25
- 보스 추가 패턴 활성화
- 적 탄속, 적 스폰 압박, 보스 쿨타임 일부 강화

## 카드 디자인

각 난이도 카드는 메인 메뉴의 헥사 버튼 감성을 유지하되, 정보량이 있으므로 완전한 육각형 버튼보다는 잘린 모서리 패널이 적합하다.

구성:

```text
[ EASY ]
low threat

Enemy Damage  -15%
Boss Pattern  reduced
Score Rate    x0.90

[ SELECTED ]
```

선택 상태:

- 카드 외곽 glow 2단
- 카드 내부 상단에 얇은 진행 바 느낌의 라인
- 하단에 `[ SELECTED ]`
- 배경에 아주 약한 펄스

Hover 상태:

- Y 위치 -4px
- 테두리 alpha 증가
- 배경 밝기 소폭 증가
- 클릭 가능 영역은 카드 전체

## Loadout 영역

현재 무기 카드 구조는 유지한다.

변경 방향:

- 난이도 카드보다 작게 배치한다.
- 카드명과 역할만 빠르게 읽히면 된다.
- 잠긴 무기는 어둡게 처리하되, 구매 가능한 경우 코인 색을 강조한다.
- 선택된 무기는 난이도 선택과 같은 `[ SELECTED ]` 규칙을 사용한다.

권장 문구:

```text
RIFLE      balanced
REVOLVER   burst single-shot
SHOTGUN    close pressure
SMG        high fire rate
```

## Trial 영역

Trial은 난이도와 별개로 "추가 위험을 걸고 점수 배율을 얻는 선택지"로 보이게 한다.

변경 방향:

- 섹션명을 `TRIAL`보다 `RISK MODULE`로 바꾸는 것도 좋다.
- 선택 시 카드가 켜지는 느낌을 명확하게 준다.
- `SCORE RATE`는 Trial 아래가 아니라 하단 실행 버튼 근처에 둔다.

Trial 선택 규칙:

- 0개: x1.00
- 1개: x1.20
- 2개: x1.35
- 3개: x1.50

난이도 배율과 Trial 배율을 함께 쓸 경우, 최종 배율을 별도 표시한다.

예시:

```text
SCORE RATE  x1.62
NORMAL x1.00 / 3 TRIALS x1.50
```

## 하단 버튼

하단 구성:

```text
[ BACK ]                SCORE RATE x1.00             [ REROLL ] [ EXECUTE ]
```

- `EXECUTE`가 가장 강한 버튼이어야 한다.
- `REROLL`은 Trial 영역과 연관된 보조 버튼으로 보이게 한다.
- `BACK`은 좌하단 고정.
- `DEV MODE`는 최종 빌드에서는 숨기거나 설정/디버그 메뉴로 이동한다.

## 애니메이션

진입:

- 배경은 메인 메뉴와 동일하게 유지
- 브라우저 창 alpha 0 -> 1
- 난이도 카드 3개가 0.06초 간격으로 아래에서 올라옴
- Loadout, Trial은 난이도 카드보다 늦게 약하게 페이드인

선택:

- 선택한 카드에 짧은 scan flash
- 하단 요약 문구가 0.15초 동안 깜빡이며 갱신
- `EXECUTE` 버튼 glow가 1회 반응

실행:

- 선택된 난이도 카드만 남기고 나머지는 빠르게 dim
- `EXECUTE` 버튼이 짧게 밝아진 뒤 게임 시작 페이드

## 색상 규칙

- 기본 UI: cyan / blue white
- Easy: green
- Normal: cyan
- Hard: red orange
- Coin: yellow gold
- Locked: dark gray + low alpha red
- Trial active: violet을 과하게 쓰지 말고 cyan + warning orange 중심

전체 화면이 한 가지 파란색으로만 보이지 않게 난이도별 포인트 컬러를 반드시 넣는다.

## 구현 우선순위

1. `DIFFICULTY` 섹션을 다시 추가하고 Easy / Normal / Hard 선택 상태를 복구한다.
2. `EXECUTE`에서 `g_Difficulty = Difficulty::NORMAL` 고정을 제거한다.
3. 난이도 카드 3개를 현재 Loadout보다 위에 배치한다.
4. 하단에 현재 선택 요약과 최종 점수 배율을 표시한다.
5. 고정 패널 크기를 해상도 대응형으로 바꾼다.
6. Trial 영역과 Reroll 버튼의 시각적 관계를 정리한다.
7. DEV MODE 토글을 숨기거나 디버그 전용 조건으로 분리한다.

## 완료 기준

- 메인 메뉴와 같은 화면 세계관으로 보인다.
- 사용자가 3초 안에 현재 난이도와 무기를 알 수 있다.
- Easy / Normal / Hard 선택이 실제 게임 난이도에 반영된다.
- 1366x768 해상도에서도 주요 버튼이 잘리지 않는다.
- `EXECUTE`를 누르기 전 최종 설정 요약을 확인할 수 있다.
- Trial은 난이도 선택을 방해하지 않고, 추가 보상 선택지로 보인다.
