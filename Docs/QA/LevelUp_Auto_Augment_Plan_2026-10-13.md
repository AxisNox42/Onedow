# 레벨업 자동 증강 진입 계획

2026-10-13 기준. 레벨업 후 Space를 눌러 증강 선택 화면을 여는 구조를, **레벨업 순간 게임이 느려지며 자연스럽게 증강 선택 화면으로 넘어가는 구조**로 바꾼다.

이 작업은 [중복 리팩터링 계획](Duplication_Refactor_Plan_2026-10-12.md)과 동시에 진행한다. 따라서 **새 동작을 기존 코드에 복사해 붙이지 않는다.** 이 기능이 지나가는 경로에 이미 있는 반복 코드를 먼저 함수로 묶고, 새 동작은 그 함수를 확장하는 방식으로만 넣는다. 구조 이관과 동작 변경은 리팩터링 계획 §8.1에 따라 별도 커밋·별도 검증으로 나눈다.

## 1. 현재 흐름 (조사 결과)

| 단계 | 위치 | 동작 |
|---|---|---|
| 레벨 문턱 통과 | `main.cpp` 고정 틱 레벨업 루프 (`ConsumeNextLevel`) | 레벨마다 `RecordLevelUp` 후 버프 보상 큐에 적재 |
| 대기 표시 | `GameManager::QueueAugmentReward` | 큐 적재 + `augReady = true`. 게임은 계속 진행 |
| 안내 문구 | `main.cpp` HUD `kAugHint` | `[ SPACE ] 증강 픽업` 3개 언어 |
| 진입 | `GameManager::HandleInput` | RUNNING에서 Space: `augReady`면 `AUG_SELECT`, 아니면 **일시정지** |
| 보상 활성화 | `main.cpp` 상태 갱신 직후 전이 감지 | `RUNNING → AUG_SELECT`이면 `ActivateNextAugmentReward()` |
| 선택 화면 | `SceneAugment.cpp` | `enterT` 1.2초 등장, 66%부터 입력. 클릭 또는 1/2/3 + Space 확정 |
| 선택 종료 | `FinishCurrentAugmentReward`, `FinishAugmentEffects`, `applyAug` 람다 | 다음 큐 항목으로 이어지거나 RUNNING 복귀 + `postPickGrace = 0.5f` |

문제: 레벨업과 증강 선택의 타이밍이 분리되고, Space가 "증강 열기"와 "일시정지"를 겸해 오입력이 생긴다.

## 2. 목표 동작

1. 레벨 문턱을 넘는 고정 틱에서 **레벨업 전환**이 시작된다. 상태는 `RUNNING`을 유지한다.
2. 전환 동안 게임 시간 배율이 1.0 → 최저 배율로 감속한다(실시간 기준). 레벨업 연출이 함께 재생된다.
3. 감속이 끝나면 증강 선택(`AUG_SELECT`)이 자동으로 열리고 기존 등장 연출이 이어진다.
4. 선택 완료 후 RUNNING 복귀 시 배율은 1.0, 기존 선택 후 보호(`postPickGrace`)는 그대로 적용된다.
5. RUNNING에서 Space는 일시정지만 담당한다. `augReady`와 `[ SPACE ] 증강 픽업` 문구는 제거한다.

## 3. 함수화 원칙 (이 작업에 적용하는 규칙)

1. **새 상태 목록을 만들지 않는다.** 전환 상태는 증강 보상 상태의 소유자(`GameManager`의 보상 큐 묶음) 안에 두고, 기존 `ClearAugmentRewardQueue()`가 함께 초기화한다. 재시작·포기·새 런 경로에 초기화 줄을 추가하지 않는다.
2. **시간 배율 결정은 한 함수.** 스모크 배율, 히트스톱, 사망 슬로모, 레벨업 감속이 각각 `delta`를 고치는 대신 하나의 순수 함수가 게임플레이 delta를 계산한다.
3. **같은 동작의 진입점은 하나.** 레벨업 기록·보상 적재, 증강 선택 열기, 선택 후 RUNNING 복귀는 각각 하나의 함수만 사용한다. 모든 레벨업 경로는 디버프를 제외한 버프 후보만 사용한다. 디버그 경로의 즉시 열기는 기존 진입점으로 표현한다.
4. **순수 계산은 렌더러·전역에 의존하지 않는다.** 배율 곡선·전환 진행은 표준 라이브러리만 쓰고 assert 기반 C++ 테스트로 검증한다.
5. **리팩터링 계획의 다른 묶음 범위는 건드리지 않는다.** 키 반복·release 플래그 정리(DL-04), 3언어 선택식(DL-02), 런 상태 보존 목록(DL-14/EX-01)은 이 작업에서 새 반복을 늘리지 않는 선에서만 다루고, 본격 이관은 해당 묶음에 맡긴다.

## 4. 이 기능이 지나가는 기존 반복 코드와 함수화 대상

| ID | 현재 반복 | 함수화 방향 | 동작 변화 |
|---|---|---|---|
| LVU-R1 레벨업 기록·보상 적재 | 고정 틱 레벨업 루프와 창의적 F키가 각자 `RecordLevelUp` + `QueueAugmentReward` 수행 | `GrantLevelUpReward` 하나로 통일. 모든 출처에서 버프 선택만 큐에 넣음 | 없음 |
| LVU-R2 증강 선택 열기 | `ActivateNextAugmentReward()` 직접 호출: 상태 전이 감지, 창의적 F키, 스모크 `AUGSEL`, `FinishCurrentAugmentReward` | `OpenAugmentReward(AugmentOpenMode)` 진입점. `Immediate`(디버그·연속 선택) / `LevelUpTransition`(일반 레벨업). 실패 시 RUNNING 유지까지 함수가 처리 | 구조 단계에서는 `Immediate`만 존재 → 동작 동일 |
| LVU-R3 선택 후 RUNNING 복귀 | `currentState = RUNNING; postPickGrace = 0.5f`가 `FinishCurrentAugmentReward`, `FinishAugmentEffects`, `applyAug` 3곳에 반복 | `ReturnToRunningAfterPick()` + 보호 시간 명명 상수 | 없음 |
| LVU-R4 게임플레이 delta 계산 | `delta *= g_SmokeTimeScale`, `DYING ? delta*0.15`, 히트스톱 시 `physDelta = 0`이 루프 곳곳에 분산 | 순수 함수 `ComputeGameplayDelta(realDelta, GameplayTimeInputs)` (새 헤더 `Core/GameplayTime.h`). 우선순위: 히트스톱 0 > 사망 0.15 > 레벨업 배율 > 1.0, 스모크 배율은 실시간에 곱함 | 없음 |
| LVU-R5 이징 함수 | `SceneAugment.cpp`의 `TarotEaseOut`과 `AugSelectionEase`가 같은 cubic ease-out | `UiLayout.h`의 `Smoothstep` 옆에 `EaseOutCubic` 추가, 두 사본을 이관 후 삭제. 레벨업 감속 곡선도 이 함수 사용 | 없음 |
| LVU-R6 피격 보호 판정 | `ApplyPlayerDamageProtection` 호출부에서 `dashInvulnerability > 0 \|\| postPickGrace > 0`을 직접 조합 | `IsPlayerDamageProtected()`로 묶음. 레벨업 전환 보호는 이 함수에 조건 하나로 추가(결정 2 적용 시) | 없음 (R 단계) |

위 함수명은 가칭이다. 구현 시 리팩터링 계획의 규칙 소유자 설계와 이름 규칙을 확인해 확정한다. `main.cpp`의 레벨업 루프는 처치 보상 이관(DL-01, 2단계) 바로 뒤에 있으므로, 같은 구간을 동시에 크게 바꾸지 않도록 작업 시작 전 해당 파일의 다른 변경을 확인한다.

## 5. 새 동작 구현 (함수 확장만으로)

- **전환 상태:** `Core/LevelUpTransition.h` — `Begin()`, `Cancel()`, `Active()`, `Update(realDelta)`(종료 시 1회 `true`), `TimeScale()`. 렌더러·전역 의존 없음. 인스턴스는 `GameManager` 보상 큐 묶음에 두고 `ClearAugmentRewardQueue()`가 `Cancel()`한다.
- **진입:** `GrantLevelUpReward`(R1)가 일반 레벨업이면 `OpenAugmentReward(LevelUpTransition)`(R2)을 요청한다. 이미 전환 중이거나 보상이 열려 있으면 큐 적재만 한다.
- **진행:** 메인 루프는 RUNNING에서 `Update(realDelta)`를 호출하고, 종료 시 `OpenAugmentReward(Immediate)`를 호출한다. 감속은 `ComputeGameplayDelta`(R4)의 입력으로 `TimeScale()`을 넘기는 것으로만 반영한다.
- **Space 제거:** `HandleInput`의 `augReady` 분기, `augReady` 필드, HUD `kAugHint`, 상태 전이 감지 코드를 삭제한다(삭제만 있고 추가 없음).
- **사격 중 오선택 방지:** 선택 화면의 기존 `inputReady` 조건에 "열린 뒤 좌클릭 해제를 한 번 확인" 조건을 더한다. 새 release 정적 변수를 만들지 않고 기존 `c.lmb` / `g_LmbPrev`를 사용하며, 상태는 `augmentSelectionSerial`이 바뀔 때 함께 초기화되는 기존 정적 묶음에 둔다.
- **연출:** 레벨업 링은 기존 `DrawVisibleConstellLine` 계열 그리기 함수를 사용한다(선만, 점선·배경 판 없음).

## 6. 경계 상황

| 상황 | 처리 |
|---|---|
| 한 틱에 여러 레벨 상승 | 전환은 한 번. 이후 큐 항목은 `FinishCurrentAugmentReward`에서 `Immediate`로 이어진다. |
| 전환 중 추가 레벨업 | 큐 적재만. 전환을 다시 시작하지 않는다. |
| 전환 중 사망 | `DYING` 우선. 전환은 `Cancel()`. 보상 큐는 기존 사망 처리에 맡긴다. |
| 전환 중 ESC·포커스 이탈 | PAUSED에서는 `Update`를 호출하지 않으므로 타이머가 멈추고, 재개 시 이어간다. |
| 히트스톱과 동시 발생 | 전환 타이머는 실시간으로 진행. 감속 시간은 최대 히트스톱(0.14초)보다 충분히 길게 둔다. |
| 재시작·포기·메뉴 | `ClearAugmentRewardQueue()` 한 곳에서 전환 해제. |
| 레벨 상한 45 | 레벨업이 없으므로 전환도 없다. |
| 창의적 F키·스모크 `AUGSEL` | `OpenAugmentReward(Immediate)` — 버프 카드만 선택. |

## 7. 결정 필요 (권장값)

1. **감속 수치** — 감속 0.45초 + 최저 배율 유지 0.20초, 최저 배율 0.15.
2. **전환 중 피격** — 무적 권장(`IsPlayerDamageProtected`에 조건 추가). 느려진 구간의 피해는 대응할 수 없는 피해가 된다.
3. **연속 레벨업마다 감속 반복** — 아니오 권장. 첫 번째만 감속.
4. **레벨업 연출 강도** — 선으로 그린 확장 링 + 가장자리 비네트 정도로 제한.

## 8. 작업 순서와 완료 조건

구조(R) 커밋은 동작을 바꾸지 않고, 동작(B) 커밋은 구조를 새로 만들지 않는다.

| 순서 | 작업 | 완료 조건 |
|---|---|---|
| 1 | **R3** 선택 후 복귀 함수화 | `postPickGrace = 0.5f` 직접 대입이 리셋 경로 외에 남지 않음. 기존 증강 선택·디버프·교체 흐름 동일 |
| 2 | **R5** `EaseOutCubic` 공용화 | `SceneAugment.cpp`의 두 사본 삭제. 선택 화면 스모크 캡처가 변경 전과 같음 |
| 3 | **R4** `ComputeGameplayDelta` + 단위 테스트 | 히트스톱·사망 슬로모·스모크 배율 조합 테스트 통과. 루프에서 `physDelta`를 직접 조정하는 분기가 사라짐 |
| 4 | **R6** `IsPlayerDamageProtected` | 대시 무적·선택 후 보호 동작 동일 |
| 5 | **R1** `GrantLevelUpReward` | 레벨업 루프·F키가 같은 함수 사용. `test_experience.ps1`, `test_experience_smoke.ps1` 통과, 텔레메트리 `LEVEL_UP` 기록 동일 |
| 6 | **R2** `OpenAugmentReward(Immediate)` | `ActivateNextAugmentReward()` 직접 호출이 함수 내부 외에 남지 않음. 스모크 `AUGSEL` 동작 동일 |
| 7 | **B1** `LevelUpTransition` + 단위 테스트 + 연결 | 배율 곡선(1.0 시작, 단조 감소, 최저값 도달), 종료 이벤트 1회, `Cancel` 후 1.0, 0/음수/큰 delta. 레벨업 → 감속 → 자동 진입, §6 표 전 항목 |
| 8 | **B2** Space 경로 삭제 | `augReady` 참조 0건. RUNNING에서 Space는 항상 일시정지 |
| 9 | **B3** 좌클릭 해제 대기 + 레벨업 링 연출 | 좌클릭을 누른 채 진입해도 선택되지 않음. 디자인 원칙 준수 |
| 10 | **B4** 문구·문서 | `TutorialText.h` 3개 언어에서 "레벨업 시 자동으로 선택 화면이 열림"으로 갱신(1·2·3 선택·Space 확정 안내는 유지). `GameManager.h` 주석, 리팩터링 계획의 후보 목록에 R1~R6 처리 결과 기록 |

새 헤더(`GameplayTime.h`, `LevelUpTransition.h`)와 테스트 `.cpp`는 Visual Studio 프로젝트와 CMake 소스 목록을 모두 갱신한다.

## 9. 검증

- **단위:** R4·B1 테스트, 기존 `test_experience.ps1`, `test_reg04_run_reset.ps1`(재시작 시 전환·큐 초기화), `test_reg02_03_stat_builds.ps1`, `test_reg01_kill_rewards.ps1`(레벨업 루프 인접 구간 회귀).
- **스모크:** XP 지급으로 레벨업을 일으켜 일정 프레임 뒤 `AUG_SELECT` 진입과 그 사이 배율 < 1 구간 확인. 큰 픽업으로 다중 레벨업 시 선택 화면이 레벨 수만큼 이어지는지 확인.
- **수동 플레이:** 일반 레벨업, 엘리트 처치 동시 레벨업, 전환 중 ESC·Alt+Tab, 전환 중 피격, 사격을 누른 채 진입, Lv45 상한, 창의적 F키.
- **잔존 검색:** `augReady`, `ActivateNextAugmentReward(` 직접 호출, `postPickGrace = 0.5f`, `delta * 0.15f`, `TarotEaseOut`/`AugSelectionEase`가 의도한 곳 외에 남지 않는지 확인.
- Windows Release 빌드. macOS CI는 로컬 변경 업로드 여부에 따라 별도 기록한다.

## 10. 구현·검증 기록 (2026-10-13)

브랜치 `feat/levelup-auto-augment`. §7 결정은 모두 권장값으로 확정했다(감속 0.45초 + 유지 0.20초, 최저 0.15배, 전환 중 무적, 연속 레벨업은 첫 번째만 감속, 선 링만 사용·비네트 없음).

| 작업 | 결과 |
|---|---|
| R3 | `ReturnToRunningAfterPick()` + `POST_PICK_GRACE`(PlayerRuntime.h). 3곳 이관 |
| R5 | `EaseOutCubic` 공용화 후 std 전용 `Render/Easing.h`로 이동(`UiLayout.h`가 포함). 장면의 두 사본 삭제 |
| R4 | `Core/GameplayTime.h`의 `ComputeGameplayDelta`. 히트스톱·사망 슬로모를 입력으로 통일. `test_gameplay_time.ps1` |
| R6 | `IsPlayerDamageProtected()` |
| R1 | `GrantLevelUpReward(…, LevelUpSource)`. 레벨업 루프·F키 공용 |
| R2 | **생략.** `GameManager::ActivateNextAugmentReward()`가 이미 단일 진입점이며, 감싸는 함수는 차이를 표현할 게 없어 추가하지 않았다. Space 경로의 전이 감지 호출은 B2에서 삭제됐다 |
| B1 | `Core/LevelUpTransition.h`(GameManager 보상 큐 묶음 소유, `ClearAugmentRewardQueue`·`ActivateNextAugmentReward`가 해제). 고정 틱과 RUNNING 프레임 블록(재생·스폰·사격·궤도체·게임 시계) 모두 `GameplayTime.h` 배율을 사용. 히트스톱은 기존대로 고정 틱만 멈춤 |
| B2 | `augReady`, `[ SPACE ] 증강 픽업` HUD, RUNNING→AUG_SELECT 전이 감지 삭제. RUNNING에서 Space는 일시정지 |
| B3 | 링 연출: 기존 `DrawSettingsOrbitArc`로 XP 색 선 링 2개. `SpawnShockWave`는 현재 링 렌더러가 없어(배경 가림만 남음) 재사용하지 않았다. **좌클릭 해제 대기는 넣지 않았다** — 선택 화면 클릭은 이미 `c.lmb && !g_LmbPrev` 엣지 판정이고 `g_LmbPrev`는 매 프레임 갱신되므로, 사격 버튼을 누른 채 진입해도 확정되지 않는다. 연타 오선택은 기존 `inputReady`(등장 66%) 지연이 막는다 |
| B4 | 튜토리얼 3개 언어 증강 페이지 갱신(문장을 `/`로 나눠 글자 축소 방지) |

검증: Windows Release 빌드, `test_gameplay_time.ps1`, `test_level_up_transition.ps1`(곡선 단조 감소·최저값 유지·1회 종료·Cancel·0/음수/큰 delta), `test_levelup_auto_augment.ps1`(레벨업 직후 RUNNING+전환 중 → 약 0.65초 뒤 입력 없이 AUG_SELECT, 3레벨 동시 상승 시 보상 3개 대기), `test_experience.ps1`, `test_experience_smoke.ps1`, `test_reg04_run_reset.ps1`, `test_reg02_03_stat_builds.ps1`, `test_reg06_skill_reset.ps1`, `test_kill_lifesteal.ps1`, `test_thrown_swarm_kill.ps1`, `test_quasar_nova.ps1` 통과. 스모크 상태 덤프에 `rewards=`(보상 큐 길이)·`levelup=`(전환 중) 필드를 추가했다. 스모크 캡처로 링 표시와 자동 진입 화면을 확인했다. 전환 중 게임 시계는 약 0.65초 실시간 동안 0.1초만 진행했다.

미검증: 수동 플레이(엘리트 처치 동시 레벨업의 체감, 전환 중 ESC·Alt+Tab, 전환 중 피격 체감, Lv45 상한, 창의적 F키), macOS 빌드.

## 11. 슬로모션 제거 (2026-10-13)

플레이 피드백: 감속이 연출보다 **렉처럼 느껴진다.** §7의 결정 1(감속 수치)을 철회하고 감속 없이 다시 설계했다.

- 레벨업 순간부터 **0.35초 동안 정상 속도로 진행**하며 XP 링 신호를 재생한다. 이 구간은 피격 보호가 유지된다. 이후 선택 화면이 자동으로 열린다. 선택 화면 등장 연출(`enterT`)부터는 기존처럼 게임이 멈춘 채 표시된다.
- `LevelUpTransition`은 배율 없는 신호 타이머가 됐다(`DURATION`, `Update`, `Progress`). `GameplayTime.h`의 `levelUpScale` 입력과, RUNNING 프레임 블록의 `runDelta` 분리를 되돌렸다. 시간 배율 소유는 R4 상태(히트스톱·사망 슬로모)로 돌아갔다.
- 튜토리얼 3개 언어에서 "시간이 느려진다" 문구를 제거했다.
- `test_levelup_auto_augment.ps1`: 신호 구간에서 게임 시계가 정상 속도로 흐르는지 확인한다. `test_experience_smoke.ps1`: 앞 단계의 레벨업이 자동으로 선택 화면을 열어 상한 단계의 XP 처리가 멈추므로, 상한 단계 전에 `RUN`으로 새 런을 시작하도록 고쳤다.

## 후속 변경 기록 (2026-10-13)

웨이브 리팩터링에 따라 레벨업에서 디버프 단계를 제거했다. `QueueAugmentReward`는 버프 선택만 적재하며, 선택 완료 후 디버프 선택으로 이어지는 경로와 전용 추첨기를 삭제했다. 모든 레벨업 출처에서 디버프 등급을 후보에서 제외한다. 웨이브 경계의 디버프는 별도 웨이브 계획에서 자동 지급한다.
