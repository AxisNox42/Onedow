# 전투 스킬 HUD 재설계 — 설계

참고 이미지: `Docs/Concepts/HUD_Critique_02_SkillButtons.png` (출발점), `Docs/Concepts/Skill_HUD_Icon_Options.png` (아이콘 방향 B + C 혼합)

## 목적

좌하단 스킬 HUD가 전투 중 한눈에 "지금 쓸 수 있나 / 얼마나 남았나 / 비어 있나 / 패시브가 어떤 상태인가"를 답하게 한다.
흰 바탕화면과 어두운 바탕화면 모두에서 판 없이 읽혀야 한다.

범위는 **표시만**이다. 스킬 종류·슬롯 수·교체 규칙(`EquipSkill`)·쿨다운 수치는 바꾸지 않는다.
스킬 획득 연출, 도감·증강 아이콘 교체는 범위 밖.

## 배치

```
  (RAIN) (LSTP) (DEATH)      ← 패시브 행: 원, 가진 것만, 왼쪽 정렬
  ─────────────────────
  SHIFT   Q      E      R    ← 키 글자: 틀 밖 왼쪽 위
  [ >> ] [ ⊙ ]  [ ◎ ]  [ + ] ← 액티브 행: 사각, 항상 4칸
```

- 위치: 좌하단 그대로. 액티브 행 기준은 `BottomLeftActionX(hs)`, `Hud::SKILL_KEYS_Y`. 패시브 행은 액티브 행 바로 위.
- 액티브 사각: 한 변 60·hs, 간격 10·hs. 슬롯은 SHIFT / Q / E / R 4칸 고정.
- 패시브 원: 반지름 17·hs, 간격 12·hs. 가진 것만 표시하고, 순서는 RAIN → LSTP → DEATH 고정(위치가 섞이지 않음). 하나도 없으면 행 자체가 없다.
- 기존 상태 슬롯(`Hud::SLOT_BAR_BASE` 줄의 RAIN / LSTP / DEATH 사각)은 없앤다.

## 공통 규칙

- 선만 쓴다. 판·막 없음, 점선 없음. 예외는 쿨다운·충전의 내부 부채꼴 채움(웨이브 노드와 같은 언어).
- 모든 선 글리프 밑에 자기 색의 어두운 키라인(웨이브 HUD의 `KeyCol`: 색 × 0.18, 알파 0.88, 두께 +3.2·hs).
- 글자는 키 글자(SHIFT/Q/E/R)와 쿨다운 중 남은 초만. 스킬 이름 글자(DASH/CLOSE …)는 없앤다. 글자는 텍스트 렌더러 할로를 그대로 쓴다.
- 아이콘은 B + C 혼합: 뜻이 바로 읽히는 선 픽토그램 + 선의 끝점·마디에 별자리 점(반지름 약 1.8·hs).
- 색은 지금 색상 유지: 대시·TIME = 시안(0.40, 1.00, 1.00 / 0.40, 0.90, 1.00), CLOSE = 파랑(0.50, 0.80, 1.00), FOCUS = 민트(0.59, 0.94, 0.86, 지금 하늘색에서 바꿔 CLOSE·TIME과 구분), RAIN = 주황(1.00, 0.50, 0.20), LSTP = 연보라(0.70, 0.70, 0.85), DEATH = 빨강(1.00, 0.36, 0.38).
- 표시 상태: RUNNING, 그리고 인벤토리가 닫힌 PAUSED (지금과 같음). PAUSED에서는 펄스 시간이 흐르지 않는다.

## 아이콘

| 대상 | 글리프 |
|---|---|
| 대시 | 겹친 쉐브론 2개 `>>` + 꼬리 짧은 선, 쉐브론 꼭짓점에 점 |
| 대시 (FLASH 업그레이드) | 대시 글리프 + 앞쪽 쉐브론 끝에 별자리 점 하나 더 (조금 더 큼) |
| CLOSE (대폭발) | 가운데 점 + 작은 원 + 대각 4방향 바깥 화살표, 화살표 끝에 점 |
| FOCUS (초집중) | 원 + 안쪽 작은 원 + 상하좌우 눈금 4개, 눈금 끝에 점 |
| TIME (시간 정지) | 모래시계 X자 윤곽 + 아래 모래선, 네 꼭짓점에 점 |
| RAIN | 비스듬히 떨어지는 화살 3개, 화살 꼬리에 점 |
| LSTP | 겹친 깃 호 3개, 호 끝에 점 |
| DEATH | 가운데 작은 원(오브) + 궤도 원 + 궤도 위 스택 점 |

## 액티브 상태

| 상태 | 틀 | 아이콘 | 내부 | 글자 |
|---|---|---|---|---|
| 준비됨 | 자기 색, 두께 2·hs, 알파 1.0 | 알파 1.0 | 없음 | 키 글자 |
| 쿨다운 | 두께 1.4·hs, 알파 0.55 | 알파 0.30 | 경과 비율 `1 − cdRemain/cdMax`만큼 12시부터 시계방향 부채꼴, 자기 색 알파 0.30, 틀 안쪽 1.5·hs 띄움 | 키 글자 + 가운데 남은 초 (`ceil`) |
| 준비 순간 | 준비됨 + 같은 모양 사각이 0 → 14·hs 바깥으로 커지며 0.45초에 걸쳐 사라짐 | | | |
| 효과 지속 중 | 틀 바깥 5·hs에 얇은 선(1.1·hs) 사각, 알파 = 남은 지속 비율 | 알파 1.0 | 쿨다운 부채꼴이 함께 진행 (쿨은 사용 순간부터 돈다) | 남은 초 |
| 빈 슬롯 | 두께 1.0·hs, 알파 0.35, 회청색(0.36, 0.41, 0.53) | `+` 한 개 | 없음 | 흐린 키 글자 |

- 효과 지속: 대시 = `dashActive` 동안 `1 − dashProgress`, FOCUS = `hyperFocusTimer / HYPER_FOCUS_DUR`, TIME = `timeStopTimer / TIMESTOP_DUR`. CLOSE는 즉발이라 없음.
- 대시 cdMax: 대시할 때 실제로 넣은 값(`DASH_CD` 또는 `HYPER_FOCUS_DASH_CD`). `PlayerRuntimeState::dashCooldownMax`를 새로 두고 `BeginPlayerDash`에서 같이 기록한다.
- 스킬 cdMax: `PlayerSkillCooldownMax(type)`.

## 패시브 상태 (숫자 없음, 아이콘이 변함)

- **RAIN**(`g_Stats.bulletRain`): 원 안이 충전 비율 `g_BulletRainTimer / bulletRainCooldown`만큼 12시부터 시계방향으로 채워진다(알파 0.30). 발사되는 순간(타이머가 되감기는 프레임) 원이 한 번 펄스하고 채움이 비워진다. 아이콘은 충전 중 알파 0.55, 가득 차면 1.0.
- **LSTP**(`g_Stats.lightStep`): ON(`lightStepDisableTimer <= 0`) = 글리프 두께 1.8·hs, 알파 1.0. OFF = 같은 글리프 두께 1.0·hs, 알파 0.35, 원 안 부채꼴이 복귀 비율 `1 − disableTimer / lightStepHitLock`만큼.
- **DEATH**(`g_ApproachOrbs`가 비어 있지 않음): 궤도 위 점 개수 = `approachStacks`(1 ~ 3으로 자름). 점은 120° 간격, 12시부터.

## 코드 구조

- **`WiNILL/Render/HudLines.h`** (새로): `wave_hud`의 `Rgb`, `KeyCol`, `Segment`, `Arc`, `Ring`, `DiamondOutline`, `SectorFill`, `Dot`을 `hud_lines` 네임스페이스로 옮긴다. `WaveHud.h`는 이것을 include 해서 쓴다(동작 변화 없음). `SectorFill`에 사각(`|x|`, `|y|` 중 큰 값 = r) 모양을 추가한다.
- **`WiNILL/Render/SkillHudModel.h`** (새로, GL 의존 없음):
  - `enum class SkillGlyph { Dash, Flash, Burst, Focus, TimeStop }`, `enum class PassiveGlyph { Rain, LightStep, Death }`.
  - `struct SkillSlotView { bool empty; SkillGlyph glyph; int colorId; float cdRemain, cdMax; float effectFrac; float pulse; const wchar_t* key; }`.
  - `struct PassiveView { PassiveGlyph glyph; float chargeFrac; bool on; int stacks; float pulse; }`.
  - `struct SkillHudView { SkillSlotView slots[4]; PassiveView passives[3]; int passiveCount; }`.
  - `struct SkillHudInputs`: 슬롯 타입·쿨 3개, 대시 쿨/최대/지속, FOCUS·TIME 타이머, dashUpgrade, RAIN/LSTP/DEATH 관련 값 — 전부 평범한 값.
  - `struct SkillHudPulse { float slot[4]; float rain; float prevCd[4]; float prevRain; void Update(const SkillHudInputs&, float delta); }`: 쿨이 0 초과 → 0 이하로 넘어가는 프레임, RAIN 타이머가 되감기는 프레임에 0.45초 시작. `delta == 0`이면 아무것도 바뀌지 않는다.
  - `SkillHudView BuildSkillHudView(const SkillHudInputs&, const SkillHudPulse&)`.
- **`WiNILL/Render/SkillHud.h`** (새로, WaveHud처럼 헤더 전용): 글리프 7종 + FLASH, 틀, 부채꼴, 펄스, 키 글자·초. `void DrawSkillHud(const SkillHudView&, float hudScale, float sh)`.
- **`main.cpp`**: `skillBox` 람다 블록과 상태 슬롯 `drawSlot` 블록을 지우고, 입력 채우기 → `g_SkillHudPulse.Update(inputs, delta)`(게임 루프, PAUSED면 0) → 렌더 단계에서 `DrawSkillHud(BuildSkillHudView(...), hudScale, sh)`.
- **`PlayerRuntime`**: `dashCooldownMax` 필드 + `BeginPlayerDash`에서 기록 + 리셋 시 0.

## 테스트

- **`tests/skill_hud_model_test.cpp`** + **`scripts/test_skill_hud_model.ps1`** (기존 cl 단일 파일 테스트 패턴):
  - 빈 Q/E/R 슬롯 → `empty`.
  - 쿨 남은 6초 / 최대 16초 → 경과 비율 0.625.
  - FOCUS 타이머 2.5초 → effectFrac 0.5. CLOSE는 항상 0.
  - dashUpgrade → `Flash`.
  - 패시브 순서 RAIN → LSTP → DEATH, 없는 것은 빠짐. approachStacks 5 → stacks 3.
  - LSTP 타이머 > 0 → `on == false`, 복귀 비율 계산.
  - 쿨 0.1 → 0 프레임에 pulse 0.45 시작, delta 0이면 그대로.
- **스모크 단계 `SKILLHUD=<preset>`** (`main.cpp` 스모크 분기):
  - `MIX`: CLOSE / FOCUS / TIME 장착, CLOSE 쿨 40%, FOCUS 효과 지속 중, TIME 준비됨, 대시 쿨다운, RAIN 60%, LSTP OFF, DEATH ×3.
  - `EMPTY`: 대시만.
  - `PULSE`: TIME의 준비 순간 펄스 중간 프레임.
- **눈 확인**: `scripts/smoke_ui.ps1`로 2560×1600, 1920×1080 캡처 → 흰 바탕(#F3F3F3)·회색(#808080)에 합성해 다섯 상태(준비 / 쿨다운 / 준비 순간 / 지속 중 / 빈 슬롯)와 패시브 세 종류가 한눈에 구분되는지 본다. 웨이브 HUD가 이전과 똑같은지 같은 캡처로 확인.
