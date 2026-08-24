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
- bin_verify/ is a previous verification output folder, not source.
- WiNILL/main.cpp.bak and WiNILL/main_temp.cpp are not build targets; avoid touching them unless explicitly needed.
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
- Latest Debug|x64 build passed.

### Open Issues

- Visual/click QA still needed for RUN CONFIG and TRIAL SELECT.
- Trial descriptions and actual effect severity need playtest validation.
- Verify ON/OFF, Back, target-count changes, and launch-time clearing behavior.
- Decide whether to remove bin_verify/.

### Next Candidates

- Screenshot-based RUN CONFIG/TRIAL SELECT polish.
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
