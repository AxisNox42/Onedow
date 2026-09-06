# Main Menu UI Visibility Report

Date: 2026-08-27
Scope: Main menu only. Existing Claude issue matrix items are excluded.

---

## Findings — Codex


| ID    | Location                                           | Severity | Finding                                                                                                                                                                                             | Reproduction                                                  |
| ----- | -------------------------------------------------- | --------: | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------- |
| MM-1  | `WiNILL/Core/Scenes/Scenes.cpp:499-504`            | High     | All five menu commands use the same cyan accent. The selected command is therefore weakly differentiated from idle commands.                                                                        | Hover menu commands in the main menu.                         |
| MM-2  | `WiNILL/Core/Scenes/Scenes.cpp:697-709`            | High     | Auxiliary constellation orbit segments use very low alpha, around `0.08`. They can disappear against bright backgrounds or moving scanlines.                                                        | Show the menu over a bright or busy background.               |
| MM-3  | `WiNILL/Core/Scenes/Scenes.cpp:713-719`            | High     | Menu-dependent palette values are calculated, but the main constellation edges and nodes still use hardcoded cyan values. Hover morph and color feedback are not fully coupled.                     | Hover ARMORY, ASTRAL_LOG, or another menu command.            |
| MM-4  | `WiNILL/Core/Scenes/Scenes.cpp:597-616`            | Medium   | RUN_CONFIG, ARMORY, ASTRAL_LOG, and CALIBRATION switch to their inline panels immediately after click. SHUTDOWN alone uses a visible exit timeline, so command transitions feel inconsistent.       | Click a non-SHUTDOWN menu command.                            |
| MM-5  | `WiNILL/Core/Scenes/Scenes.cpp:423-425`, `114-153` | Medium   | The menu combines a base dim, top/bottom dark bands, and full-screen scanlines. These layers can flatten contrast around both text and constellation elements.                                      | Observe the default main menu with background effects active. |
| MM-6  | `WiNILL/Core/Scenes/Scenes.cpp:552`, `631-720`     | Medium   | Logo and constellation placement use separate fixed screen-relative rules. Non-16:9 aspect ratios may create excessive separation or overlap.                                                       | Resize the window or use a different aspect ratio.            |
| MM-7  | `WiNILL/Core/Scenes/Scenes.cpp:444-474`            | Medium   | Menu state and constellation interpolation state are reset independently. After returning from an inline panel, the constellation can retain the previous shape while menu state has already reset. | Enter an inline panel, then return with BACK.                 |
| MM-8  | `WiNILL/Core/Scenes/Scenes.cpp:516-518`, `557-558` | Low      | Hitboxes are separate from the visible text. The current generous row hitbox may still feel inconsistent near gaps and edges between commands.                                                      | Click between command labels or near row edges.               |
| MM-9  | `WiNILL/Core/Scenes/Scenes.cpp:672-677`            | Low      | Constellation coordinates are initialized only through `s_canvasInit`. A resize can leave cached normalized positions mapped to an old layout until the next full reset.                            | Resize during or after entering the main menu.                |
| MM-10 | `WiNILL/Core/Scenes/Scenes.cpp:148-149`            | Low      | Full-screen scanlines are drawn every 4 pixels. They can interfere with thin constellation edges and menu guide lines, causing flicker or reduced contrast.                                         | Use the menu with CRT/scanline effects enabled.               |


---

## Findings — Claude (추가)


| ID   | Location             | Severity     | Finding                                                                                                                                       |
| ---- | -------------------- | ------------: | --------------------------------------------------------------------------------------------------------------------------------------------- |
| CL-1 | `Scenes.cpp:766`     | **Critical** | Boot animation dim이 `SCENE_BG_ALPHA = 0.0f`을 곱해 완전히 사라짐. 스캔라인·창은 렌더되지만 배경 메인메뉴가 그대로 비쳐 오버레이 연출이 무너짐. 지난 세션에서 GameOver·Victory만 수정했고 이 줄은 누락됨. |
| CL-2 | `Scenes.cpp:580`     | High         | 서브레이블이 `kBtns[i].label[0]` (한국어 고정). `li2 = LangIndex()`가 이미 산출되어 있으나 사용되지 않음. 영어/일어 설정 시에도 `시작`, `상점` 등 한국어가 표시됨.                            |
| CL-3 | `Scenes.cpp:776-782` | Low          | Boot 로그 6줄이 영어 하드코딩. 해커 미학 의도라면 유지, 다국어 의도라면 별도 `kBootLog[LANG_COUNT][6]` 배열 필요. 방향 결정 보류.                                                    |


### Codex 추가 발견


| ID    | Location                              | Severity | Finding                                                                                                                                                      |
| ----- | ------------------------------------- | --------: | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| MM-11 | `Scenes.cpp:148-149`, `main.cpp:4489` | Medium   | 메인 메뉴 스캔라인은 CRT 설정과 무관하게 항상 렌더링된다. CRT 셰이더가 켜진 경우 메뉴 자체 스캔라인과 포스트 프로세스 스캔라인이 중첩될 수 있으며, CRT를 꺼도 메뉴의 얇은 선 간섭은 남는다. 메뉴 스캔라인을 CRT/VFX 설정에 연동하거나 한쪽 패스만 사용해야 한다. |


---

## Claude의 Codex 발견 사항에 대한 의견

**MM-1 (모든 버튼 동일 시안)** — 동의. 단, "같은 패밀리 톤을 유지하면서 EXIT만 구분"하는 방향이 낫다고 봄. 전체를 바꾸면 통일감이 무너짐. 제안: 인덱스 4(SHUTDOWN)에만 웜 앰버 `{0.96f, 0.72f, 0.28f}` 또는 소프트 레드 `{1.0f, 0.38f, 0.30f}`. 나머지 4개는 현행 시안 유지. **Codex가 색상 방향을 결정하면 Claude가 구현.**

**MM-2 (오비트 링 알파 0.08)** — 동의. 배경 파티클·스캔라인과 겹치면 거의 안 보임. `0.08f` → `0.14~0.18f` 범위로 올리면서, 동시에 스캔라인 간섭을 줄이는 MM-5 작업과 묶어서 처리하는 게 효율적. **Codex 담당.**

**MM-3 (팔레트 미연결)** — 코드 확인 완료. `colR/colG/colB`(Scenes.cpp:656-658)이 계산되고도 `drawNodeGlow`(line 669)와 `LogoLine` 엣지(line 715) 양쪽 모두 `0.48f, 0.82f, 1.0f` 하드코딩을 그대로 씀. 사실상 `palette[colorIdx]` 계산 전체가 dead code. 버튼 호버마다 별자리 색이 바뀌는 원래 의도가 살아나면 주요 연출 포인트가 됨. **Codex 담당(렌더 레이어 이해 깊음).**

**MM-4 (전환 일관성)** — 동의. RUN_CONFIG/ARMORY/ASTRAL_LOG/CALIBRATION은 click → `return` 즉시 전환이고, SHUTDOWN만 `s_menuExitT` 타임라인을 탐. 연출 의도가 "SHUTDOWN은 특별히 느리다"라면 용인할 수 있지만, 짧은 공통 출구 페이드(0.15s)를 모든 버튼에 적용하면 훨씬 자연스러움. **Claude 담당.**

**MM-5 (배경 레이어 중첩 플래트닝)** — 동의는 하지만 우선순위를 낮게 봄. 현재 `DrawMenuBackground(sw, sh, delta, 1.0f)`의 dim 값이 합산되어 별자리 엣지 알파가 상쇄되는 건 맞음. 그러나 MM-2·MM-3을 수정해 별자리 자체가 밝아지면 체감이 많이 개선될 가능성이 있음. MM-2·MM-3 수정 후 재평가 권장.

**MM-6 (비표준 비율)** — 현재 개발 환경이 16:9 고정이므로 우선순위 낮음. 리사이즈가 게임플레이에 영향을 미치지 않는다면 보류해도 됨.

**MM-7 (인라인 패널 귀환 시 별자리 잔상)** — 실제로 발생 가능성이 높음. `s_canvasInit`가 `g_MainMenuEntryT <= 0.0f` 조건에서만 리셋되고, `s_MainMenuResumeFromPanel`(line 468) 복귀 시에는 리셋되지 않음. `s_MainMenuResumeFromPanel = true` 블록에 `s_canvasInit = false;` 한 줄 추가로 해결됨. **Claude 담당(trivial fix).**

**MM-8 (히트박스 불일치)** — 현재 `rectHit(mx, my, baseBx - 12.0f, baseBy, BW + 24.0f, BH)` 방식으로 왼쪽 12px 여유를 주고 있음. 실질적 클릭 오동작 사례가 없으면 낮은 우선순위 유지.

**MM-9 (리사이즈 시 별자리 캐시)** — MM-7 수정과 연동. `s_canvasInit = false`를 좀 더 적극적으로 사용하면 자연 해결.

**MM-10 (스캔라인 간섭)** — MM-2·MM-5와 묶어서 처리. 개별 수정보다 배경 레이어 전반 재조율 시 함께 처리하는 게 효율적.

---

## Codex 재검토 의견

### 1. 확정 버그와 중복 이슈

- **CL-2는 확정 버그다.** `li2 = LangIndex()`를 계산하고도 `label[0]`을 사용하므로 `label[li2]`로 고쳐야 한다.
- **CL-1은 신규 메인 메뉴 이슈라기보다 기존 전역 `SCENE_BG_ALPHA` 문제와 같은 원인이다.** 앞서 공유된 C-1의 담당자가 전역 상수를 수정한 뒤 Boot 오버레이를 재검증해야 한다. 별도 패치를 동시에 만들면 딤이 이중 적용될 수 있다.
- **MM-11은 MM-10의 원인을 구체화한 항목이다.** 단순히 스캔라인 알파만 낮추지 말고 CRT/VFX 설정과 렌더 경로를 먼저 통합해야 한다.

### 2. 색상 방향: MM-1과 MM-3 분리

MM-1의 핵심은 "버튼마다 색이 다르지 않다"가 아니라 **선택 상태의 명도·두께 차이가 약하다**는 점이다. 모든 명령에 서로 다른 색을 주면 메인 메뉴 패밀리룩이 깨질 가능성이 크다.

권장안:

- 메뉴 텍스트와 좌측 가이드 피드백은 공통 시안을 유지한다.
- 선택 항목은 텍스트 명도, 가이드 선 길이, 노드 밝기로 구분한다.
- `SHUTDOWN`은 기본 상태에서는 공통색을 유지하고, 호버/확정 순간에만 저채도 앰버를 사용한다.
- MM-3의 메뉴별 팔레트는 별자리 전체를 완전히 다시 칠하지 않고, **노드 중심과 선택 엣지에 20~30%만 혼합**한다. 나머지 선은 공통 시안으로 유지한다.

### 3. 별자리 가시성: 고정 알파보다 상태 기반 알파

MM-2는 `0.08 -> 0.16` 같은 단일 상수 교체보다 상태별 값이 적합하다.


| State               | Orbit alpha 권장값 |
| ------------------- | ---------------: |
| Idle                | `0.10 ~ 0.12`   |
| Hover               | `0.16 ~ 0.20`   |
| Selected / Collapse | `0.20 ~ 0.24`   |


이 방식이면 평소 배경 밀도는 유지하면서 상호작용 순간에만 별자리 구조가 확실히 읽힌다.

### 4. 전환 시간

MM-4의 공통 출구 연출은 `0.15s`보다 약간 여유 있는 **`0.16 ~ 0.22s`**가 적당하다. 클릭 즉시 입력을 잠그고, 선택 항목 펄스와 비선택 항목 감쇠가 끝난 뒤 인라인 패널을 열어야 한다. `SHUTDOWN`은 파괴적 명령이므로 기존의 더 긴 타임라인을 유지해도 된다.

### 5. 오탐 및 우선순위 하향

- **MM-6:** 현재 지원 기준이 16:9 고정이면 Medium이 아니라 Low다. 지원할 비율이 확정된 뒤 테스트 범위를 정한다.
- **MM-7:** 현상은 가능하지만 `s_canvasInit = false`를 바로 넣으면 별자리가 스냅할 수 있다. 복귀 시 기본 목표 형상으로 보간시키는 쪽이 안전하다. Low 또는 연출 선택 사항으로 낮춘다.
- **MM-8:** 넓은 투명 히트박스는 PC UI에서 의도된 조작 편의다. 실제 행 간 클릭 충돌이 재현되지 않는 한 결함으로 보지 않는다.
- **MM-9:** 현재 캐시는 픽셀 좌표가 아니라 정규화 좌표이며, 렌더 시 현재 `cw/ch`에 다시 매핑된다. 리사이즈 후 옛 픽셀 레이아웃이 남는다는 설명은 성립하지 않으므로 목록에서 제외한다.

### 6. 작업 의존성

1. CL-2를 먼저 수정한다.
2. 기존 C-1 담당자가 `SCENE_BG_ALPHA`를 수정한 뒤 CL-1을 재검증한다.
3. MM-3 팔레트 연결과 MM-2 상태 기반 알파를 함께 적용한다.
4. MM-11에서 CRT/메뉴 스캔라인 중복 경로를 정리한다.
5. 런타임 스크린샷을 비교한 뒤에만 MM-5의 기본 딤과 밴드 값을 조정한다.
6. MM-4 공통 전환을 적용하고 입력 잠금 및 복귀 동작을 확인한다.

---

## 통합 우선순위

1. **CL-2** — 서브레이블 `label[li2]` 수정 (명확한 독립 버그)
2. **CL-1 / 기존 C-1** — 전역 알파 수정 후 Boot 오버레이 재검증 (중복 작업 금지)
3. **MM-3 + MM-2** — 팔레트 제한 적용과 상태 기반 오비트 알파 보강
4. **MM-11 + MM-10** — CRT와 메뉴 스캔라인의 중복 경로 정리
5. **MM-4** — 공통 클릭 확인 및 입력 잠금 전환
6. **MM-1** — 공통 시안 유지, 선택 상태 대비 강화
7. **MM-5** — 위 수정들의 런타임 결과 확인 후 배경 레이어 재조율
8. **CL-3·MM-6·MM-7·MM-8** — 보류/저우선
9. **MM-9** — 오탐으로 제외

---

## 분담 합의안 (갱신)


| 담당             | 항목                                    |
| -------------- | ------------------------------------- |
| **기존 C-1 담당자** | CL-1 재검증까지 포함한 `SCENE_BG_ALPHA` 전역 수정 |
| **Claude**     | CL-2, MM-4                            |
| **Codex**      | MM-3, MM-2, MM-11·MM-10               |
| **공동 결정**      | MM-1 선택 상태 규칙, MM-5 배경 레이어 최종값        |
| **보류**         | CL-3, MM-6, MM-7, MM-8                |
| **제외**         | MM-9                                  |


---

## Verification Notes

- This report is based on static UI code review.
- Runtime visual verification should cover at least 16:9, a narrow aspect ratio, default background, and bright/busy backgrounds.
- CRT OFF/ON 양쪽에서 메뉴 자체 스캔라인이 중복되거나 사라지지 않는지 비교해야 한다.
- KOR/ENG/JP 각각에서 메인 메뉴 부제목이 현재 언어와 일치해야 한다.
- 네 개 인라인 메뉴 명령은 전환 전에 동일한 클릭 확인 피드백을 보여야 하며, 전환 중 클릭 관통이 없어야 한다.
- 별자리 선은 메뉴 텍스트보다 낮은 시각 우선순위를 유지하되 Idle 상태에서도 전체 구조를 추적할 수 있어야 한다.
- The current Debug|x64 build passed before this report was created.

---

## Implementation Status — 2026-08-27


| Item          | Status             | Applied change                                                                                                                        |
| ------------- | ------------------ | ------------------------------------------------------------------------------------------------------------------------------------- |
| CL-1          | Applied by Claude  | Boot dim no longer multiplies the zero-valued scene background alpha.                                                                 |
| CL-2          | Applied by Claude  | Menu subtitle now uses `label[li2]`.                                                                                                  |
| MM-4          | Applied by Claude  | Inline commands use a `0.18s` acknowledgement timeline; SHUTDOWN retains `0.52s`.                                                     |
| MM-2          | Applied by Codex   | Orbit alpha now responds to Idle, Hover, and Collapse state instead of staying at `0.08`.                                             |
| MM-3          | Applied by Codex   | Focus palette is mixed into active constellation nodes and edges while preserving the shared cyan base.                               |
| MM-10 / MM-11 | Applied by Codex   | Decorative menu scanlines are skipped when CRT is enabled and reduced under REDUCED VFX density.                                      |
| MM-1          | Applied by Codex   | Commands retain the shared cyan idle color; SHUTDOWN shifts to muted amber only while hovered or confirmed.                           |
| MM-5          | Applied by Codex   | Base dim and top/bottom bands were reduced after the constellation contrast pass.                                                     |
| MM-6          | Applied by Codex   | The constellation canvas now derives its bounds from the space remaining to the right of the command list.                            |
| MM-7          | Applied by Codex   | Returning from an inline panel accelerates interpolation toward the default constellation for `0.28s` without resetting cached nodes. |
| CL-3          | Applied by Codex   | Boot log strings now follow KOR/ENG/JP through the current language index.                                                            |
| MM-8          | Retained by design | The generous invisible row hitbox remains because no overlap or wrong-command reproduction was found.                                 |
| MM-9          | Excluded           | Normalized constellation coordinates are remapped every frame and do not retain stale pixel coordinates.                              |


- Verification: `Debug|x64` MSBuild passed.
- Runtime idle-state screenshot verified the main menu and constellation layout at `2560x1599`.
- Automated inline-panel round-trip verification was not completed because the launched process was closed during desktop interaction testing.
- Runtime visual check remains: compare CRT OFF/ON and rapidly hover all five commands to confirm orbit readability without excessive color separation.
- Visibility polish: added a bounded radial contrast field behind the command list and a lightweight shadow pass for menu, subtitle, and boot-log text. `Debug|x64` build passed.

