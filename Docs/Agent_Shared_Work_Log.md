# Agent Shared Work Log

## 사용 규칙 - LOCKED

이 섹션은 최초 작성 규칙이므로 수정하지 않는다.

1. Codex와 Claude는 각자 지정된 작업 구역만 수정한다.
2. 다른 에이전트의 구역은 삭제, 축약, 재작성하지 않는다.
3. 공통으로 알아야 할 내용은 `Shared Handoff`에 날짜와 작성자를 붙여 추가한다.
4. 충돌, 미확인 사항, 빌드 실패는 숨기지 말고 `Shared Handoff` 또는 자기 구역의 `Open Issues`에 남긴다.
5. 작업 기록은 최신 항목이 위로 오게 추가한다.
6. 사용 규칙 변경이 필요하면 문서를 직접 고치지 말고 사용자에게 먼저 확인받는다.
7. 날짜별 작업 기록은 `### YYYY-MM-DD` 아래에 묶고, 날짜가 바뀌면 새 날짜 제목 위에 `---` 표시줄을 둔다.

---

## Shared Handoff

### Current Core State

- RUN CONFIG now follows: weapon select -> optional trials -> execute.
- Trial flow now uses: TRIAL MODULE ON -> select trial count with < NN > -> SELECT TRIALS -> dedicated TRIAL SELECT page -> EXECUTE RUN.
- Trial UI uses existing runtime data: g_TrialPool, g_TrialSelected, TRIAL_DEFS, TrialScoreBonusForDef.
- Turning TRIAL MODULE OFF clears selected trials so hidden penalties cannot remain active.
- RUN CONFIG UI polish includes OS-path subtitle, CREDITS panel, radar morphing, stat glitch text, terminal-style trial toggle/count stepper.
- Claude rebuilt Codex and Settings scenes around the constellation/terminal visual language.
- Normal mob spawning now starts lower-density and ramps by time.
- Active demo weapon set is Rifle + Static Field. Cannon is removed from RUN CONFIG/ARMORY and cannon-specific augments are disabled.
- Latest checked build: MSBuild Debug|x64 passed. Existing warnings only: APIENTRY and/or LIBCMT.

### Collision Notes

- WiNILL/Core/Scenes/Scenes.cpp is the highest-risk conflict file: RUN CONFIG, Settings, and Codex UI changes are concentrated there.
- Any block that changes g_BatchAlpha must restore it to 1.0f before leaving the scene function.
- Temporary verification and scratch files were removed on 2026-08-25: bin_verify/, WiNILL/main.cpp.bak, WiNILL/main_temp.cpp, root patch/find scripts, and Docs/orca-paste screenshots.
- RUN CONFIG and TRIAL SELECT are build-verified, but still need real screen/click QA.

### Key Files

- WiNILL/Core/Scenes/Scenes.cpp: major UI rendering, RUN CONFIG, Settings, Codex scenes.
- WiNILL/System/Settings.h: trial definitions, selected trial state, score multiplier, runtime trial effects.
- WiNILL/main.cpp: spawn curve, run loop, trial effect usage.
- Docs/Agent_Shared_Work_Log.md: shared coordination log. Do not edit LOCKED rules. Do not edit Claude Workspace without explicit need.

---

## Codex Workspace

### Current State

- Recent Codex work: RUN CONFIG and trial flow, spawn balance, augment/debuff values, build fixes.
- RUN CONFIG state:
  - Weapon selection enables execution.
  - Trials OFF starts the run directly.
  - Trials ON exposes a count stepper and then opens the dedicated trial selection page.
  - The trial page requires exactly the requested number of selections before final execution.
- Trial selections are connected to the existing runtime arrays and actual gameplay effects.
- Player weapon visuals now replace the full player body per weapon: Rifle uses Layered Cross-Star dual 4-point frames + slow outer orbit brackets/dashed nodes, Static Field uses Resonance Beacon counter-rotating triangle/hex + field zaps.
- Active weapon presentation is narrowed to Rifle and Static Field. Cannon is removed from RUN CONFIG/ARMORY and disabled through AugRemoved for compatibility.
- The in-run player unique window no longer redraws a late titlebar chrome pass.
- ARMORY now opens as an in-Main-Menu panel: clicking ARMORY keeps GameState::MAIN_MENU, immediately starts the menu-to-tree animation, and returns through the same reverse path with BACK/ESC/right-click.
- Latest Debug|x64 build passed.

### Open Issues

- Visual/click QA still needed for RUN CONFIG and TRIAL SELECT.
- Trial descriptions and actual effect severity need playtest validation.
- Verify ON/OFF, Back, target-count changes, and launch-time clearing behavior.

### Next Candidates

- Screenshot-based RUN CONFIG/TRIAL SELECT polish.
- Visual QA for ARMORY Main Menu expansion timing and tree spacing.
- Decide whether trial page needs REROLL, locked slots, or risk-tier labels.
- 5-minute and 10-minute spawn-curve survival tests.
- Trial multiplier and difficulty report after playtesting.

---

## Claude Workspace

### Current State

- 브라우저 크롬: `BROWSER_CHROME_H = 0.0f` (타이틀바 완전 제거), `DrawBrowserChrome` 드로우 없음
- 씬 전환: `StartSceneFade()` 기반 콘텐츠 알파 페이드 (전체화면 블랙 아님)
- 배경 시스템: `DrawMenuBackground()` 공용 함수로 모든 메뉴 씬 동일 배경
- 패널 투명도: `SceneFlowWindow` / `SceneAppWindow` 파라미터로 제어 가능
- 전역 알파: `g_BatchAlpha` (DrawPrim + TextRenderer 적용)
- DEV MODE: RUN CONFIG 씬 하단 우측 토글 버튼
- **타이틀바 전면 제거 완료**: `WindowChrome.h` FLOW_CHROME_TOP=10px, `SceneDeskWindow`/`SceneFlowWindow`/`SceneAppWindow`/`DrawBrowserChrome` TB 드로우 제거, `main.cpp` winChrome 람다 stub화, Tutorial·JobSelect contentY 조정, 부팅 애니메이션 TB 제거 + 로그 Y 재조정. Debug|x64 통과.
- **Main Menu UI**: 기존 헥사곤 버튼·진입 애니메이션 유지 + 별자리 프레임(코너 브래킷·glow) + 타이틀 다이아몬드 액센트 추가. Debug|x64 통과.
- **RUN CONFIG UI**: 별자리 프레임 + 블룸 셰이더 + 헥사곤 레이더 차트 + 무기 전환 모핑 애니메이션 완료
- **Settings UI**: 3탭 (GAME/AUDIO/SYSTEM), 진입 애니메이션 (헤더·카드 스태거·우측 패널), 리셋 다이얼로그, 완료 오버레이 구현 완료
- **Settings UI 폴리시**: 코너 브래킷 버튼·ON/OFF 텍스트 토글·탭-패널 연결선·텍스트 계층(영문 메인/한글 서브)·행 테두리 다이어트 적용
- **ASTRAL_LOG(Codex) UI**: CODEX.DB→ASTRAL_LOG 명칭 변경, 탭 ENTITIES/MODULES/APEX, 그룹 CLASS-I/CLASS-II/DETECTED, ○───● 별자리 리스트 스타일, 우측 패널 = 회전 노드 뷰어(38%) + 정보 블록(62%) 완료. 미해금 항목 밝기 조정(alpha 0.36→0.62)
- **ARMORY(Shop) UI**: 구 AppWindow 스타일 전면 교체. 현재 4탭(전체/영구/소총/위성), 좌측 별자리 스크롤 리스트, 우측 디테일 패널 — META 업그레이드(레벨바+버튼 기능), 테마 BUY/EQUIP(기능), 증강 카탈로그(인게임 픽 전용 안내) 완료.

### Open Issues

- `g_BatchAlpha` 페이드인이 일부 씬에만 적용됨. 다른 씬 전환에도 확장 필요.
- Settings 씬 게임 중 오버레이 모드(`settingsOverlay`) 동작 미검증.

### Next Candidates

**UI 작업 (우선순위 순)**
- 씬 전환 페이드를 다른 씬에도 확장 적용

**적 비주얼 재설계 (다음 대형 작업)**
- 성진/성군/성진류/펄사 — 점+선 기반 별자리 스타일 외형으로 교체
- 보스 FORK: 외형 리디자인 (기존 OS 외형 탈피)
- 보스 TESS: 패턴을 "세계의 끝" 컨셉으로 교체
- 코드 내 적 네이밍 교체 (비주얼 작업과 동시 진행)

**확정 개발 테마**
- 배경: 딥 네이비 (투명 오버레이 통해 실제 바탕화면 비침)
- 색조: 성운 색 (청보라 #6A4BE8, 시안 #35BFE0, 연보라 #A088F8)
- UI 형태 언어: 육각형, 별자리 점선, 다이아몬드 노드, glow 라인
- 폰트 계열: Chakra Petch (영문), 시스템 한글

---

## Change Log

### 2026-08-26

#### Codex

- Fixed Main Menu re-entry after RUN_CONFIG Back: cleared the delayed menu selection state after scene fade dispatch and reset Main Menu UI state from the RUN_CONFIG Back path.
- Applied A-lite UI family pass: shared left/radial vignette layers across RUN_CONFIG, ARMORY, ASTRAL_LOG, and SETTINGS, plus stronger constellation map lines/nodes with dark underlays.
- Polished Main/Sub-scene continuity: added Main Menu anchor line, reduced left-list panel fill weight in RUN_CONFIG/ARMORY/ASTRAL_LOG, and unified Settings title to CALIBRATION.
- Started floating UI redesign pass: ARMORY/ASTRAL_LOG/CALIBRATION left-side cards now lean on invisible hitboxes, anchor lines, focus markers, scanlines, and reduced panel fills; RUN_CONFIG weapon cards were lightly de-boxed while preserving layout.
- Restored ARMORY right-panel structure without returning to heavy cards: added product-code title anchors, terminal-style spec/quote rows, gold cost emphasis, and command-style transaction buttons with hover scan/slash feedback.
- Reframed ARMORY closer to Main Menu expansion: stronger left vignette, floating category/list treatment, lighter right-side hologram canvas, product-code anchors, and terminal quote rows while preserving existing hitboxes.
- Reworked ARMORY into a cascading text-tree layout: vertical root commands, depth-2 item branches with wide invisible hitboxes, a large constellation canvas, and a compact data-tag transaction readout instead of a heavy shop panel.
- Shifted ARMORY toward an in-Main-Menu expansion feel: Main Menu ARMORY now enters SHOP without black scene fade, and SHOP renders the ONEDOW/menu ghost sliding left while the ARMORY tree unfolds from that row.
- Build: MSBuild Debug|x64 passed.

---

### 2026-08-25

#### Codex

- Reworked Main Menu toward the current RUN CONFIG constellation style: vertex ONEDOW logo, large framed command panel, row-card menu layout, hover emphasis, constellation nodes/lines.
- Refined Main Menu hierarchy: stronger opaque command panel/cards, larger primary START row, unified button color, removed slider-like internal lines.
- Reworked Main Menu into an asymmetric title layout: removed the central command box visual, shifted menu commands left, made English system codes primary with Korean subtitles, added minimal hover guides, reactive right-side constellation morphs, and delayed click collapse before scene fade.
- Improved Main Menu readability: added a global left-side vignette for floating command text and reinforced the right constellation canvas with radial darkening, dark edge underlays, and layered node glow/white cores.
- Reworked ARMORY right panel toward RUN CONFIG style: credits moved into the detail panel, selected item detail gains stats/action layout plus constellation visualization.
- Reworked ARMORY readability after screenshot review: made tabs/detail surfaces more opaque, replaced the sparse dark detail pane with a brighter grouped layout, and added an empty-category state.
- Rebased ARMORY onto the actual RUN CONFIG layout math: same header/footer/left-mid-right column proportions, right detail panel starts at colY, category strip fills the left-only Y offset, and item rows render as compact RUN CONFIG-style cards.
- Tuned ARMORY after screenshot pass: larger category strip, brighter item card surfaces without changing text colors, selected-card pulse treatment, raised credits card, and larger Back button label.
- Adjusted ARMORY list readability: item/group text now uses white text while category/rarity color is carried by card fills, frames, markers, and selected-card pulse nodes.
- Changed ARMORY group headers into larger section cards so labels like RIFLE SYSTEMS are readable and visually bind the following item rows.
- Reduced ARMORY left-panel color saturation to sit closer to inactive RUN CONFIG rows: darker neutral tab/item surfaces, weaker rarity fills, and subtler frames/markers.
- Removed ARMORY left-list slider-like vertical bars from group headers and selected item cards.
- Improved ARMORY visibility pass: brighter selected-list focus, light-gray data labels, brighter module trace label, and boxed augment description text.
- Added ARMORY astral-map detail pass: STARDUST currency label, celestial data labels, constellation map naming, RA/DEC metadata, and subtle orbit arcs on detail frames.
- Localized active ARMORY astral-map labels for Korean/English switching; Japanese currently follows the existing English fallback in this scene.
- Reworked ASTRAL_LOG layout: moved category tabs above the left list, removed duplicate category titles from left/right panels, and replaced the empty detail hint with a faint astrolabe orbit grid.
- Matched ASTRAL_LOG category tabs to the RUN CONFIG/ARMORY strip language: contiguous dark tabs, low-saturation fills, selected bottom line, and subdued inactive text.
- Forced ASTRAL_LOG into the RUN CONFIG family look: removed closed rectangle borders from its main panels, rolled panel/frame colors back to cyan/teal, and converted the left codex list into wide selectable card rows.
- Enlarged ASTRAL_LOG scene panels: increased overall screen coverage, reduced header/footer dead space, and widened the left card-list column.
- Shifted ASTRAL_LOG closer to ARMORY layout: wider scene canvas, ARMORY-like header/footer proportions, category strip at the top of the left column, list panel below it, and a larger right detail panel.
- Polished ASTRAL_LOG density/readability: reduced the left/right gap, added a faint master frame, enlarged the left constellation viewer inside the right detail panel, and grouped title/description/stat text into translucent info containers.
- Removed redundant ASTRAL_LOG module rarity text from the detail title box; rarity remains available in existing module metadata.
- Enlarged the ASTRAL_LOG module detail title and added width-based scaling so long module names stay inside the title box.
- Increased ASTRAL_LOG left list/category text sizes and raised right-detail titles while enlarging description/data text for readability.
- Brightened ASTRAL_LOG panel, list-card, selected-row, and info-box surfaces slightly to improve readability without changing the overall cyan constellation style.
- ASTRAL_LOG panels now inherit the active category color, and each category's entry animation only plays once per game process.
- Settings scene first readability pass: brighter left tab/right panel surfaces, stronger row controls, larger tab/header text, and removed the left-tab slider-like selection bar.
- Reworked Settings into SYS_CALIBRATION: GAME now uses DISPLAY/CONTROL/LANGUAGE groups with code labels and segmented controls; AUDIO/SYSTEM use matching grouped preview layouts.
- Refined SYS_CALIBRATION OS theme: compact top-aligned tabs, cyan-fixed master frames/controls, tab color limited to small markers/tags, red limited to DANGER ZONE/reset, and added a non-interactive calibration node decoration.
- Expanded SYS_CALIBRATION layout, reordered SYSTEM groups to place DANGER ZONE last, replaced AUTO SAVE with a system-status log row, and added calibration-node click pulse feedback.
- Localized SYS_CALIBRATION primary labels for Korean/English switching: tabs, section headers, row names, toggle states, language options, linked-channel text, and system-status copy now react to the selected language.
- Build fix: restored missing AUG_SELECT orbit variables used by the constellation augmentation selection layout.
- Cleaned temporary workspace files and added ignore rules for verification folders, backup/temp C++ files, pasted screenshots, and ad hoc patch/find scripts.
- Build: MSBuild Debug|x64 passed.

---

### 2026-08-24

#### Claude

- Enemy visual redesign (Constellation roster: Process/Adware/DDoS/Botnet) — full implementation complete:
  - **Process (NORMAL)**: 솔리드 삼각형 → 아웃라인 삼각형만 (glow+core A+B), 크기 base×0.75, 스포크 3줄(0.25α)
  - **Adware (RangedMob)**: Pulsar 별자리 비주얼 → 외부 다이아몬드 아웃라인 + 내부 삼각형 아웃라인, 코너 브래킷 제거
  - **DDoS (DDOS)**: 솔리드 삼각형 → 세장형 아웃라인 삼각형(화살표 느낌, base×0.55 len/×0.22 wid), 회전버그 수정
  - **Botnet (SPAWNER)**: 다이아몬드 → 4방향 별 아웃라인(R1=base×1.55, R2=base×0.65), glow+core, 끝점 다이아몬드 액센트
- Adware rotation 버그 수정: `rotAngle` 필드 적분 방식으로 변경 (RangedMob.h), 이동 중 3× 가속, 이동 중 발사 금지
- 회전 위치 버그 수정: `worldX*0.01f` → `(float)((size_t)m % 628)*0.01f` (포인터 해시 기반 고정 오프셋)
- Glow+core (A+B) 기법 전 적 외형에 적용: 두꺼운 glow(α0.10~0.12) + 가는 core(α0.85~0.90) 레이어
- 퇴역 적 비주얼 코드 EntityDraw.cpp에서 전면 제거: SPLITTER/BLINKER/CHARGER/WEAVER/BRUTE/ORBITER/SHIELDED/BADSECTOR/REGERROR
- Augment.h: AugRemoved()에 퇴역 17개 증강/디버프 추가 (D_SPLITTER, D_BLINKER, D_ORBITER, D_SHIELDED 등)
- main.cpp: Adware 창 이름 `popup.exe` → `adware.node`
- Build: MSBuild Release|x64 passed (C4828 인코딩 경고만 — 기존).

---

### 2026-08-22

#### Codex

- Added `Docs/Enemy_Constellation_Concept.md` for the four-enemy constellation roster: Process, Adware, DDoS, Botnet.
- Changed Botnet/SPAWNER summon output from normal Process mobs to DDoS swarm shards.
- Updated Process, DDoS, and Botnet in-run visuals toward dot-line constellation silhouettes.

#### Claude

- DEV mode F key now increments playerLevel and resets XP before opening aug select (level-up pattern).
- AugSelect card layout overhauled: icon centered in top 60%, name+desc+key grouped in bottom 40%, inner border glow on hover, constellation frame intensity tied to hover/flash state.
- Codex ENTITY/MODULE/APEX right panels: added system-log key-value rows (THREAT_LV, PATTERN_ID, STATUS / MODULE_CLASS, TYPE_ID, ACQUISITION / CLASS, SIGNAL_ID, THREAT_LV) below existing content.
- Shop META tab: added diamond tier visualization (filled/empty/pulsing-next diamonds) below upgrade button.
- Shop AUG CATALOG tab: added rotating inner-orbit + outer-orbit diamond geo decoration in center panel.
- Settings reset dialog + complete overlay: title scale raised, separator thickened, body text scaled down and grayed for better typographic hierarchy.
- In-game HUD Issue 3: thin cyan safe-zone frame (7px inset, 1.5px wide, 9% alpha) drawn around screen edges during RUNNING/PAUSED. Thin vertical cyan connector line (1.5px, 11% alpha) drawn from HP panel bottom to skill-key row top.
- Scenes.cpp: added `const float now = (float)glfwGetTime();` at top of Scene_Shop to fix build errors.
- Build: Debug|x64 passed.

---

### 2026-08-21

#### Codex

- Reworked RUN CONFIG trial flow into: ON + count stepper -> dedicated TRIAL SELECT page -> final execution.
- Trial page requires exactly the requested count before EXECUTE RUN is enabled.
- Reworked player weapon visuals away from a shared base body: rotating dual-equilateral-triangle rifle core and static-field hex/orbit body.
- Removed Cannon from the demo weapon set: RUN CONFIG selection, ARMORY tab/catalog exposure, and cannon-specific augment availability are disabled while legacy enum/code branches remain for compatibility.
- Updated active player weapon silhouettes: Rifle is now Layered Cross-Star with two counter-rotating 4-point star frames and recoil vertex pulse; Static Field is now a resonance beacon with counter-rotating polygons, field pad, and limited target zaps.
- Added Rifle outer orbit volume: dashed orbit ring, two satellite nodes, four floating bracket fragments; removed the old translucent player rear circle/cross overlay.
- Fixed the in-game player unique window titlebar redraw path.
- Build: MSBuild Debug|x64 passed.

#### Claude

- Rebuilt Codex (ASTRAL_LOG) scene: ENTITIES/MODULES/APEX tabs, CLASS-I/II/DETECTED groups, ○───● list style, rotating node viewer + info block right panel.
- Rebuilt Shop (ARMORY) scene: current 4-tab layout (전체/영구/소총/위성), constellation scroll list, META upgrade / THEME buy-equip / AUG catalog detail panels, functional coin transactions.
- Fixed ASTRAL_LOG unseen-item text alpha (0.36 → 0.62) for readability.
- Added constellation theme to Main Menu (kept original hex buttons + intro animation intact): corner bracket glow frame around button group, diamond + connector accents on title.
- Removed ALL title bars: BROWSER_CHROME_H→0, FLOW_CHROME_TOP→10px, all four WindowChrome functions stripped of TB drawing, winChrome lambda stubbed, Tutorial/JobSelect contentY adjusted, boot animation TB removed + log Y recalibrated.
- Build: Debug|x64 passed.

---

### 2026-08-20

#### Codex

- Polished RUN CONFIG: OS-path subtitle, CREDITS panel, radar/stat readability, execute button, trial toggle.
- Changed normal mob spawn toward low early density with time-based ramping.
- Fixed Settings build error by replacing undefined rWk with rightWake.

#### Claude

- Rebuilt Settings scene as a 3-tab large UI with constellation frames/glow and reset modal.
- Restored source after encoding damage and recorded Release|x64 success.

---

### 2026-08-19

#### Codex

- Removed difficulty-select direction and moved toward RUN CONFIG loadout/trial/execute flow.
- Connected trial effects to score multiplier and actual run values.

#### Claude

- Worked on main menu/difficulty UI direction and enemy concept notes.

---

### 2026-08-26

#### Codex

- RUN_CONFIG 중앙 데이터 영역에 `네모 텍스처.png`를 패널 배경 마스크로 적용하고, 중앙 스탯 영역보다 넓게 배치.
- 패널 텍스처 알파 보정 로더 추가: PNG 내부의 낮은 알파를 2.5배 보정하여 면이 배경에 묻히지 않도록 처리.
- `ICON_CONFIG_PANEL`을 `Resource.rc`의 RCDATA로 등록하고 exe 내장 리소스를 우선 로드하도록 변경. 외부 파일 경로는 폴백으로 유지.
- 텍스처 파일을 `WiNILL/Icons/CONFIG_PANEL.png`에 추가하고, `Resource/Icons` 및 `bin/Resource/Icons`에도 실행용 파일 배치.
- 중앙 패널 렌더 순서를 텍스처 배경 -> 브라켓 -> 텍스트/스탯 순으로 정리하고 검정색 대비 면으로 조정.
- Build: MSBuild Debug|x64 `BUILD_OK`.
