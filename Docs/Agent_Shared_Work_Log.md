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
- Astral enemy blueprint now includes `GRAVIS` as a 3T gravity-field elite enemy.
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
- **UI 가시성 감사 + 전면 수정**: `SCENE_BG_ALPHA=0.0f` 버그 수정, 텍스트 인코딩 깨짐 수정, 다국어 힌트 추가, HP 경고·일시정지 텍스트 대비 강화, 스킬 HUD 박스 별자리 스타일화, GameOver/Victory 버튼 `DrawMenuCommandFeedback` 리뉴얼 완료. Release|x64 통과.

### Open Issues

- `g_BatchAlpha` 페이드인이 일부 씬에만 적용됨. 다른 씬 전환에도 확장 필요.
- Settings 씬 게임 중 오버레이 모드(`settingsOverlay`) 동작 미검증.
- GameOver/Victory 버튼 실제 클릭 QA 미완 (기존 `UIButton`에서 `DrawMenuCommandFeedback`으로 교체됨).

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

### 2026-08-31

#### Codex - Shared UI Depth Task 7 Verification

- Verified the merged Claude/Codex UI-depth work with a full MSBuild Debug|x64 build; `WiNILL/bin/WiNILL.exe` built successfully with no new errors.
- Runtime-checked the Main Menu and ARMORY category-entry states at desktop resolution. Primary command text remains dominant after ambient-layer pruning, and category focus/credits remain readable.
- Confirmed Task 8 ARMORY row behavior in source: Ambient-tier selected fill, Structural-tier drifting scanline, growing left focus bar, rarity-colored markers, and reduced text indent all use the shared depth helpers.
- Automated capture could not reliably enter the ARMORY item row because Windows DPI/fullscreen coordinates did not match the capture harness. The moving scanline/focus-bar animation still needs one manual runtime visual check; this is not a build blocker.

#### Codex - ASTRAL_LOG Archive Surface and Constellation Detail

- Rebuilt the inline ASTRAL_LOG content area as one filled archive surface, binding the record list and selected-record detail into a single hierarchy.
- Added an archive header with category, observed-record progress, and category-specific record labels.
- Added a deterministic nine-node constellation above the detail text, with distinct entity/module/apex geometry, orbital scaffolds, depth-weighted nodes, and rarity/category accent colors.
- Expanded the sparse detail tag into a readable record view with title, summary, class/status metadata, record type, and read-only state.
- Kept scrolling and hit testing constrained to the list column while preserving the existing inline entry/exit flow.
- Vertically centered the combined list/detail archive surface independently from the lower main-menu root commands, with a protected top margin for short viewports.
- Expanded the centered archive surface vertically by up to `72px * uiS` and raised inactive/unseen list text contrast while preserving selected-item emphasis.
- Strengthened archive constellation edges, nodes, and orbital guides; unseen records now retain a clearly readable silhouette instead of nearly disappearing.
- Increased archive header, list, title, summary, metadata, and status text scales while retaining width-based fitting for long localized strings.
- Build: MSBuild Debug|x64 passed; `git diff --check` passed.

---

### 2026-09-01

#### Codex - In-game HUD Signal Layer

- Kept `zwins`, WorldScissor regions, and collision geometry intact while removing opaque FakeWindow surfaces and heavy window chrome from enemy and boss regions.
- Replaced non-boss window visuals with lightweight constellation corner signals and retained only compact identification tags and 2px gauges where useful.
- Kept boss identity and health in the dedicated top HUD instead of duplicating it inside a world window.
- Removed the player FakeWindow background and its enclosing HP/EXP panel; retained the player shell and slim HP/EXP gauges as gameplay HUD.
- Replaced the in-game shop application-style window with a cyan interaction signal frame.
- Build: MSBuild Debug|x64 passed; only pre-existing `APIENTRY` redefinition and `LNK4098` warnings remain.

---

### 2026-09-01

#### Codex - Enemy constellation visual pass and debug spawn acceleration

- Rebuilt enemy silhouettes around shared `EnemyNodeAnchor` coordinates so frame endpoints, `CircleTexture` halos, bright star centers, and core alignment use the same positions.
- `ROTOR`: reduced to one rotating square frame with four smaller textured stars and a larger textured core.
- `DDOS` display path: reduced to a smaller rotating triangle with three nodes and a distinct warning core.
- `RangedMob` display path: replaced the nested triangles with a circular observation instrument using partial arcs, dotted orbit segments, four textured markers, and a larger core pulse during BURST.
- `SPAWNER` display path: simplified to a restrained three-node generation core while preserving its existing gameplay FSM.
- Reduced line glow width and flushed pending batch geometry before each icon draw to keep textured halos behind their owning stars and prevent pass-order artifacts.
- Added Debug-only enemy spawn acceleration: four-times spawn-curve progression and 0.45x normal/ranged spawn intervals. Release values remain unchanged.
- Resource check: `ICON_CONSTELLATION_LINE` and `ICON_CONSTELLATION_CIRCLE` are embedded through `WiNILL/Resource.rc`.
- Build: MSBuild Debug|x64 `BUILD_OK`; existing `APIENTRY` and `LNK4098` warnings only.

---

### 2026-09-01

#### Codex - Astral Enemy Visual Prototype

- Reworked the existing enemy render branches without changing `MobKind`, spawn rates, HP, collision, or attack behavior.
- Added shared `DrawEnemyWirePolygon`, `DrawEnemyNode`, and `DrawEnemyCore` helpers in `WiNILL/Render/EntityDraw.cpp`.
- Updated `NORMAL` (The Rotor) with layered counter-rotating triangular frames, tip nodes, and a readable bright core.
- Updated `DDOS` (The Node) with an incomplete jittering signal graph, pulsing nodes, and warning core instead of a closed static diamond.
- Updated `SPAWNER` (The Hive) with a six-node constellation, radial links, stronger open/spawn readability, and phase-aware core feedback.
- Routed enemy node/core halos through `g_ConstellationCircleTex` with a primitive fallback, so the prototype follows the background constellation texture path even when the packaged texture is unavailable.
- Kept the legacy NORMAL star geometry as a hidden fallback while making the visible silhouette two counter-rotating triangular frames with tip nodes and a central core.
- Replaced the `RangedMob` (The Lens) square-layer render path with the same triangular constellation language while preserving its idle/charging/burst state feedback.
- Kept the existing enemy FSMs, collision, spawn behavior, and combat values unchanged; this pass is visual-only.
- Build: MSBuild Debug|x64 passed; `git diff --check` passed for the touched source/doc files. A pre-existing malformed `Scenes.cpp` stat-string block and missing closing brace were minimally repaired to restore compilation.

---

### 2026-08-30

#### Codex - ARMORY Two-Stage Browser and Astral Data Plates

- Replaced ARMORY's simultaneous category/list/detail presentation with a two-stage flow: category selection first, then an item browser in the same left command slot plus one right-side detail readout.
- Added forward/reverse browse interpolation, transition input locking, and hierarchical ESC/right-click behavior: items return to categories before categories return to the main menu.
- Kept category and item hitboxes wide while preventing the left command rail from consuming the detail area.
- Added filled `AstralDataPlate` surfaces with restrained dual-layer navy fills, cut-corner accents, partial brackets, and asymmetric markers to ARMORY, ASTRAL_LOG, and CALIBRATION information regions.
- Anchored ARMORY's compact transaction tag directly to the detail plate after removal of the right-side constellation visualization, and reduced the plate to the actual data height.
- Embedded `LineTexture.png` and `CircleTexture.png` into the executable resources with file-path fallback loading, so constellation masks no longer depend on the launch directory.
- Standardized the only runtime output at `WiNILL/bin/WiNILL.exe`; the stale root-bin executable was removed by the clean rebuild so both developers no longer launch different builds.
- Verified the rebuilt executable contains the embedded Line, Circle, Panel, and LeftGradient PNG resources at their full source byte sizes.
- Raised Astral Data Plate fills from hardcoded near-black to theme-derived blue-gray. After runtime review, limited the embedded Panel texture to a low-alpha edge treatment instead of stretching its bright center across the full surface.
- Reduced ARMORY's detail plate from a mostly empty full-width region to a bounded 860x310 UI-scale area while preserving the transaction tag, credit readout, and wide logical hitboxes.
- Locked shared data-plate fills to neutral black while retaining their existing alpha.
- Replaced the ARMORY-only vertical lift with a shared `MainMenuCommandStartY` anchor used by the main menu, ARMORY, ASTRAL_LOG, RUN_CONFIG, and inline SETTINGS.
- Unified ARMORY, ASTRAL_LOG, and inline SETTINGS background contrast through one shared main-menu dim pass; removed their scene-specific central/right radial dim fields so inline transitions no longer change the background brightness profile.
- Build: MSBuild Debug|x64 passed; output `WiNILL/bin/WiNILL.exe`.

#### Codex - Main Menu Visual-Depth Pruning and Motion Profiles

- Removed `DrawMenuCommandAstralAura` and its per-command miniature constellation, wake, and dust rendering.
- Reduced the main-menu ambient particle pool from 55 to 30 particles (`REDUCED` VFX renders 18) and capped pulse alpha through `PulsedAmbientAlpha`.
- Removed the 42-star/12-link layer from `DrawMainMenuAstralVeil`; retained only its readability fields so the menu no longer runs two ambient star systems simultaneously.
- Added frame-rate-independent `ConstellationMotionState` interpolation with field-specific settle thresholds and an integrated motion phase.
- Added menu-specific orbital behavior profiles: RUN_CONFIG convergence, ARMORY expansion, ASTRAL_LOG stabilization, CALIBRATION alignment, and SHUTDOWN decay.
- Build: MSBuild Debug|x64 passed; output `WiNILL/bin/WiNILL.exe` updated at `2026-08-30 21:59:01`.

---

### 2026-08-27

#### Claude

- **UI 가시성 감사 보고서 작성**: Scenes.cpp·main.cpp·SceneUI.cpp·UiColors.h 분석. Critical 1건, High 3건, Medium 3건, Low 3건 도출.
- **[C-1] SCENE_BG_ALPHA 버그 수정** (`Scenes.cpp`): `constexpr float SCENE_BG_ALPHA = 0.0f`가 GameOver·Victory 배경 딤을 완전히 무효화하던 문제 수정. `* SCENE_BG_ALPHA` 곱셈 제거 → 배경 딤 정상 동작.
- **[H-2] 증강 힌트 텍스트 인코딩 수정** (`main.cpp`): L"[ SPACE ]  ★★★★ ★★★★" (EUC-KR 원본 "증강 픽업"이 UTF-8 재해석으로 U+FFFD×8개로 깨짐) → 다국어 배열 `kAugHint[3]` (KR/EN/JP) 로 교체.
- **[H-3] 조작 힌트 다국어 추가** (`main.cpp`): 세 언어 모두 동일한 영어였던 조작 힌트를 `kCtrlHint[3]` 배열로 분리. KR: `WASD 이동 / 마우스 사격 / SHIFT 대시 / Q/E/R 스킬 / ESC 일시정지`, JP: 일본어 대응 추가.
- **[M-1] HP 위험 경고 알파 강화** (`main.cpp`): 계수 `(0.08+0.13×pulse)×(0.5+0.5×sev)` → `(0.12+0.22×pulse)×(0.6+0.4×sev)`. 최대 알파 `0.21 → 0.34`.
- **[M-2] Pause 씬 텍스트 대비 향상** (`Scenes.cpp`): 서브타이틀 알파 `0.60→0.80` + 색 밝힘, 하단 힌트 알파 `0.55→0.72` + 색 밝힘.
- **[L-3] 안전지대 가이드라인 제거** (`main.cpp`): alpha 0.09로 사실상 보이지 않던 화면 테두리 가이드라인 블록 전면 제거.
- **[L-2] 스킬 HUD 박스 별자리 스타일화** (`main.cpp`): `drawRect 0.88` 불투명 배경 → 투명도 `0.22~0.40` 배경 + 8-코너 브래킷. 전체 투명 오버레이 스타일과 통일.
- **[L-1] Settings 헤더 프로필 박스 패널 제거** (`Scenes.cpp`): `drawRect+drawConstellFrame` 패널 → 플로팅 텍스트 + 1px 하단 구분선. 패널리스 방향 통일.
- **[H-1] GameOver/Victory 버튼 리뉴얼** (`Scenes.cpp`): `UIButton` (구형 사각형 테두리) → `DrawMenuCommandFeedback` 좌정렬 별자리 스타일. GameOver=청색 accent, Victory=녹색 accent. 호버 슬라이드·앵커 라인·정적 호버 상태 포함. `Scene_Victory`에 누락된 `delta` 추출 추가.
- Build: MSBuild Release|x64 통과. 기존 APIENTRY 경고만 존재.

---

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

#### Codex - Panel Texture Coverage

- Reused the embedded `g_ConfigPanelTex` alpha-mask behind inline ARMORY list/detail regions.
- Added the same mask behind ASTRAL_LOG's list and read-only data tag.
- Added the mask behind the inline settings detail panel; existing text, constellation, and bracket layers remain above it.
- Build: MSBuild Debug|x64 `BUILD_OK`.

---

### 2026-08-27

#### Codex - UI Visibility Review and Fixes

- Reviewed main-menu inline UI, ARMORY, ASTRAL_LOG, and RUN_CONFIG visibility/input risks without duplicating the Claude issue matrix.
- Removed the RUN_CONFIG-specific background darkening during inline handoff to prevent brightness flashes.
- Removed the duplicate ARMORY left vignette pass used during inline entry.
- Tightened the RUN_CONFIG contrast texture to the stat-row bounding area and lowered its alpha.
- Reduced main-menu and RUN_CONFIG weapon/BACK hitboxes to avoid visual-area overlap with neighboring panels.
- Build: MSBuild Debug|x64 `BUILD_OK`.

#### Codex - Shared Inline UI Readability Pass

- Extended the bounded contrast and text-shadow treatment beyond the main menu.
- RUN_CONFIG: weapon choices, BACK/PLAY, loadout title, stat labels, and values now use the shared readable-text pass.
- ARMORY: root commands, item groups/list entries, product header, transaction rows, credits, action buttons, and compact data tags now use the same pass.
- ASTRAL_LOG: root commands, archive groups/list entries, and decrypted detail text now retain contrast over bright backgrounds.
- CALIBRATION: root tabs, group selectors, detail title, setting rows, and return hint now use the shared pass.
- Added borderless, bounded dark surfaces behind the actual RUN_CONFIG stats, ARMORY list/detail, ASTRAL_LOG list/tag, and CALIBRATION selector/detail regions; these replace weak texture-only contrast on bright backgrounds.
- Kept the pass out of the combat HUD to avoid doubling text rendering across high-frequency gameplay information.
- Build: full MSBuild Debug|x64 rebuild passed; both executable copies updated at `2026-08-27 21:27:21`.

#### Codex - Shared Menu Interaction System

- Added common helpers for command-button hover interpolation, top-to-bottom reveal timing, and rendering feedback.
- Unified the interaction style used by the main menu, ARMORY/ASTRAL_LOG/CALIBRATION root commands, RUN_CONFIG weapon/BACK/PLAY commands, and Game Over/Victory actions.
- Standardized command entrance timing to a `0.14s` stagger and hover interpolation to the same response curve, including the left guide, scan line, node marker, and click pulse.
- Applied the shared smooth focus transition to ARMORY and ASTRAL_LOG item lists while preserving their existing wide logical hitboxes.
- Kept sliders and segmented selectors visually distinct because their control semantics differ, while sharing the same hover timing and active-state contrast rules.
- Build: full MSBuild Debug|x64 rebuild passed; both executable copies updated at `2026-08-27 23:00:50`.

---

### 2026-08-28

#### Codex - Shared Three-Column Inline Layout

- Added a shared `InlineThreeColumnLayout` scaffold for non-game inline screens.
- Repositioned ARMORY, ASTRAL_LOG, CALIBRATION, and RUN_CONFIG onto the same root/content/detail column anchors and common top/bottom bounds.
- Removed ARMORY and ASTRAL_LOG content-column Y movement that depended on the selected category, keeping the workspace stable while the selected data changes.
- Moved the ASTRAL_LOG read-only data tag from the detached lower-right position to the shared detail-column baseline.
- Anchored RUN_CONFIG stat readout to the shared content column and PLAY to the shared detail-column endpoint.
- Runtime-checked ARMORY, ASTRAL_LOG, and CALIBRATION at `2560x1599`; root navigation, content, and detail columns now follow the same spatial hierarchy.
- Build: full MSBuild Debug|x64 rebuild passed.

#### Codex - RUN_CONFIG Combat Constellation Reference

- Detached inline RUN_CONFIG from the provisional shared three-column layout.
- Replaced the rectangular stat panel and six horizontal bars with a central weapon-power constellation: three hex measurement rings, six animated stat axes, connected value nodes, and a rotating energy core.
- Added weapon-specific abstract previews without gameplay projectile spawning: repeated linear impulses for RIFLE and expanding radial pulses for STATIC FIELD.
- Kept the main-menu-derived left weapon/BACK controls, wide logical hitboxes, reverse exit flow, and PLAY input lock behavior intact.
- Connected PLAY to the power core as the terminal node of the composition and retained the existing 0.5-second launch transition.
- Added local radial contrast behind the core and weapon header instead of reintroducing a large rectangular application panel.
- Build: full MSBuild Debug|x64 rebuild and final incremental Debug|x64 build passed.

#### Codex - Constellation Texture and Density Pass

- Routed shared constellation nodes through the supplied `CircleTexture.png` mask, including dark halos, colored energy discs, orbit satellites, stat nodes, and weapon-preview particles.
- Routed shared constellation edges through `LineTexture.png` with a dark separation pass and tinted luminous pass; retained primitive fallbacks when either texture cannot load.
- Enriched the RUN_CONFIG power profile with a counter-rotating outer hex cage, an 18-node dotted orbital belt, four measurement ticks per stat axis, and layered core energy discs.
- Expanded the weapon behavior previews: RIFLE now carries moving textured energy packets with line trails, while STATIC FIELD emits three rotating rings of textured pulse particles.
- Build: MSBuild Debug|x64 `BUILD_OK`.

#### Codex - RUN_CONFIG Orbital Power Sphere

- Added a fake-3D orbital instrument behind the six-axis weapon profile, based on the supplied spherical constellation reference.
- The instrument combines a dense latitude/longitude wire globe, three independently tilted orbital planes, 54 stable dust particles, and four high-luminance orbit stars.
- Added depth-weighted line/node alpha and mild perspective scaling so front and rear orbital segments separate without a 3D renderer or FBO.
- Kept the six-axis polygon and labels as the foreground information layer, preserving actual stat readability over the new ambient structure.
- Used stable procedural samples rather than per-frame randomness to prevent particle flicker and allocation churn.
- Build: MSBuild Debug|x64 `BUILD_OK`.

#### Codex - Main Menu Orbital Sphere Canvas

- Reused the RUN_CONFIG orbital power sphere as the main menu's large right-side ambient instrument.
- Removed the previous menu-specific 10-node morphing constellation and its coordinate/edge state entirely; the orbital sphere is now the sole primary canvas object.
- Added two broken outer instrument rings, 24 radial calibration ticks, a focus-driven bright satellite, and a short satellite trail around the wire globe, three orbital planes, 54 dust particles, and four bright stars.
- Connected menu focus to the sphere phase/color response and slightly expands the instrument on hover.
- Added subtle inverse mouse parallax while preserving the existing center-collapse scene transition.
- Removed the previous single dotted ellipse so the new orbital paths do not overlap with redundant decoration.
- Replaced the pale cyan main-menu palette with a more saturated cobalt-blue range and updated the shared dark-theme Frame/Accent/NodeGlow tokens to match.
- Added a dark separation pass behind the orbital globe's dense latitude/longitude lines so the instrument remains readable over bright scene backgrounds.
- Raised all orbital motion to a shared `1.8x` speed scale: globe rotation, orbital planes, dust, bright stars, outer instrument rings, and focus satellite motion now accelerate together.
- Rebuilt the main-menu instrument again as a cleaner five-plane armillary system: removed the outer calibration rings/ticks and fixed focus satellite.
- Each root command now owns one orbital plane and one representative star; hover brings that plane forward with added thickness, brightness, and speed.
- Added an 84-particle volumetric shell and a compact wireframe stellar core so the composition reads as one celestial mechanism instead of stacked UI decoration.
- Polished menu focus as continuous per-orbit weights, allowing the previous orbit to settle while the next orbit gains thickness, brightness, and speed without snapping.
- Added rear-orbit occlusion through the stellar core, short trails behind active stars, and longitude lines on the core to improve depth and spherical readability.
- Kept the RUN_CONFIG power-profile renderer unchanged.
- Build: MSBuild Debug|x64 `BUILD_OK`.

#### Codex - Main Menu Astral Readability Layer

- Added a borderless translucent astral veil behind the main-menu command area instead of adding individual button cards.
- The veil uses two oversized radial fields, 42 slowly drifting ambient stars, and 12 faint constellation links so it reads as part of the scene background rather than a UI panel.
- Added a command-local hover aura: a dark optical falloff, a restrained accent field, and a three-node micro-constellation appear behind only the focused command.
- Raised primary and secondary command text alpha and strengthened their shadow separation while preserving the existing wide hitboxes, reveal timing, click pulse, and orbital canvas.
- Build: MSBuild Debug|x64 passed. Runtime window creation passed; pixel capture was unavailable because desktop screenshot permission was not granted.
- Removed the broad `LeftGradient.png` hover fill after runtime review showed it as a detached gray card; hover now uses three short `LineTexture.png` energy wakes.
- Routed miniature constellation halos, cores, and 8 orbiting dust particles through `CircleTexture.png`; logo constellation nodes now use the same texture-backed layering instead of primitive diamonds.
- Reduced representative-star and central-core shadow radii/alpha so texture-backed halos no longer read as large gray stains on bright desktop backgrounds.
- Retained `LineTexture.png` for constellation links and kept primitive gradient/geometry paths only as missing-resource fallbacks.
- Build: MSBuild Debug|x64 passed.

---

### 2026-08-31

#### Codex - ARMORY Unified Work Surface

- Reworked the active ARMORY browsing layout to use one RUN_CONFIG-sized dark work surface for the item tree and detail view.
- Moved the item list into a dedicated center column inside that surface and added a faint divider to establish the list-to-detail reading flow.
- Removed the duplicated list/detail panel treatment; the remaining list tint is intentionally low-alpha so it does not become a second card.
- Kept the existing category navigation, scroll range, item hitboxes, purchase/equip transactions, node map, credit display, and action-button feedback intact.
- Added a compact ARMORY assembly header and visible node count, and raised inactive/group item text contrast for bright-background readability.
- Build: MSBuild Debug|x64 passed; `git diff --check` passed for `Scenes.cpp`.
- Reordered the active ARMORY detail surface so its constellation map occupies the upper area and the selected node's data tag sits in the lower-right with a safe margin.
- Removed the duplicate product header from the active detail path and simplified `DESC:`/`STAT:`-style metadata into a continuous readout while preserving purchase, equip, and catalog actions.
- Build: MSBuild Debug|x64 passed.
- Enlarged the active ARMORY detail tag typography and moved the tag left inside the detail area so it sits close to the augmentation list instead of drifting toward the corner.
- Increased the tag's vertical breathing room and kept its purchase command hitbox aligned with the new layout.
- Build: MSBuild Debug|x64 passed; `git diff --check` passed.
- Moved the active ARMORY detail tag further toward the upper-left of the right detail area while leaving the category rail and constellation map positions unchanged.
- Replaced the partial detail-tag marks with a single corner-bracket frame around the entire explanation block, keeping the existing transparent surface treatment and command hitbox.
- Build: MSBuild Debug|x64 passed.
- Rebalanced the active ARMORY detail column into two full-width horizontal bars: the constellation map occupies the upper bar and the selected node explanation occupies the lower bar.
- Docked the explanation tag to the lower bar with a full-width bracket frame, while preserving the left category list, map styling, and purchase/equip hitboxes.
- Build: MSBuild Debug|x64 passed.
- Increased the active ARMORY header, category/list labels, constellation label, and detail-tag typography for stronger readability without changing the established layout.
- Kept row heights and command hitboxes stable, and updated the hover underline width calculation to match the enlarged command text.
- Build: MSBuild Debug|x64 passed; `git diff --check` passed.
- Enlarged the active ARMORY text scale by a further 1.3x across the assembly header, node list, constellation label, and detail tag.
- Preserved the existing panel dimensions, row spacing, and input hitboxes so the larger type does not change interaction geometry.
- Build: MSBuild Debug|x64 passed; `git diff --check` passed.
- Restored the ARMORY assembly/list typography to its prior scale and limited the size adjustment to the active detail explanation tag.
- Reduced the detail tag text to approximately 85% of the previous enlarged scale while keeping its full-width lower-bar layout and input geometry unchanged.
- Build: MSBuild Debug|x64 passed; `git diff --check` passed.
- Reworked the RUN_CONFIG trial-count control from a filled rectangular arrow box into a lightweight bracket navigator.
- Replaced text arrows with line-drawn chevrons, animated their hover length/brightness, and kept the existing left/right hitboxes and count display intact.
- Build: MSBuild Debug|x64 passed; `git diff --check` passed.

#### Codex - In-game CircleTexture sight markers

- Removed non-boss world-window identifier text so enemies no longer display detached names near the upper-left of their regions.
- Added large `CircleTexture` sight markers at the exact world-space center of normal, summoned, elite, and ranged enemies.
- Rendered the textured sight marker before each enemy silhouette, leaving the core, constellation nodes, and gameplay scissor/collision regions unchanged.
- Kept the marker low-alpha and gently pulsed so it replaces the old window backdrop without hiding the enemy body.
- Build: MSBuild Debug|x64 passed; existing `APIENTRY` redefinition and `LNK4098` warnings only.

#### Codex - GENESIS Sequential Spawn and SCOPE Scale

- Increased `GENESIS` (`SPAWNER`) body scale from `1.8x` to `2.2x` so its summon core and constellation frame read clearly in the arena.
- Changed the summon phase from a four-unit burst to one `ROTOR` emitted every `0.22s` after the opening animation completes; the existing cycle repeats after the hive closes.
- Matched `SCOPE` body and rear sight sizing to the `ROTOR` visual baseline instead of the oversized fake-window width calculation.
- Build: MSBuild Debug|x64 passed; existing `APIENTRY` redefinition warning only.

#### Codex - SCOPE and SWARM Balance Pass

- Increased `SCOPE` (`RangedMob`) visual baseline from `16px` to `25.6px` (`1.6x`) and centralized the value as `RangedMob::VISUAL_BASE_PX` for both body and rear sight rendering.
- Increased `SCOPE` base HP from `150` to `360` (`2.4x`) so its ranged role is not erased by a single early attack cycle.
- Increased `SWARM` (`DDOS`) size scale from `0.5x` to `1.0x`, matching the regular `ROTOR` body scale while preserving its smaller-node constellation design.
- Build: MSBuild Debug|x64 passed; existing `APIENTRY` redefinition and `LNK4098` warnings only.

---

### 2026-09-02

#### Codex - Early Enemy Roster Visibility

- Removed score gates for `SPAWNER/GENESIS` and `DDOS/SWARM` so all four active enemy silhouettes can appear from the first run.
- Removed the ranged-mob warm-up delay; ranged enemies now follow their normal spawn interval from run start.
- Kept conversion probabilities, time-based DDoS pressure/cap ramp, boss suppression, and roster caps so the opening does not flood immediately.
- Build: MSBuild Debug|x64 passed; existing `APIENTRY` redefinition and `LNK4098` warnings only.

- Increased the enemy sight-marker radius by roughly 25% and raised its base alpha from `0.10` to `0.16`, while preserving the core/node halo levels.
- Build: MSBuild Debug|x64 passed.
- Added a centered player `CircleTexture` sight marker behind the weapon shell, with a restrained cyan pulse and no change to player hitboxes or gameplay coordinates.
- Kept enemy markers low-alpha so the player remains the strongest readable gameplay anchor instead of making every enemy equally bright.
- Build: MSBuild Debug|x64 passed; existing `APIENTRY` redefinition warning only.
- Removed the idle pulse from enemy and player sight textures so `CircleTexture` reads as a persistent identity field instead of a projectile appearing and disappearing.
- Ranged enemies only raise the marker alpha during `BURST`; normal gameplay keeps a fixed low-alpha field.
- Build: MSBuild Debug|x64 passed; existing `LNK4098` warning only.

#### Codex - Global In-Game Enemy/Bullet Visibility

- Replaced the per-fake-window enemy, bomber, bullet, enemy-part, and approach-orb scissor passes with one global world-space combat pass.
- Removed the player-window, ranged-window, and turret-window clipping from those render paths; `zwins` remains available for collision, gauges, and signal-frame metadata.
- Converted turret body/gauge rendering to world-space coordinates so the turret remains visible without its window scissor.
- Converted VOLLEY, Tesseract, Ether Sword, and FORK.worm rendering to single full-arena passes, preserving boss telegraphs and task-kill rendering.
- Build: MSBuild Debug|x64 passed; existing `LNK4098` linker warning only.

---

### 2026-09-01

#### Codex - Dual CircleTexture Sight Separation

- Added a black rear `CircleTexture` behind enemy and player sight markers at 1.8x the foreground marker size, using the same world-space center to prevent visual drift.
- Preserved the colored foreground `CircleTexture`; texture-free fallback now uses the same two-layer order with circles.
- Removed the visible player fake-window/layout frame while keeping `playerWin` for gameplay coordinates, collision, and sizing logic.
- Build: MSBuild Debug|x64 passed; existing `LNK4098` linker warning only.
- Fixed the dark rear-layer path: it now requires both the embedded `CircleTexture` and icon shader, otherwise it falls back to world-space circles instead of silently disappearing.
- Raised rear-layer alpha independently to `clamp(alpha * 1.6, 0.16, 0.42)` so the black 1.8x separation layer remains visible instead of inheriting near-zero foreground alpha.
- Build: MSBuild Debug|x64 passed; existing `APIENTRY` redefinition and `LNK4098` warnings only.
- Isolated the black rear-layer diagnostic to the player only; enemy rear layers are disabled for this test.
- Forced the player's rear `CircleTexture` tint alpha to `1.0` while retaining the 1.8x size, so a missing ring now indicates a texture/resource, shader, blend, or dark-background issue rather than low alpha.
- Verified the source `CircleTexture.png` has an opaque center (`alpha 255`) with a feathered edge; the asset itself is not uniformly low-alpha.
- Build: MSBuild Debug|x64 passed; latest standalone launch reached the gameplay window for visual inspection.

#### Codex - Global Enemy Rear CircleTexture Prepass

- Split the black rear CircleTexture from the player marker so the player rear field is rendered separately from the colored foreground marker.
- Added a global rear-field prepass before enemy bodies: player, normal/summoned/elite monsters, ranged mobs, and bombers all receive a black CircleTexture at `1.8x` their foreground sight-field radius.
- Kept rear fields out of per-entity body rendering, preventing one mob's black field from drawing over neighboring mob silhouettes.
- Preserved the existing colored foreground CircleTexture, enemy constellation shapes, player weapon shell, and gameplay hitboxes.
- Build: MSBuild Debug|x64 passed; existing `APIENTRY` redefinition and `LNK4098` warnings only.
- Tuned the black rear CircleTexture opacity from `1.0` to `0.5` while keeping the `1.8x` radius and global prepass ordering unchanged.
- Build: MSBuild Debug|x64 passed; existing `APIENTRY` redefinition warning only.
- Adjusted the black rear CircleTexture to alpha `0.7` and reduced its shared radius scale from `1.8x` to `1.4x`, including the player marker.
- Reduced the shared regular-enemy HP ramp to `0.80x`; this affects normal, special, elite, and ranged mob spawn HP while leaving boss HP and progression/trial multipliers unchanged.
- Disabled enhanced mob variants such as Swift/Tanky in the runtime spawn path; the legacy `MakeElite` API remains only for compatibility and is no longer invoked.
- Replaced per-entity rear CircleTexture flushes with one batched icon draw and viewport culling, preserving the player/enemy marker sizes and draw order while reducing frame-time spikes.
- Reused the rear-marker CPU buffers across frames to avoid repeated heap allocations while the mob count changes.
- Build: MSBuild Debug|x64 passed; existing `APIENTRY` redefinition and `LNK4098` warnings only.
