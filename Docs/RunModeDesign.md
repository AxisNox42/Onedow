# 메인 모드 / 무한 모드 구조 설계

작성일: 2026-08-11  
대상 체크리스트: `메인 모드 / 무한 모드 구조 설계 문서 작성`

## 결정

최근 추가된 층/탑 진행 개념은 폐기한다.

메인 모드는 층을 오르는 구조가 아니라, 정해진 보스 시퀀스를 클리어하는 유한 런으로 둔다. 무한 모드는 기존 점수 기반 생존 루프를 정규화한다. 두 모드는 같은 전투, 성장, 보스, 상점 시스템을 공유하되 런 종료 조건과 기록 정책만 분리한다.

## 원칙

- `GameState`는 화면 상태만 표현한다.
- 메인/무한/크리에이티브 구분은 별도 `RunMode`로 표현한다.
- 층, 층 목표 처치 수, 층 클리어 배너, 탑 클리어 문구는 쓰지 않는다.
- 정규 모드에는 기획 검증된 보스만 넣는다.
- 비활성 보스는 정규 편입 전제가 아니라 삭제 또는 부품화 대상이다.
- 크리에이티브는 테스트용이며 기록과 보상을 남기지 않는다.

## RunMode

```cpp
enum class RunMode {
    Main,
    Endless,
    Creative
};
```

초기 구현은 전역 `g_RunMode`로 시작해도 된다. 이후 기술 부채 해소 단계에서 `RunState::mode`로 이동한다.

## 모드별 목표

### 메인 모드

메인 모드는 데모의 기준 경험이다.

- 목표: 정해진 보스 시퀀스를 끝까지 클리어한다.
- 진행: 점수/킬/시간으로 일반 구간을 진행하다가 보스 조건을 만족하면 다음 보스가 등장한다.
- 종료: 마지막 정규 보스 처치 시 `VICTORY`.
- 기록: 클리어 여부, 점수, 처치 수, 생존 시간, 난이도, 직업/무기, 선택 증강.
- 보상: 코인, 업적, 도감 해금.

권장 보스 시퀀스 1차안:

```text
FORK -> VOLLEY -> Final
```

최종 보스가 준비되지 않은 동안은 다음 중 하나로 둔다.

```text
FORK -> VOLLEY -> VOLLEY 강화형
FORK -> VOLLEY -> FORK 강화형
FORK -> VOLLEY -> 데모 전용 임시 보스
```

중요한 점은 “보스 수를 늘리는 것”이 아니라 “정식 흐름의 마침표가 되는 보스가 있는 것”이다.

### 무한 모드

무한 모드는 기존 점수 경쟁/생존 구조를 유지한다.

- 목표: 사망 전까지 최대한 오래 생존하고 높은 점수를 낸다.
- 진행: 점수 또는 시간 기준으로 보스가 반복 등장한다.
- 종료: 플레이어 사망 시 `GAMEOVER`.
- 클리어: 없음.
- 기록: 최고 점수, 생존 시간, 처치 수, 보스 처치 수, 난이도, 직업/무기, 선택 증강.
- 보상: 코인과 일반 업적은 허용. 메인 클리어 전용 보상은 없음.

무한 모드의 보스는 메인에서 검증된 보스만 반복한다.

권장 반복안:

```text
FORK -> VOLLEY -> FORK 강화 -> VOLLEY 강화 -> 반복
```

### 크리에이티브 모드

크리에이티브는 개발/테스트 모드다.

- 시작 점수, 보스, 시작 증강 등을 설정할 수 있다.
- 기록을 남기지 않는다.
- 코인과 업적을 지급하지 않는다.
- 정규 편입 전 보스나 패턴을 테스트할 수는 있지만, 정규 콘텐츠로 간주하지 않는다.

## 공통 흐름

```text
MAIN_MENU
  -> MODE_SELECT
  -> DIFFICULTY_SELECT
  -> JOB_SELECT
  -> READY
  -> RUNNING
      -> AUG_SELECT / DEBUFF_SELECT
      -> boss warning
      -> boss fight
      -> intermission / RUN_SHOP
      -> RUNNING
  -> GAMEOVER or VICTORY
```

`MODE_SELECT`가 최종 구조다. 단기 구현에서는 난이도 선택 화면에 `Main / Endless` 토글을 임시로 붙여도 된다.

## RunRules

모드별 규칙은 값으로 분리한다.

```cpp
struct RunRules {
    bool allowVictory;
    bool allowRecords;
    bool allowMetaRewards;
    bool scoreBasedBosses;
    bool fixedBossSequence;
    int maxBossClears;
};
```

권장 기본값:

```text
Main:
  allowVictory = true
  allowRecords = true
  allowMetaRewards = true
  scoreBasedBosses = true
  fixedBossSequence = true
  maxBossClears = 3

Endless:
  allowVictory = false
  allowRecords = true
  allowMetaRewards = true
  scoreBasedBosses = true
  fixedBossSequence = false
  maxBossClears = -1

Creative:
  allowVictory = false
  allowRecords = false
  allowMetaRewards = false
  scoreBasedBosses = false
  fixedBossSequence = false
  maxBossClears = -1
```

## 보스 등장

층 조건은 사용하지 않는다.

메인 모드:

```text
score >= nextBossScore
  -> bossSequence[bossClearCount] 등장
```

무한 모드:

```text
score >= nextBossScore
  -> 반복 로테이션에서 다음 보스 등장
```

크리에이티브:

```text
설정된 보스 즉시 등장 또는 수동 호출
```

`g_NextBossScore`는 유지한다. 첫 보스는 낮은 점수에서 나오고, 이후 보스는 일정 간격으로 재설정한다.

권장값:

```text
firstBossScore = 50,000
nextBossGap = 200,000
```

보스 처치 직후 점수가 이미 다음 기준을 넘었으면 `currentScore + nextBossGap`으로 재설정해서 연속 보스 소환을 막는다.

## 보스 처치

메인 모드:

```text
bossClearCount++
if bossClearCount >= mainBossSequenceLength:
    VICTORY
else:
    intermission / run shop
    nextBossScore 재설정
```

무한 모드:

```text
bossClearCount++
intermission / run shop
nextBossScore 재설정
```

크리에이티브:

```text
테스트 정책에 따라 즉시 RUNNING 또는 intermission
기록/보상 없음
```

## 보스 정책

정규 보스 편입 기준:

- 이름과 컨셉이 게임 테마에 맞는다.
- 플레이어에게 요구하는 대응이 명확하다.
- 기존 보스와 탄막 언어가 겹치지 않는다.
- 페이즈 전환 이유가 있다.
- 시각 연출과 히트 판정이 설명 가능하다.
- 1분 안에 “이 보스는 무엇을 시험하는가”를 설명할 수 있다.

비활성 보스 처리:

- 기획이 없으면 정규 로테이션에 넣지 않는다.
- 쓸만한 패턴만 분리해서 보관할 수 있다.
- 유지 비용이 큰 코드는 삭제한다.

## UI

### 모드 선택

필수 선택지:

- Main Mode: 정해진 보스 시퀀스를 클리어.
- Endless Mode: 죽을 때까지 생존.
- Creative: 개발자 해금 시 표시, 기록 없음.

### HUD

메인 모드:

- 현재 보스 진행도.
- 다음 보스까지 남은 점수.
- 런 골드.
- 보스 경고.

무한 모드:

- 생존 시간.
- 다음 보스까지 남은 점수.
- 처치 수.
- 런 골드.

크리에이티브:

- Creative 표시.
- 기록/보상 비활성 표시.
- 선택된 테스트 조건.

층 번호, 층 목표 처치 수, 층 클리어 문구는 표시하지 않는다.

## 기록

메인과 무한은 기록을 분리한다.

메인 기록:

- 클리어 여부.
- 최고 점수.
- 최고 난이도 클리어.
- 클리어 시간.

무한 기록:

- 최고 점수.
- 최고 생존 시간.
- 최대 보스 처치 수.

기존 최고점은 무한 모드 기록으로 간주하는 것이 가장 안전하다. 메인 기록은 새 구조 구현 후부터 쌓는다.

## 구현 순서

1. `RunMode.h` 추가.
2. `g_RunMode`와 `RulesFor(g_RunMode)` 추가.
3. 모드 선택 UI 추가.
4. `g_CreativeMode`가 정규 모드 분기를 대신하던 부분을 `RunMode` 기준으로 치환.
5. 보스 등장 조건을 점수 기반 공통 함수로 분리.
6. 메인 보스 시퀀스와 무한 보스 로테이션을 분리.
7. 보스 처치 후 `VICTORY` 여부를 `RunRules`로 판단.
8. 결과 기록을 메인/무한으로 분리.
9. HUD에서 다음 보스 진행도와 생존 정보를 모드별로 표시.
10. 비활성 보스 잔여 코드 삭제 또는 부품화.

## 수용 기준

- 층 관련 코드와 UI가 없다.
- 메인 모드는 보스 시퀀스 완료 시에만 승리한다.
- 무한 모드는 승리 없이 사망까지 진행된다.
- 메인/무한 기록이 섞이지 않는다.
- 크리에이티브는 기록과 보상을 남기지 않는다.
- 비활성 보스는 정규 로테이션에 섞이지 않는다.

## 이번 문서에서 보류

- 신규 보스 상세 기획.
- 최종 보스 패턴.
- 스킬 트리.
- Steam 연동.
- 전체 UI 리디자인.
- `main.cpp` 대규모 분리.
