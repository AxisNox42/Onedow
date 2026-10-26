# 웨이브 HUD 인수인계 — 2026-10-17

## 한눈에

전투 화면 위 가운데의 웨이브 HUD를 새로 만들었습니다. 지금 화면에는 아래 세 가지만 있습니다.

- 헤더: `WAVE 05 / 15`와 웨이브 남은 시간 `02:38`. 남은 시간이 10초 이하가 되면 시간 숫자만 빨갛게 커집니다.
- 기호 트랙: 7칸 창(지난 3 · 현재 · 다음 3)만 보입니다. 현재 칸은 항상 가운데이고, 창 양 끝 칸은 40%로 흐리게 그립니다. 런의 첫 웨이브와 최종 웨이브는 그 너머가 없으니 흐리지 않습니다. 일반 웨이브는 ○, 정예 웨이브는 ◇, 최종 웨이브는 ◈입니다. 칸 아래 글자나 몹 그림은 없습니다.
- 현재 칸: 테두리는 자기 색으로 항상 꽉 차 있고, 웨이브가 진행되면서 안쪽이 12시부터 시계방향으로 채워집니다.

[Docs/Concepts/HUD_Wave_FinalSpec.png](../Concepts/HUD_Wave_FinalSpec.png)는 중간 단계 시안입니다. 정예 태그, 몹 그림, 디버프 줄이 들어 있지만 이 셋은 이후에 모두 뺐습니다. 실제 결과는 이 문서와 코드를 기준으로 봅니다.

## 현재 상태

- 브랜치 `main`. 웨이브 HUD 커밋은 `9cc61e5`부터 `2eefa5f`까지입니다(아래 표). 아직 푸시하지 않았습니다.
- Release 빌드 성공: `build/windows/x64/Release/WiNILL.exe`.
- 작업 트리에는 다른 작업(퀘이사·제네시스 정예)의 미커밋 변경이 많습니다. `main.cpp`, `Monster.h`, `EntityDraw.cpp`, `Codex.h`, `SmokeCapture.h`, `MonsterManager.h`와 Docs/Concepts의 삭제·추가 파일이 여기에 해당합니다. 웨이브 HUD와는 관계없으니 되돌리거나 함께 커밋하지 마세요. 커밋 전에 항상 `git status --short`로 확인합니다.
- `main.cpp`에서 이 작업의 변경만 골라 커밋할 때는 HEAD 버전에 내 hunk만 적용해서 스테이징했습니다. `git apply --cached`나 `git hash-object -w` + `git update-index --cacheinfo`를 쓰면 됩니다.

| 커밋 | 내용 |
|---|---|
| `9cc61e5` | 웨이브 트랙 첫 구현. 정예 종류 미리 정하기, 스모크 단계 `DIFF=`·`CODEXMOB=` 추가 |
| `09ce2f1` | 정예 칸 타이머를 마름모 테두리로 옮김. 태그에서 몹 그림 제거 |
| `e92da54` | 정예 텍스트 태그 제거. 트랙 선이 칸 테두리 앞에서 끊기도록 수정 |
| `50526bf` | 디버프 줄 제거. 번호·타이머·트랙만 남김 |
| `9e2a7d1` | 키라인을 색조 키라인으로 바꿔 어떤 배경화면에서도 보이게 함 |
| `d963c7d` | 웨이브 막바지에 현재 칸 테두리가 흐려지던 문제 수정 |
| `c7db80a` | 타이머를 테두리 대신 안쪽 시계방향 채움으로 변경 |
| `dab4430` | (다른 세션) 선 그리기 함수를 `HudLines.h`로 분리 |
| `2eefa5f` | 트랙을 3·현재·3 창으로 변경. `ClosedOutline` 모서리를 마이터 조인으로 바꿔 흐린 마름모의 모서리가 겹쳐 보이지 않게 함 |
| `6f6a4d5` | 정예 후보를 모든 난이도에서 퀘이사·그라비스·제네시스 3종 무작위로 변경 |
| `12be408` | 일반 몹 첫 등장 웨이브를 난이도별로 펼침 ([EnemyRoster.h](../../WiNILL/Entity/EnemyRoster.h), 테스트 `scripts/test_enemy_roster.ps1`) |
| `630fe86` | 엘리트 몹 3종을 일반 명단에서 빼고 정예 웨이브에서만 나오게 함 |

## 코드 위치

- [WiNILL/Render/WaveHud.h](../../WiNILL/Render/WaveHud.h)
  - 헤더 전용 파일이라 vcxproj나 CMake를 수정할 필요가 없습니다.
  - 진입점은 `DrawWaveHud(sw, hudScale, hudTopY, gameTime, difficulty, gm)`입니다.
  - 그리기 도우미는 `wave_hud` 네임스페이스에 있습니다: `Segment`, `Arc`, `Ring`, `DiamondOutline`, `SectorFill`, `Dot`.
- [WiNILL/main.cpp](../../WiNILL/main.cpp)
  - 상단 HUD 블록에서 `DrawWaveHud(...)`를 한 줄로 호출합니다. 예전 점 5개 HUD와 디버프 토스트 코드는 지웠습니다.
  - `ApplyWaveDebuff`는 디버프를 고르기 전에 `lastWaveDebuff = -1`로 초기화합니다.
  - 스모크 단계 `DIFF=`, `CODEXMOB=`도 여기에 있습니다.
- [WiNILL/Entity/EnemySpawner.cpp](../../WiNILL/Entity/EnemySpawner.cpp) / [.h](../../WiNILL/Entity/EnemySpawner.h)
  - 정예 종류를 런 시작 때 미리 정합니다. `ResetEnemySpawnState()`가 정예 웨이브마다 `elitePick`을 굴립니다.
  - `GetScheduledElite(wave, difficulty)`는 예정된 정예의 종류, 등장 여부, 생존 여부를 돌려줍니다.
  - 정예 후보는 모든 난이도에서 퀘이사·그라비스·제네시스 3종 중 무작위입니다(`kEliteKinds`, `6f6a4d5`). 등장 시각은 웨이브 시작 1:30 그대로입니다. `DifficultyParams::eliteTierHalfSteps`는 이때 제거했습니다.
- [WiNILL/Core/WaveSystem.h](../../WiNILL/Core/WaveSystem.h)
  - 상수 `ELITE_WAVE_INTERVAL = 5`, `ELITE_SPAWN_WAVE_SECONDS = 90`을 추가했습니다.

## 확정된 규칙

- 5의 배수 웨이브는 모든 난이도에서 정예 웨이브(◇)입니다. 트랙은 여전히 예정된 몹을 `IsEliteMob`으로 확인한 뒤 ◇를 그립니다.
  - 예전에는 EASY 정예 웨이브에 레굴루스나 안타레스(진짜 정예 아님)가 나와서 일반 칸으로 표시했습니다. `6f6a4d5`부터는 EASY도 정예 3종 중 하나가 나옵니다.
- 보스 웨이브는 기획에는 있지만 보스가 아직 구현되지 않았습니다. 지금은 정예까지만 표시합니다. 보스를 추가할 때는 트랙에 새 기호를 넣어야 합니다.
- 트랙에는 기호만 둡니다. 정예 이름 태그, 몹 실루엣, 디버프 줄은 사용자가 헷갈린다고 판단해 모두 뺐습니다.
  - 정예 이름을 도감 규칙(처치 → 이름, 목격만 → `???`, 미발견 → `?`)으로 보여주는 작업은 한 번 구현했다가 기호형으로 바꾸면서 제거했습니다.
- 대비: 판이나 막은 쓰지 않습니다(디자인 규칙). 각 선 아래에 자기 색 × 0.18, 알파 0.88, 두께 +3.2px인 색조 키라인을 깝니다. 글자 halo와 같은 원리입니다.
  - 게임 창이 투명 프레임버퍼라서 게임은 바탕화면 밝기를 알 수 없습니다. 배경에 맞춰 바꾸는 방식은 불가능하니, 어떤 배경에서도 통하는 방식만 써야 합니다.
  - CircleTexture 원판은 판/막에 해당해서 쓰지 않기로 했습니다.
- 현재 칸
  - 테두리는 자기 색(일반 하늘색, 정예 주황, 최종 금색)으로 꽉 채워 그립니다.
  - 안쪽은 경과 시간만큼 시계방향으로, 자기 색 알파 0.85로 채웁니다. 0.55로 하면 어두운 바탕에서 색이 갈색이나 청록으로 탁해졌습니다.
  - 최종 칸은 이중 마름모를 유지하고, 안쪽 마름모 안을 채웁니다.
  - 마지막 10초에도 칸 색은 바꾸지 않습니다. 헤더 시간 숫자만 빨갛게 바뀝니다.
- 트랙 선은 각 칸 테두리 앞에서 끊어 그립니다(`nodeHalf` + `linkPad`). 선이 칸을 관통하면 안 됩니다.
- 창 크기는 `kWindowSide = 3`, 칸 간격은 `30 * hudScale`로 고정입니다. 창 밖 웨이브(1 미만, 최종 초과)는 그리지 않습니다.
- 선 그리기 함수는 [WiNILL/Render/HudLines.h](../../WiNILL/Render/HudLines.h)에 있고 스킬·체력 HUD와 함께 씁니다. 바꿀 때는 세 HUD를 모두 확인하세요.

## 몹 첫 등장 (EnemyRoster.h)

| 몹 | EASY | NORMAL | HARD |
|---|---|---|---|
| 스웜 | 1 | 1 | 1 |
| 로터 | 2 | 2 | 1 |
| 짐벌 | 3 | 3 | 2 |
| 스코프(원거리) | - | 6 | 4 |
| 레굴루스 | - | 8 | 6 |
| 안타레스 | - | 11 | 8 |
| 마그네타 | - | - | 11 |

- 등장 확률은 첫 등장 웨이브부터 웨이브마다 2%씩 올라 상한에서 멈춥니다. 로터는 나머지 몫을 가져갑니다. EASY·NORMAL 1웨이브는 로터가 아직 없어 전부 스웜입니다.
- 정예 웨이브(5의 배수)에는 새 몹이 처음 나오지 않습니다.
- 엘리트 몹(그라비스·제네시스·퀘이사)은 일반 명단에 없고, 정예 웨이브의 예정 정예로만 나옵니다.

## 확인 방법

빌드:

```
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build_windows.ps1
```

스모크 캡처의 결과는 `build/smoke/<Tag>/`에 저장됩니다.

```
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/smoke_ui.ps1 -Steps "RUN~1;WAVETIME=630~1.5;WAVETIME=810~1.5;WAVETIME=894~1.5;RUN~0.5;DIFF=EASY~0.3;WAVETIME=1700~1.5" -Tag wave
```

- `WAVETIME=<초>`: 해당 시점으로 바로 이동합니다.
  - 630은 4웨이브 중반, 810은 5웨이브(정예) 중반, 894는 5웨이브 막바지입니다.
  - EASY 1700은 최종 10웨이브입니다. HARD 3480은 최종 20웨이브입니다.
- `DIFF=EASY|NORMAL|HARD`: `RUN` 뒤에 씁니다. 한 번 바꾼 난이도는 다음 `RUN`에서도 유지되므로, 다른 난이도로 돌아가려면 다시 `DIFF=`를 넣으세요.
- `CODEXMOB=UNSEEN|SEEN|KNOWN`: 도감 몹 상태를 바꿉니다. 저장되지 않습니다. 지금 HUD는 이 값을 쓰지 않지만 정예 이름 표시를 되살릴 때 쓸 수 있습니다.
- 캡처 PNG는 배경이 투명입니다. 반드시 여러 배경에 합성해서 봐야 합니다.
  - 흰색 `#F3F3F3`, 중간 회색 `#808080`, 어두운 `#07090F`, 채도 높은 단색, `C:/Windows/Web/Wallpaper/*/*.jpg` 기본 배경화면.
  - 지금까지 비교 결과: `build/smoke/keyAB_sheet.png`(키라인 비교), `build/smoke/sector_sheet.png`(최종 형태).
- 주의: `WAVETIME`으로 이동하면 웨이브 전환이 일어나지 않아서 디버프가 걸리지 않습니다. 실제 전환을 보려면 경계 몇 초 앞(예: `WAVETIME=356~6`)에서 기다리세요.
- 주의: 정예를 `KILL`하면 경험치 때문에 레벨업 카드가 떠서 게임 시간이 멈춥니다.

## 남은 일

1. **웨이브 디버프 표시가 하나도 없습니다.** 예전 4초 토스트도 지웠고, 트랙 밑 디버프 줄도 사용자 요청으로 뺐습니다. 지금은 디버프가 걸려도 화면에 아무 피드백이 없습니다. 웨이브 HUD와 따로, 전환 순간에만 짧게 보여주는 방식 등을 사용자와 정해야 합니다.
2. **보스 웨이브 기호**: 보스가 구현되면 트랙에 추가합니다. 기획은 [Boss_Design_Proposal_2026-10-12.md](../Boss_Design_Proposal_2026-10-12.md)에 있습니다.
3. **남은 HUD 비평 항목**: [HUD_Critique_02_SkillButtons.png](../Concepts/HUD_Critique_02_SkillButtons.png)와 [HUD_Critique_03_HpXp.png](../Concepts/HUD_Critique_03_HpXp.png), [HUD_Critique_04_Layout.png](../Concepts/HUD_Critique_04_Layout.png)의 제안이 아직 구현되지 않았습니다.
   - 스킬 버튼: 빈 Q/E/R 슬롯 표시, 아이콘과 테두리 쿨다운, 액티브는 사각·패시브는 원으로 구분.
   - 체력·경험치: HP 색 고정과 10칸 분할, XP를 화면 하단 줄로 분리.
   - 좌상단 `Lv.` 텍스트와 하단 키 안내도 정리 대상입니다.
4. **로컬 전용 문서**: `Docs/Concepts/HUD_*.png` 8장(비평·시안·대비 비교)과 이 문서는 사용자 결정에 따라 로컬에만 두고 커밋하지 않습니다.
5. **소소한 정리**
   - `EnemySpawner`의 `ScheduledEliteInfo`에 `spawned`·`alive` 필드가 있지만, 태그를 뺀 뒤로 HUD는 쓰지 않습니다. 정예 상태를 다시 쓸 계획이 없으면 정리해도 됩니다.
   - `CODEXMOB=` 스모크 단계도 같은 경우입니다.

## 사용자 디자인 성향

- 판·막 없이 글리프 단위로 가독성을 확보합니다. 점선은 쓰지 않습니다. 몹 디자인은 선만 쓰고 채우기를 하지 않습니다.
- 설명은 그림으로 보여주는 걸 선호합니다. 시안은 Docs/Concepts의 어두운 남색 보드 스타일로 만들었습니다(PIL 스크립트, 1600px 폭, 맑은 고딕).
- 정보가 많은 것보다 단순한 기호를 선호합니다. 이번에 태그, 몹 그림, 디버프 줄이 차례로 빠졌습니다.
- 다른 작업과 섞지 말고 해당 작업의 변경만 골라서 커밋하기를 원합니다.
