# Agent Shared Work Log

## 사용 규칙 - LOCKED

이 섹션은 최초 작성 규칙이므로 수정하지 않는다.

1. Codex와 Claude는 각자 지정된 작업 구역만 수정한다.
2. 다른 에이전트의 구역은 삭제, 축약, 재작성하지 않는다.
3. 공통으로 알아야 할 내용은 `Shared Handoff`에 날짜와 작성자를 붙여 추가한다.
4. 충돌, 미확인 사항, 빌드 실패는 숨기지 말고 `Shared Handoff` 또는 자기 구역의 `Open Issues`에 남긴다.
5. 작업 기록은 최신 항목이 위로 오게 추가한다.
6. 사용 규칙 변경이 필요하면 문서를 직접 고치지 말고 사용자에게 먼저 확인받는다.

---

## Shared Handoff

### 2026-08-20 / Claude

**RUN CONFIG 씬 전면 재설계 — 별자리 UI + 레이더 차트 + 무기 전환 애니메이션**

**개발 테마 확정**
- 게임 전체 비주얼 방향: 딥 네이비 + 성운 색(청보라·시안) + 별자리 점선 UI
- UI 언어: `drawConstellFrame` (코너 브래킷 + 다이아몬드 노드), `SetGlowFx(uFx==2)` 블룸
- 적 테마: 밤하늘 별자리 존재 (성진/성군/성진류/펄사)
- 보스 테마: 다크 사이버/별자리 하이브리드

**RUN CONFIG UI 작업 (4단계)**

1. **별자리 스타일 UI 테두리** — `drawConstellFrame` + `SetGlowFx` 전면 적용
   - 무기 카드: 고정 높이 100px, gap 24px, 컬러 스트라이프 + 프레임 glow
   - 우측 패널: 프로파일/노트/스탯 각 서브카드에 별자리 프레임
   - 시련 토글 카드: 별자리 프레임 + ON/OFF 버튼

2. **UI 블룸 셰이더 추가** — `MainShader.cpp`에 `uFx==2` 브랜치
   - 어두운 바디는 사실상 boost 없음(luma≈0), 밝은 accent 색만 시각적 발광
   - `DrawPrim.h`에 `extern GLint g_MainFxLoc` + `SetGlowFx(bool)` 헬퍼 추가

3. **레이더 차트 스탯 시각화** — 3×2 그리드 → 헥사곤 레이더 차트로 교체
   - `WDetailDef.norm[6]` 정규화 값 추가 (소총/화포/정전기장)
   - 4단계 그리드 링, 6축 선, 폴리곤 fill + 외곽선 + 버텍스 다이아몬드
   - 우측: 라벨 + 수치 6행 리스트 (방향 자동 정렬 텍스트 포지션)

4. **무기 전환 트랜지션 애니메이션**
   - 레이더 모핑: `s_RadarCur[6]` 매 프레임 `UiApproach` lerp (speed=7)
   - 패널 콘텐츠 페이드: `s_PanelFadeT` — 무기 변경 시 0 리셋 → 5.5×dt로 1까지 상승
   - `g_BatchAlpha` 범위 제어로 우측 패널 전체에 적용
   - 처음 무기 클릭 시 레이더 0→폴리곤 펼쳐짐, 전환 시 flicker+morph

**폰트 크기 정리**
- "RUN CONFIG" 헤더: 1.36f → 1.00f
- 우측 패널 무기명: 1.50f → 1.10f (역할 태그 Y도 58→44 조정)
- ON/OFF 버튼: 0.70f → 0.55f
- 점수 배율 표시: 0.68f → 0.52f

관련 파일:
- `WiNILL/Core/Scenes/Scenes.cpp`
- `WiNILL/Render/DrawPrim.h`
- `WiNILL/Render/MainShader.cpp`

---

### 2026-08-19 / Claude

**브라우저 크롬 + 씬 전환 + 배경 시스템 대규모 개편**

- `BROWSER_CHROME_H` 64px 탭 시스템 → 31px 단일 주소창 바로 교체
- 타이틀바 진행 바 추가: 보스전 중 보스 HP / 일반 플레이 중 다음 보스 등장 점수 진행률
- `DrawBrowserChrome` 호출 위치를 `[7]` UI 블록 최상단 렌더로 이동 (모든 상태에서 항상 표시)
- `SceneContext.h`에 씬 페이드 전환 시스템 추가 (`g_FadeAlpha`, `g_FadeDir`, `StartSceneFade`)
- 메인메뉴 "시작" 클릭 → 타이틀+버튼만 알파 페이드아웃, RUN CONFIG 패널은 알파 페이드인
- 페이드 타이밍: `FADE_OUT_DUR = 0.35f`, `FADE_IN_DUR = 0.35f` (총 0.7초)
- `g_BatchAlpha` 전역 알파 배율을 `DrawPrim.h`에 추가, `TextRenderer.h`도 반영
- `DrawMenuBackground()` 공용 함수 분리 — 파티클+스캔라인+비네트를 모든 메뉴 씬에서 공유
- `SceneFlowWindow` / `SceneAppWindow`에 `dimAlpha`, `bodyAlpha`, `shadowAlpha` 파라미터 추가
- 메뉴 씬 전체(Shop, Codex, Tutorial, Settings, JobSelect, DifficultySelect, CreativeConfig)에서 전체화면 딤 오버레이 제거
- HUD 상단 기준 `8.0f` → `BROWSER_CHROME_H + 4.0f` (타이틀바 아래로 이동)
- 플레이어 위치 상단 클램프: 플레이어 중심점이 타이틀바 아래에 위치하도록 (창 경계 기준 아님)
- 대시 목적지도 동일하게 클램프 적용
- RUN CONFIG 씬 하단 버튼 열 우측에 DEV MODE 토글 버튼 추가

관련 파일:
- `WiNILL/Core/Scenes/SceneContext.h`
- `WiNILL/Core/Scenes/WindowChrome.h`
- `WiNILL/Core/Scenes/WindowChrome.cpp`
- `WiNILL/Core/Scenes/Scenes.cpp`
- `WiNILL/Render/DrawPrim.h`
- `WiNILL/Render/TextRenderer.h`
- `WiNILL/main.cpp`

주의:
- `g_BatchAlpha`는 씬 함수 진입 시 설정하고 반드시 끝에서 `1.0f`로 복구해야 함. 현재 `Scene_DifficultySelect`만 적용 중.
- `SceneFlowWindow` 시그니처가 `dimAlpha / bodyAlpha / shadowAlpha` 3개 파라미터로 확장됐으므로 신규 호출 시 기본값 확인 필요 (각각 `0.0f / 0.97f / 0.32f`).

---

### 2026-08-19 / Codex

- 난이도 선택 시스템을 플레이어 흐름에서 제거했다.
- 내부 `Difficulty` enum은 보스 API와 기록 저장 호환 때문에 유지했지만, 새 런 시작 시 `Difficulty::NORMAL`로 고정한다.
- 기존 `GameState::DIFFICULTY_SELECT`는 `GameState::RUN_CONFIG`로 이름을 바꿨다.
- 런 설정 화면은 `총기 선택 -> 시련 모듈 -> 실행 요약` 구조로 변경했다.
- 시련은 3개 랜덤 카드에서 `초반 / 중반 / 후반 / 보스` 4축 구조로 확장했다.
- 시련 효과가 점수 배율뿐 아니라 실제 런 수치에도 연결되도록 했다.
- 마지막 확인 빌드:
  - `MSBuild Debug|x64`
  - 오류 0개

관련 파일:

- `WiNILL/System/Settings.h`
- `WiNILL/Core/GameManager.h`
- `WiNILL/Core/GameManager.cpp`
- `WiNILL/Core/Scenes/SceneBoot.cpp`
- `WiNILL/Core/Scenes/Scenes.h`
- `WiNILL/Core/Scenes/Scenes.cpp`
- `WiNILL/Core/Scenes/WindowChrome.h`
- `WiNILL/main.cpp`

주의:

- `WiNILL/main.cpp.bak`, `WiNILL/main_temp.cpp`에는 예전 난이도 코드가 남아 있지만 빌드 대상이 아니라 건드리지 않았다.
- `Settings.h`의 `Difficulty` enum과 `GetDifficultyParams()`는 아직 호환용으로 남아 있다. 완전 제거하려면 보스별 `Update(..., Difficulty)` 시그니처와 기록 저장 구조까지 같이 바꿔야 한다.

---

### 2026-08-19 / Claude — 적 컨셉 확정 (코드 변경 없음, 기획 메모)

**게임 테마 방향 전환 결정**

- 기존: OS 프로세스/악성코드 vs 플레이어
- 변경: 밤하늘 배경화면 위에서 별자리 존재들과 싸우는 구조
- 비주얼 언어: 점(dot) + 선(line) 으로만 구성된 디자인
- 데모 범위: 밤하늘/별자리 테마 단일

**일반 몹 구성 확정 (4종)**

| 새 이름 | 구 이름 | 역할 |
|---|---|---|
| 성진(星塵) | 프로세스 | 단독 배회 잡몹, 능력 없음 |
| 성군(星群) | 봇넷 | 성진류 무리를 이끄는 중심별 |
| 성진류(星塵流) | 디도스 | 성군에 종속된 별 알갱이 떼 — 성군 없이는 미등장 |
| 펄사(Pulsar) | 애드웨어 | 원거리 탄막 슈터 |

**제거 확정 적 목록**

웜, 트로이목마, 크래셔, 버그, 커널프로세스, 스파이웨어, 랜섬웨어, 레지스트리에러  
(디도스는 성진류로 전환, 독립 개체에서 성군 종속 개체로 역할 변경)

**보스 방향 (미착수)**

- FORK: 비주얼 리디자인 예정 (기존 OS 외형 탈피)
- TESS: 외형 완성 상태, 패턴을 "세계의 끝" 컨셉으로 교체 예정
- GATE.lock: 검토 미정

**작업 순서 합의**

1. UI 정리 먼저 (RUN CONFIG 검증, 씬 전환 등 미완)
2. 비주얼 리디자인 (점+선 기반 몹 외형 교체)
3. 네이밍 교체 (비주얼 작업과 UI 텍스트를 한 번에 묶어서)

---

## Codex Workspace

### Current State

- 런 설정 UI:
  - 왼쪽: 총기 선택 및 선택한 총기 상세
  - 가운데: 4축 시련 모듈
  - 오른쪽: 실행 요약
- 실행 조건:
  - 총기를 고르면 실행 가능
  - 시련은 선택사항
- 시련 수치 연결:
  - 초반 원거리몹 시작 보정
  - 일반 몹 스폰 압박
  - 시작 최대 체력 감소
  - 중반 특수/정예몹 비율 증가
  - 자폭병 시작 시간/주기 변경
  - 후반 스폰/체력 램프 강화
  - 보스 HP 증가
  - 보스 경고 시간 감소

### Open Issues

- 런 설정 화면은 빌드만 확인했고, 실제 클릭/화면 배치 플레이 검증은 아직 별도로 하지 않았다.
- `Difficulty` 완전 삭제는 보스 코드와 저장 기록 구조까지 포함하는 별도 작업으로 분리하는 편이 안전하다.
- 시련 설명과 실제 효과의 체감 밸런스는 플레이 테스트 후 재조정 필요.

### Next Candidates

- 런 설정 화면 실제 실행 테스트
- 시련별 수치 밸런스 리포트 작성
- `RUN_CONFIG`에 맞춰 문서/주석의 난이도 표현 정리
- 백업 파일 정리 여부 사용자 확인

---

## Claude Workspace

### Current State

- 브라우저 크롬: 31px 단일 주소창 바 (탭 시스템 제거), 진행 바 내장
- 씬 전환: `StartSceneFade()` 기반 콘텐츠 알파 페이드 (전체화면 블랙 아님)
- 배경 시스템: `DrawMenuBackground()` 공용 함수로 모든 메뉴 씬 동일 배경
- 패널 투명도: `SceneFlowWindow` / `SceneAppWindow` 파라미터로 제어 가능
- 전역 알파: `g_BatchAlpha` (DrawPrim + TextRenderer 적용)
- DEV MODE: RUN CONFIG 씬 하단 우측 토글 버튼
- **RUN CONFIG UI**: 별자리 프레임 + 블룸 셰이더 + 헥사곤 레이더 차트 + 무기 전환 모핑 애니메이션 완료

### Open Issues

- `g_BatchAlpha` 페이드인이 현재 `Scene_DifficultySelect`에만 적용됨. 다른 씬 전환에도 적용하려면 씬별로 추가 작업 필요.
- 메뉴 씬들의 전체화면 딤을 제거했는데, 패널 바깥 가장자리 배경이 메인메뉴와 완전히 동일한지 시각 검증 필요.
- Settings 씬은 게임 오버레이 여부(`settingsOverlay`)에 따라 배경 처리가 분기됨 — 게임 중 설정 열기 시 정상 동작 확인 필요.
- RUN CONFIG 씬 레이아웃 실 플레이 검증 (클릭 동작, 화면 배치) 미완.

### Next Candidates

**UI 작업 (우선순위 순)**
- 메인메뉴 UI를 RUN CONFIG와 동일한 별자리 디자인 톤으로 재작업
- 씬 전환 페이드를 다른 씬(Shop, Codex 등)에도 확장 적용
- 시련 상세 페이지 제작 (3카드 선택 → 4축 시련 구조 연결)

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

### 2026-08-20 / Claude

- `drawConstellFrame` + `SetGlowFx(uFx==2)` 전면 적용 (RUN CONFIG 카드 전체)
- `MainShader.cpp` uFx==2 블룸 브랜치 추가
- `DrawPrim.h` — `drawConstellFrame`, `drawNeonBorder`, `SetGlowFx` 추가
- 레이더 차트 — `WDetailDef.norm[6]` + 6축 헥사곤 spider chart
- 레이더 모핑 애니메이션 — `s_RadarCur[6]` lerp (UiApproach, speed=7)
- 패널 전환 페이드 — `s_PanelFadeT` + `g_BatchAlpha` 범위 제어
- 폰트 크기 조정 — 헤더·무기명·배율·토글 버튼 전반 축소

### 2026-08-19 / Claude

- 브라우저 크롬 31px 리디자인, 진행 바 추가
- 씬 페이드 전환 시스템 구현 (콘텐츠 알파 방식)
- `g_BatchAlpha` 전역 알파 배율 추가
- `DrawMenuBackground()` 공용 배경 함수 분리
- 모든 메뉴 씬 전체화면 딤 제거
- `SceneFlowWindow` / `SceneAppWindow` 파라미터 확장
- HUD / 플레이어 클램프 타이틀바 기준으로 수정
- DEV MODE 토글 버튼 추가

### 2026-08-19 / Codex

- 공유 작업 로그 파일 생성.
- 규칙 잠금 섹션, 공용 인수인계, Codex 전용 구역, Claude 전용 구역 분리.
