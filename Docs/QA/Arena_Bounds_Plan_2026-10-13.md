# 맵(아레나) 경계·카메라 줌 단일화 + 안타레스 벽 충돌 계획

2026-10-13 기준. 점수에 따라 넓어지는 맵의 경계를 **한 곳에서 계산하고 모든 시스템이 같은 값을 읽는 구조**로 바꾼다. 줌아웃 상태에서 생기는 화면 뒤틀림(§1A)을 같은 작업 안에서 정리한다. 그 위에서 안타레스가 돌진 중 맵 가장자리에 부딪히면 기절하도록 한다.

**상태: 구현 완료 (2026-10-13), 브랜치 `feat/arena-bounds`.** 결과는 §9. 결정 D-01~D-09는 모두 권장값으로 진행했다.

이 작업은 [중복 리팩터링 계획](Duplication_Refactor_Plan_2026-10-12.md)과 동시에 진행한다. 리팩터링 계획의 **DL-07**(`ConstrainGenesis/Gimbal/RegulusToViewport` 범위 계산 반복)을 이 계획이 흡수한다. 구조 이관(R)과 동작 변경(B)은 리팩터링 계획 §8.1에 따라 별도 커밋·별도 검증으로 나눈다.

## 0. 선행 정리: 현재 HEAD 빌드 상태

안타레스 벽 기절의 초안이 이미 작업 트리에 있다. 그런데 두 파일이 서로 다른 상태다.

- `main.cpp`의 `ConfineAntares` 호출 3줄은 동시 진행 중인 리팩터링 커밋 `675ab8c (LVU-R4)`에 **함께 커밋됐다**.
- `Monster.h`의 `ConfineAntares` 본문과 `antaresInArena` 필드는 **아직 미커밋**이다.
- 따라서 HEAD 단독으로는 빌드되지 않는다. 작업 트리 전체로는 Release x64 빌드가 성공했다.

이 초안은 아래 R4/B2에서 공통 경계 경로로 대체된다. 처리 방식은 §7 결정 D-01로 정한다.

## 1. 현재 구조 (조사 결과)

맵은 별도 크기가 없다. **카메라 줌아웃으로 보이는 영역이 곧 맵**이다. 줌 목표값은 점수에서 바로 계산된다.

| 위치 | 하는 일 | 문제 |
|---|---|---|
| `main.cpp:1279-1280` | `t = min(1, score / 2000000)`, `zoomTarget = 1 - 0.286 * t` | 성장 곡선의 상수가 코드 안에 박혀 있음. 이름·출처 없음 |
| `main.cpp:1289-1290` | `g_ArenaExX/Y = screen * 0.5 * (1/zoom - 1)` | 줌에서 확장량을 다시 계산해 전역에 저장 |
| `main.cpp:2262` `GetPlayerViewportBounds` | 화면 중앙·줌·상하 inset으로 플레이어 영역 계산 후 클램프 | 맵 계산 경로 ① |
| `main.cpp:2342` 대시 시작 | `ccY + (BROWSER_CHROME_H - ccY) / zoomNow`로 상단 한계를 **직접 다시 계산** | ①과 같은 식의 복사 |
| `main.cpp:2382-2384` 넉백 후 클램프 | 같은 상단 식을 또 직접 계산, 좌우는 `ccX ± halfW` | ①과 같은 식의 복사 |
| `main.cpp:2537-2540` 몹 가두기 | `ScreenToWorldX/Y(0, screen)`로 경계 계산 | 맵 계산 경로 ② (카메라 역변환) |
| `Monster.h:1140-1186` | Genesis/Gimbal/Regulus가 각자 `minX/maxX…` 범위 계산 | DL-07. 같은 범위식 3벌 + 안타레스 초안 1벌 |
| `RangedMob.h:92-93` `pickNewTarget` | `-g_ArenaExX … screenW + g_ArenaExX` | 맵 계산 경로 ③ (전역 확장량) |
| `ProjectileSystem.cpp:89-92` | `screen * 0.5 * (1/zoom - 1) + 200` | `g_ArenaEx` 식의 복사 |
| `MonsterManager.h:38-47` `EdgePoint` + `EnemySpawner.cpp:126-127` | 스폰 영역 = **줌을 반영하지 않은** `0..screenWidth`, 바깥 150px | 맵이 넓어진 것을 모름 |
| `EnemySpawner.cpp:79-82` Genesis 앵커 | `160 + rand(screenWidth - 320)` | 줌을 반영하지 않은 화면 기준 |
| `main.cpp` "Large controller enemies" 루프 (Gravis/Quasar) | `margin..screenWidth - margin`으로 가두고 표류 방향 반사 | 맵 계산 경로 ④. **줌을 반영하지 않은 원래 화면**에 가둔다. 넓어진 맵에서 보이지 않는 벽처럼 동작한다. Regulus와 같은 반사 코드가 한 벌 더 있다 |
| `main.cpp` `ApplySingleAugIdx`의 접근 오브(`D_APPROACH`) 생성 | `0..scrW` 가장자리 ±40px | 줌을 반영하지 않아 최대 줌아웃 시 보이는 맵 안에서 생성 |

줄 번호는 작성 시점 기준이다. 동시에 진행 중인 리팩터링으로 `main.cpp` 줄 번호가 계속 바뀌므로, 구현할 때는 함수명과 주석으로 다시 찾는다.

`g_ZoomCX/CY`는 0으로 초기화하는 코드만 있고 0이 아닌 값을 넣는 곳이 없다. 그래서 현재는 경로 ①②③이 같은 사각형을 가리킨다. 하지만 같은 사실을 서로 다른 세 가지 식으로 관리하고 있다.

### 이 구조 때문에 생기는 버그

1. **스폰이 보이는 맵 안에서 일어난다.** 최대 줌아웃(0.714)에서 맵은 한쪽으로 `screenWidth × 0.2`만큼 넓어진다(1920px 기준 약 384px). 스폰 여백은 150px뿐이라 몹이 화면 안에서 갑자기 나타난다.
2. **Genesis 앵커가 넓어진 맵의 중앙 쪽에만 몰린다.** 앵커 범위가 원래 화면 크기에 고정돼 있다.
3. **안타레스가 맵 밖으로 나간다.** 경계 처리 대상에서 빠져 있다. 사용자 보고 이슈다.
4. **Gravis/Quasar는 넓어진 맵을 쓰지 못한다.** 원래 화면 크기 안에서 반사된다.
5. **접근 오브가 보이는 맵 안에서 생긴다.** 스폰과 같은 원인이다.
6. 성장 곡선이나 화면 inset을 바꾸면 위의 여러 곳을 사람이 맞춰야 한다.

## 1A. 줌아웃 상태의 UI·렌더 뒤틀림 점검 (2026-10-13)

과거 폴리모프 2페이즈 줌에서 생긴 뒤틀림은 세 종류였다(`216d9d8`, `e6f1fcd`, `1eecaac`).

- UI가 줌된 투영으로 그려져 줄어들고 밀림
- 월드 물체에 붙은 글자가 물체에서 떨어짐
- 상태를 바꿀 때 줌이 풀림

현재 코드를 이 세 종류 기준으로 정적 점검했다. **실제 줌아웃 화면 캡처는 아직 하지 않았다.** 스모크 캡처에 점수나 줌을 지정하는 단계가 없어서다(§5에 추가).

### 문제 없음 (코드로 확인)

| 항목 | 근거 |
|---|---|
| HUD·메뉴·장면 UI 위치와 크기 | 월드 패스가 끝나면 `g_BaseOrtho`로 되돌린 뒤 HUD를 그린다. 글자는 `TextRenderer`가 항상 자체 화면 투영을 쓴다 |
| 크로스헤어 | 화면 마우스 좌표 `mx, my`로 화면 패스에서 그린다 |
| 조준·이동 입력 | `ScreenToWorldX/Y`로 변환한다 |
| 피해 숫자 위치 | `W2SX/W2SY`로 변환하고 `g_ViewZoom`을 곱한다 |
| 아이콘·광원 원판 | `ProjectWorldDisc`로 월드 크기를 유지하고 카메라가 줌을 한 번만 적용한다 |
| 화면 테두리 비네트·플래시·피격 비네트 | 기본 투영으로 화면 전체에 그린다 |

### 뒤틀림 있음 (코드로 확인, 캡처로 재확인 필요)

| ID | 증상 | 원인 |
|---|---|---|
| UI-Z1 | **사망 순간 화면이 원래 크기로 확 줌인했다가 다시 줌아웃된다.** 맵 가장자리에서 죽으면 폭발 위치가 순간적으로 화면 밖이 될 수 있다 | 사망 처리에서 `g_ViewZoom = g_ViewZoomTarget = 1.0f`로 즉시 되돌린다(폴리모프 시절 코드). 그런데 DYING은 줌 유지 상태 목록에 있어서, 다음 프레임부터 점수 기준 줌으로 다시 보간된다 |
| UI-Z2 | **일시정지 → 설정, 증강 교체(`AUG_REPLACE`), 게임오버에서 뒤 배경 월드가 원래 크기로 확 줌인된다.** 일시정지로 돌아오면 다시 줌아웃된다 | "줌 유지 상태" 목록(RUNNING, DYING, PAUSED, AUG_SELECT, DEBUFF_SELECT)과 "월드를 그리는 상태" 목록(앞의 것 + READY, AUG_REPLACE, GAMEOVER, 일시정지에서 연 SETTINGS)이 따로 관리되어 서로 다르다. 블러 상태 목록과 화면 흔들림 상태 목록도 각각 따로 있다. 같은 사실을 네 곳에서 관리한다 |
| UI-Z3 | **치명타 피해 숫자의 "치명타/CRIT" 라벨이 줌아웃해도 줄지 않는다.** 숫자만 줄어 크기 비율이 어긋난다 | 숫자 크기에는 `g_ViewZoom × UiScale`을 곱하지만 라벨 크기에는 곱하지 않는다. 피해 숫자 크기 계산이 두 줄에 따로 있다 |
| UI-Z4 | 메뉴 등 월드를 그리지 않는 상태에서도 줌·흔들림이 들어간 투영이 그대로 남는다. 지금은 그 상태에서 줌이 1.0으로 고정되어 드러나지 않을 뿐이다 | 기본 투영 복원이 월드 렌더 블록 안에만 있다. UI 정확성이 "줌이 1.0일 것"이라는 다른 코드의 부수효과에 기대고 있다 |

### 정책 확인 필요

- 플레이어 HP/XP 원형 바와 무기 셸은 월드 좌표로 그려진다. 그래서 최대 줌아웃에서 71%로 작아진다. 플레이어 몸체와 같은 비율이라 의도된 동작으로 보이지만, 가독성 기준으로 화면 고정 크기를 원하는지 확인이 필요하다(D-07).
- 피해 숫자는 화면 흔들림을 따르지 않는다. 그래서 흔들릴 때 몹 위치와 잠깐 어긋난다. 줌과는 무관한 기존 동작이라 이번 범위에서 제외한다.

## 2. 원칙

1. **맵 사각형의 출처는 하나.** 고정 틱마다 한 번 계산하고, 소비자는 값을 인자로 받는다. 새 전역 확장량을 만들지 않고 `g_ArenaExX/Y`는 제거한다.
2. **성장 곡선은 명명된 정의.** 최대 확장 배율과 도달 점수를 상수로 두고, 줌 목표는 순수 함수가 계산한다. 이번 작업에서 수치는 바꾸지 않는다.
3. **순수 계산은 렌더러·전역에 의존하지 않는다.** 사각형·inset·클램프·성장 곡선은 표준 라이브러리만 쓰고 assert 기반 C++ 테스트로 검증한다(`GameplayTime.h`와 같은 방식).
4. **의도된 차이는 명명된 정책으로 남긴다.** 다음 차이는 없애지 않고 정책으로 표현한다.
   - 플레이어는 HUD inset(상단 크롬, 하단 게임바·작업표시줄)을 제외한 영역을 쓴다.
   - 몹별 여백(몸체·광원 크기)은 다르다.
   - 경계에 닿았을 때 반응은 몹마다 다르다. 단순 정지, Regulus 반사, Genesis 앵커 보정, 안타레스 기절.
5. **콘텐츠 병행 규칙.** 통합 이후 새 몹은 공통 경계 함수와 정책만 확장한다. 범위식을 복사하지 않는다(리팩터링 계획 §8.3).

## 3. 설계

### 3.1 `Core/ArenaBounds.h` (신규, 순수)

가칭이며 구현 시 리팩터링 계획의 이름 규칙에 맞춰 확정한다.

- `struct ArenaRect { float left, top, right, bottom; }`
  - `Inset(margin)`: 여백만큼 줄인 사각형. 맵이 여백의 두 배보다 작으면 중앙으로 수렴한다. 현재 3벌의 `min(margin, half)` 처리를 그대로 옮긴다.
  - `Contains(x, y)`, `Clamp(x, y)`: 클램프 결과로 X/Y 각각 경계에 닿았는지(`ArenaHit`) 돌려준다.
  - `Expand(padding)`: 스폰·탄환 소거에 쓰는 바깥 사각형.
- `struct ArenaGrowth { float maxScale; float fullScore; }` — 현재 값은 `{ 1.4f, 2000000.0f }`. 0.714와 0.286은 이 값에서 계산한다.
- `float ArenaZoomForScore(int score, const ArenaGrowth&)`: 줌 목표값.
- `ArenaRect ComputeArenaRect(float screenW, float screenH, float zoom)`: 카메라가 보는 월드 사각형.
- `ArenaRect PlayerArea(const ArenaRect&, float zoom, float topInset, float bottomInset)`: HUD inset을 반영한 플레이어 영역.

### 3.2 소유와 흐름

```text
main 고정 틱 시작
  └─ arena = ComputeArenaRect(screen, zoomNow)        ← 유일한 계산
       ├─ PlayerArea(arena, …) → 이동/대시/넉백 클램프
       ├─ MonsterManager::UpdateAll(…, arena)
       │    ├─ RangedMob::pickNewTarget(arena.Inset(80))
       │    └─ 경계 처리 (3.3)
       ├─ EnemySpawner(…, arena) → EdgePoint(arena.Expand(150)), Genesis 앵커(arena.Inset(160))
       └─ UpdateProjectiles(…, arena) → arena.Expand(200) 밖 탄환 소거
```

`GetPlayerViewportBounds` / `PlayerViewportBounds`는 `PlayerArea`로 대체한다. 대시 시작에는 같은 사각형을 넘긴다. 접근 오브 생성도 `arena.Expand(40)`을 쓴다.

### 3.2A 카메라 상태와 투영 (UI-Z1, Z2, Z4)

- **"런 화면" 판정은 하나.** `IsRunViewState(state)` 하나가 줌 유지와 월드 렌더를 함께 결정한다. 일시정지에서 연 설정 화면처럼 이전 상태가 필요한 경우도 이 함수가 처리한다.
  - 블러 배경과 화면 흔들림은 이 판정 위에서 명명된 하위 정책으로 표현한다. 예: 흔들림은 런 화면 중 모달이 없는 상태만.
  - 서로 다른 목록 네 개를 두지 않는다.
- **줌 목표는 한 함수.** 런 화면이면 `ArenaZoomForScore(score)`, 아니면 1.0을 쓴다. 사망 처리의 즉시 1.0 대입은 삭제한다. 사망 중에도 줌을 유지하며, 결과 화면으로 나갈 때만 1.0이 된다.
- **투영 전환은 두 함수.** `BeginWorldPass()`는 줌과 흔들림이 적용된 투영, `BeginScreenPass()`는 기본 투영을 건다. 화면 패스 시작은 월드 렌더 여부와 상관없이 항상 `BeginScreenPass()`를 호출한다. `g_MainOrtho` 직접 `memcpy` 3곳(`main.cpp` 2곳, `ScenePause.cpp` 1곳)을 이 함수로 바꾼다.
- **월드에 붙는 글자 크기는 한 함수.** `WorldTextScale(base)`는 `g_ViewZoom × UiScale`을 적용한다. 피해 숫자와 치명타 라벨이 같이 쓴다(UI-Z3).

### 3.3 몹 경계 처리 단일화 (DL-07)

- `Monster::ArenaMargin()`: 몹 종류별 여백 정의 한 곳. 현재 값을 그대로 옮긴다.
  - Genesis: `base * 2.75 + 8`
  - Gimbal: `34 * sizeScale`
  - Regulus: `base * 2.12 + 14`
  - Gravis: `92`, Quasar: `76` (현재 main 루프의 값)
  - Antares: `26 * sizeScale` (프레임 가장자리. `AntaresRamRange`와 같은 기준)
- 표류 방향 반사(Regulus·Gravis·Quasar 세 벌)는 `ReflectHeading(angle, ArenaHit)` 하나로 묶는다.
- `Monster::ConstrainToArena(const ArenaRect&)`: `arena.Inset(ArenaMargin())`로 클램프하고, 닿았으면 종류별 반응을 호출한다. 기존 세 함수와 main의 루프를 대체한다.
  - Genesis: 앵커도 같은 사각형으로 클램프
  - Regulus: 닿은 축의 표류 각도 반사
  - Antares: §3.4
  - 경계 처리 대상이 아닌 종류는 아무것도 하지 않는다. 대상 목록도 이 함수 안의 정의 하나로 관리한다.
- 진입 정책: Genesis/Gimbal/Regulus는 현재처럼 **즉시** 맵 안으로 들어온다. Antares는 **맵 안에 완전히 들어온 뒤부터** 가둔다. 스폰 직후 붙잡혀 기절하지 않게 하기 위해서다. 이 차이는 명명된 진입 정책 값으로 표현한다.
- 호출 시점은 현재 `CollisionSystem::Update` 다음 위치를 유지한다. 몰림·당김·무기 넉백 이후에 가둔다. 초안처럼 몹 업데이트 직후에 별도 루프를 두지 않는다.

### 3.4 안타레스 벽 충돌 (동작 추가)

- 맵 안에 들어온 뒤 CHASE/SKID 중 경계에 닿으면, 그 자리에서 멈추고 `STUNNED`로 전환한다. 기절 시간은 램 기절과 같은 `ANTARES_STUN_TIME`이다.
- 1단계 속도(200 px/s 미만)는 기절 없이 벽을 따라 미끄러진다. 플레이어가 벽에 붙어 있을 때 기절이 반복되지 않게 하기 위해서다(D-03).
- 기절·휴식 상태에서는 위치 클램프만 한다.
- 기존 초안 `ConfineAntares`, `antaresInArena`, main의 별도 호출 루프는 이 경로로 대체한 뒤 삭제한다.

## 4. 작업 순서와 완료 조건

구조(R) 커밋은 동작을 바꾸지 않고, 동작(B) 커밋은 구조를 새로 만들지 않는다.

| 순서 | ID | 작업 | 완료 조건 |
|---|---|---|---|
| 0 | FIX | D-01에 따라 HEAD 빌드 복구 | HEAD 단독 Release 빌드 성공 |
| 0.5 | ARN-T | 스모크 단계 `SCORE=<n>` 추가. 점수와 줌 목표를 설정하고 줌을 목표값으로 바로 맞춘다 | 줌아웃 상태의 HUD·일시정지·설정·증강 교체·사망 캡처가 가능. **변경 전 기준 캡처를 먼저 저장**해 UI-Z1~Z3 재현을 확인 |
| 1 | ARN-R1 | `ArenaBounds.h` + 단위 테스트 + `test_arena_bounds.ps1` | 아래 §5 단위 항목 통과. vcxproj·filters·CMake 갱신 |
| 2 | ARN-R2 | 성장 곡선을 `ArenaGrowth` / `ArenaZoomForScore`로 이관 | main의 `2000000`, `0.286` 직접 사용 0건. 점수별 줌 목표가 이전과 같음(테스트로 확인) |
| 3 | ARN-R3 | 고정 틱 `arena` 1회 계산. 플레이어 이동·대시·넉백, RangedMob, 탄환 소거를 이관하고 `g_ArenaExX/Y` 제거 | `g_ArenaEx`, `ccY + (BROWSER_CHROME_H - ccY) / zoomNow`, `0.5f * (1.0f / safeZoom - 1.0f)` 잔존 0건. 플레이어 이동 한계와 탄환 소거 위치가 이전과 같음 |
| 4 | ARN-R4 | `ArenaMargin` + `ConstrainToArena` + `ReflectHeading`으로 Genesis/Gimbal/Regulus 이관, 기존 3함수 삭제. Gravis/Quasar 루프도 같은 함수로 옮기되 **기존 사각형(원래 화면)을 인자로 넘겨** 동작을 유지한다. `tests/mob_death_fx_test.cpp`의 Gimbal 호출 갱신 | `Constrain*ToViewport` 0건, main의 Gravis/Quasar 전용 루프 0건. 다섯 몹의 경계 동작이 이전과 같음 |
| 5 | ARN-R5 | `IsRunViewState`, `BeginWorldPass` / `BeginScreenPass`, `WorldTextScale` 추출. 기존 상태 목록과 투영 `memcpy`를 이관(목록 차이는 그대로 보존) | 네 상태 목록과 `g_MainOrtho` 직접 `memcpy`가 함수 내부 외에 0건. 변경 전 기준 캡처와 동일 |
| 6 | ARN-B1 | 스폰 `arena.Expand(150)`, Genesis 앵커 `arena.Inset(160)`, 접근 오브 `arena.Expand(40)`, Gravis/Quasar를 실제 맵 사각형으로 변경 | 최대 줌아웃에서도 몹·오브가 화면 밖에서 진입. 앵커와 Gravis/Quasar가 넓어진 맵 전체를 사용 |
| 7 | ARN-B2 | 안타레스를 `ConstrainToArena`에 편입하고 §3.4 동작 적용. 초안 코드 삭제 | `ConfineAntares`, `antaresInArena` 0건. §6 경계 상황 전 항목 |
| 8 | ARN-B3 | 줌 유지를 `IsRunViewState` 기준으로 변경(UI-Z2), 사망 시 즉시 1.0 대입 삭제(UI-Z1), 화면 패스 시작에서 항상 기본 투영(UI-Z4), 치명타 라벨에 `WorldTextScale` 적용(UI-Z3) | 같은 점수로 PAUSED↔SETTINGS, AUG_REPLACE, 사망→게임오버를 지날 때 월드 크기가 변하지 않음. 라벨과 숫자 비율이 줌과 무관하게 일정. 기준 캡처와 비교해 HUD 위치 동일 |
| 9 | ARN-D | 문서 | 리팩터링 계획 DL-07을 "ARN-R4로 완료"로 갱신, 상태 목록 통합을 리팩터링 후보 목록에 기록, 도감·튜토리얼에 안타레스 벽 기절 설명이 필요한지 확인 |

## 5. 검증

- **단위(`ArenaBounds`):**
  - 줌 1.0/0.714에서 사각형 크기
  - inset이 반폭보다 클 때 중앙 수렴
  - 클램프 결과와 X/Y 충돌 플래그
  - 점수 0, 1,000,000, 2,000,000, 초과 점수에서의 줌 목표(기존 식과 일치)
  - 0과 음수 줌 방어(현재 `0.01` 하한 유지)
- **단위(안타레스):** 맵 밖 스폰 → 진입 전에는 미적용, 진입 후 고속 충돌 → STUNNED와 위치 클램프, 1단계 접촉 → 미끄러짐, 기절 중 클램프만. 가능하면 `Monster`를 직접 생성하는 기존 `mob_death_fx_test.cpp` 방식을 쓴다.
- **줌아웃 UI 캡처 (ARN-T):** `smoke_ui.ps1`로 `SCORE=0`과 `SCORE=2000000` 각각에서 다음을 KR/EN/JP와 작은 창(`-Size`)으로 찍는다.
  - RUN HUD, 피해 숫자(치명타 포함)
  - PAUSED, 일시정지에서 연 SETTINGS
  - AUGSEL, 증강 교체, DYING 진행 중, GAMEOVER

  ARN-R 단계는 변경 전 캡처와 같아야 하고, ARN-B3 이후에는 UI-Z1~Z3 증상이 사라져야 한다.
- **회귀:** `test_mob_death_fx.ps1`, `test_genesis_launcher.ps1`, `test_regulus_beam.ps1`, `test_gameplay_time.ps1`, Windows Release 빌드.
- **수동 플레이:**
  - 점수 0과 최대 줌아웃 상태 각각에서 플레이어가 네 변 끝까지 이동, 대시, 넉백
  - 원거리 몹 목표점 분포
  - 스폰 진입 위치
  - Genesis 앵커 위치
  - Regulus 반사
  - 안타레스 고속 돌진으로 벽 충돌, 플레이어가 벽에 붙은 상태에서의 추격
- **잔존 검색:**
  - 맵 경계: `g_ArenaEx`, `2000000`, `0.286`, `ScreenToWorldX(0`, `Constrain\w*ToViewport`, `ConfineAntares`, `GetPlayerViewportBounds`, `(1.0f / safeZoom - 1.0f)`, `rand() % scrW`
  - 카메라·투영: `memcpy(g_MainOrtho`, `g_ViewZoom = 1.0f`, `g_ViewZoomTarget = 1.0f`(줌 목표 함수 외부)

## 6. 경계 상황

| 상황 | 처리 |
|---|---|
| 줌아웃이 진행 중(목표로 보간 중) | 매 틱 현재 줌으로 계산하므로 맵이 부드럽게 커진다. 경계 몹은 넓어진 쪽으로 자연히 풀린다 |
| 메뉴 복귀·재시작으로 줌 1.0 복귀 | 다음 틱 사각형이 작아진다. 즉시 진입 몹은 클램프, 안타레스는 맵 안 상태면 클램프만 한다(기절 없음. 이동 중이 아니면) |
| 창 크기·해상도 변경 | `screenWidth/Height`에서 매 틱 계산하므로 별도 처리 없음 |
| 아주 작은 창 | `Inset`의 중앙 수렴으로 min > max가 되지 않음 |
| 안타레스 램과 벽 충돌이 같은 틱 | 램이 먼저(몹 업데이트 안), 이미 STUNNED이므로 벽 처리는 클램프만 |
| 안타레스가 모서리(두 변 동시)에 충돌 | 한 번만 기절. 두 축 모두 클램프 |
| 특이점 당김·무기 넉백으로 밀려 경계 접촉 | 이동 상태가 CHASE/SKID이고 2단계 이상이면 기절. 정지 상태면 클램프만 |

## 7. 결정 필요 (권장값)

| ID | 항목 | 권장 |
|---|---|---|
| D-01 | HEAD 빌드 복구 방식 | 미커밋 `Monster.h` 초안을 **별도 fix 커밋으로 먼저 커밋**해 HEAD를 빌드 가능하게 만든다. 이후 ARN-B2에서 공통 경로로 대체한다. (대안: main의 3줄을 되돌리는 커밋. 이 경우 안타레스 이탈 문제는 B2 전까지 그대로 남는다) |
| D-02 | 플레이어와 몹의 맵 차이 | 몹은 화면 전체, 플레이어는 HUD inset 제외 영역 — 현행 유지. 정책 이름으로 명시 |
| D-03 | 안타레스 벽 기절 속도 기준 | 2단계(200 px/s) 이상만 기절. 1단계는 미끄러짐 |
| D-04 | 벽 기절 시간 | 램 기절과 동일(`ANTARES_STUN_TIME`). 별도 값이 필요하면 명명 상수로 분리 |
| D-05 | 스폰 여백 150px·탄환 소거 여백 200px | 수치는 유지하고 맵 바깥 기준으로만 변경. 이름 붙은 상수로 이관 |
| D-06 | 성장 곡선 수치(1.4배 / 200만 점) | 이번 작업에서는 변경하지 않음. 밸런스 조정은 별도 커밋 |
| D-07 | 플레이어 HP/XP 원형 바 크기 | 현행 유지(월드 크기, 몸체와 함께 축소). 가독성 문제가 보고되면 별도 작업 |
| D-08 | 사망·게임오버 중 줌 | 사망 연출 동안 현재 줌 유지, 게임오버 결과 화면에서도 유지(월드는 이미 비워짐). 메뉴로 나갈 때만 1.0 |
| D-09 | 증강 교체·일시정지 설정 화면의 배경 줌 | 런 중 화면이므로 줌 유지 |

## 8. 범위 밖

- 카메라 중심 이동(`g_ZoomCX/CY`의 비중앙 값)은 현재 쓰이지 않는다. 이번에는 지원 범위를 넓히지 않고 현재 동작(화면 중앙 기준)만 보존한다. 필요해지면 `ComputeArenaRect`에 중심 인자를 추가한다.
- 맵 경계 시각 표시(테두리 연출)는 추가하지 않는다.
- 몹 전체에 경계를 적용하는 일(일반 로터·스웜 등)은 하지 않는다. 대상 목록 확장은 `ConstrainToArena`의 정의 한 곳만 바꾸면 되는 구조로 둔다.

## 9. 구현 결과 (2026-10-13)

동시 리팩터링과 충돌하지 않도록 별도 워크트리 브랜치 `feat/arena-bounds`에서 진행했다.

| 순서 | 커밋 | 내용 |
|---|---|---|
| FIX | `883687c` | `Monster.h` 초안을 단독 커밋해 HEAD 빌드 복구 (`feat/levelup-auto-augment`) |
| ARN-T | `5ebf352` | 스모크 단계 `SCORE=`, `PAUSESETTINGS`, `AUGREPLACE`, `DIE` |
| ARN-R1 | `3c08dcd` | `Core/ArenaBounds.h` + `tests/arena_bounds_test.cpp` + `scripts/test_arena_bounds.ps1` |
| ARN-R2 | `1f8bc57` | 줌 곡선을 `ArenaZoomForScore`로 이관 |
| ARN-R3 | `57405aa` | 플레이어·대시·넉백·원거리 몹·탄환 경계를 `MonsterManager::arena` 하나로. `g_ArenaExX/Y` 삭제 |
| ARN-R4 | `d9495a0` | `ArenaMargin` / `ConstrainToArena` / `ReflectHeading`. 몹 경계 처리 3함수 + Gravis/Quasar 루프 통합 (DL-07) |
| ARN-R5 | `ef9e988` | `IsRunWorldState` / `IsRunModalState` / `IsRunActiveState`, `BuildWorldOrtho` / `UseMainProjection` / `BeginScreenPass`, `WorldTextScale` |
| ARN-B1 | `3b4076f` | 스폰·Genesis 앵커·접근 오브를 맵 기준으로, Gravis/Quasar를 맵 전체로, 대시 목표를 플레이어 영역으로 |
| ARN-B2 | `bdbfbd7` | 안타레스를 `ConstrainToArena`에 편입 (진입 후부터, 2단계 이상 충돌 시 기절) |
| ARN-B3 | `381dd14` | UI-Z1~Z4 수정 |

**계획과 달라진 점**

- 맵 사각형은 새 전역 대신 `MonsterManager::arena`에 둔다. 몹·스폰·탄환이 이미 이 객체를 받고 있고, 테스트들이 `UpdateAll`을 위치 인자로 호출해서 인자를 늘리기 어려웠다. 갱신은 `RefreshArenaFromCamera()` 하나가 줌 갱신 직후와 고정 틱 시작에서 한다.
- `ComputeArenaRect`는 카메라 줌 중심(`ZCX/ZCY`)도 받는다. 맵이 카메라가 실제로 보는 영역과 항상 같다(`mob_death_fx_test`에서 확인).
- 사망 시 줌 리셋은 삭제했고, 결과 화면으로 넘어갈 때의 리셋도 삭제했다(D-08). 메뉴로 나가면 런 화면 판정에서 빠지므로 1.0으로 돌아간다.
- 대시 목표가 하단 HUD 영역까지 갈 수 있던 차이(다음 틱에 다시 끌려옴)를 B1에서 함께 정리했다.
- `scripts/test_mob_death_fx.ps1`은 vswhere를 PATH에 넣지 않아 실행되지 않고 있었다. 다른 테스트 스크립트와 같게 수정했다.

**검증**

- Windows Release x64 빌드 성공.
- 테스트 스크립트 23개 통과(패키지·블러·경험치 스모크 제외). 새로 추가한 확인 항목:
  - `arena_bounds_test`: 줌 곡선이 기존 식과 같은지, 사각형·inset·clamp·반사·비중앙 줌.
  - `mob_death_fx_test`:
    - 맵 = 카메라 영역
    - 스폰이 맵 밖 150px
    - Genesis 앵커가 맵 안
    - Gravis가 원래 화면 밖에서도 맵 안이면 이동하고 맵 가장자리에서 반사
    - 안타레스가 진입 중 무시, 고속 충돌 시 기절, 1단계는 미끄러짐, 기절 중 고정, `Update` 경로로 맵 밖 이탈 없음
- 스모크 캡처 비교(`build/smoke/zoom_before*` → `zoom_after*`, 점수 2,000,000):
  - 일시정지에서 연 설정 화면의 배경이 원래 크기(줌 1.0)로 그려지던 문제가 사라졌다.
  - HUD·일시정지·증강 교체·결과 화면의 UI 위치는 변경 전과 같다.
- 수동 플레이(실제 돌진 충돌의 손맛, 대시·넉백 경계, 작은 창)는 아직 하지 않았다.

**남은 항목**

- `feat/levelup-auto-augment`로의 병합은 동시 작업 세션의 미커밋 변경이 정리된 뒤 진행한다. 겹치는 파일: `main.cpp`, `GameManager.h`, `Monster.h`, `MonsterManager.h`, `EnemySpawner.cpp`.
- 치명타 라벨은 이제 `UiScale`도 받으므로, 큰 해상도에서 줌 1.0일 때 이전보다 조금 커진다(숫자와 같은 비율).
