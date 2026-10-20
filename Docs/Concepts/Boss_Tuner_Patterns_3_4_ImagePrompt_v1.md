# 조율자 — 패턴 3·4 이미지 생성 기록 v1

2026-10-19 · 내장 `image_gen` 사용 · CLI/API fallback 미사용

출력: `Boss_Tuner_Patterns_3_4_v1.png`

참조: `Boss_Tuner_Ignition_Control_v1.png`는 임시 본체와 패널·표식 스타일, `Quasar_Elite_PatternComparison_v2.png`는 얇은 선과 도표 구성을 참고했다. 기존 보스 문서 미참조.

```text
Use case: infographic-diagram.
Asset type: Korean game boss design reference comparing proposed third and fourth patterns, wide landscape.
Primary request: Add TWO new DIRECT attacks for the existing planning-only boss "조율자" / alternate "계측자", matching attached concept board layout and thin line language. Unlike previous abilities that buff and command existing monsters, both new attacks work with NO MONSTERS alive. This is concept planning, not gameplay screenshot.
Style: almost-black #080910, fine SOLID periwinkle and white geometric lines, minimal glow, hollow shapes, thin dark-gray panel borders, highly readable Korean sans serif. No starfield decoration, no painted sci-fi, no solid body, no filled hazard shapes, no dotted or dashed lines. Cyan HOLLOW diamond is player. Background very subtle long blue-gray contour lines remain continuous through empty body spaces.
Same provisional boss icon as reference Boss_Tuner_Ignition_Control_v1.png: small hollow white hexagonal core, THREE disconnected thin angular graduated bracket segments in periwinkle. Boss not an existing Quasar or Gravis. No ignited mobs anywhere in main diagrams. Grey arrows explanatory.
Title exact "조율자 · 패턴 3·4 추가 시안"
Subtitle exact "몹 없이도 실행 · 위치를 조율하고 각도를 측정한다"
Two equal large columns, each subdivided into TWO spacious stages, bottom full-width four-pattern differentiation strip.
LEFT heading "패턴 3 · 좌표 도약"
Subcaption "지점 고정 → 본체 이동 → 국소 충격"
Top stage label "① 도착 지점 예고". Boss UPPER LEFT, four clear separated CORNER BRACKETS frame one medium SQUARE landing area LOWER RIGHT. The square's four side boundaries between brackets are thin solid dim periwinkle, readable full footprint. Inside square a TINY DIM hollow hexagon means destination ghost, not second boss. Cyan player currently inside square near lower-right corner and small short GREY arrow points OUT of square into lower-right empty space. No line from boss to destination, no projectile. Small caption "현재 위치에 표식 · 고정 뒤 추적 없음".
Bottom stage label "② 이동 후 한 번 충격". Same square exact same location, real boss now INSIDE square with its three separate brackets compressed around hexagonal core. Previous boss location only TINY FAINT GREY hexagonal ghost, clearly not another targetable boss. Square border and four corners briefly WHITE with short inward ticks indicating ONE LOCAL PULSE inside square. No expanding rings, no persistent field. Cyan player now safely OUTSIDE square lower-right, away from border. No motion beam connecting old and new boss positions. Small caption "이동 경로는 무해 · 사각 내부에 한 번 피해". Bottom left column response "대응 · 표시된 사각에서 이탈".
RIGHT heading "패턴 4 · 성도 주사"
Subcaption "회전 범위 예고 → 한 줄로 훑기 → 눈금 복귀"
Top stage label "① 시작각·끝각 예고". Boss at LEFT CENTER, frames have aligned into a clearly elongated graduated ruler aligned outward right, stationary core. A wide OPEN FAN outline toward RIGHT shows anticipated sweep of around 100 degrees, entirely dark interior NO fill. Two thin solid BLUE radial boundaries indicate start lower-right and end upper-right, each with small endpoint bracket ticks. ONE SMALL GREY CURVED arrow outside arc shows sweep from bottom to top. Cyan player on lower-left BEHIND boss, safe outside fan. Small caption "전체 범위와 회전 방향을 먼저 표시".
Bottom stage label "② 고정된 각도로 주사". Same boss coordinates, same fan boundaries. ONLY ONE bright white SINGLE-DIRECTION ray from boss rightwards at current middle angle. Short ruler ticks sit along one side of active ray at intervals. Other boundaries stay DIM BLUE; arc also DIM. No other active rays, no opposing beam behind boss, no filled sector. Cyan player safely behind boss lower-left. ONE tiny GREY curved arrow outside fan indicates rotation. Small caption "밝은 선만 피해 · 부채꼴 전체 피해 아님". Bottom right column response "대응 · 훑는 범위 밖으로 이동".
Bottom strip title "네 패턴의 시각적 구별".
Four small aligned columns with icons and exact captions:
"1 · 성좌 점화" / small square mob with four tiny white star rays / "몹의 코어가 점등".
"2 · 성좌 조율" / small square mob with side chevrons and curved grey arrow / "몹이 당겨졌다 방출".
"3 · 좌표 도약" / four corner brackets with tiny core at center / "보스의 위치가 이동".
"4 · 성도 주사" / open fan with one white radial line / "한 줄이 각도로 회전".
Tiny footer exact "기획 시안 · 네 패턴 동시 사용 없음 · 수치와 실제 회피 가능성은 검증 전"
Precise Korean, no extra text. Keep all diagrams hollow so background visible. These are NEW proposals; final name and boss body remain provisional. White means active hazard, blue thin means telegraph, grey means explanatory past position or movement. Avoid visually confusing teleport route with damage beam.
```

검토: 좌표 도약의 사각 예고·이동 후 국소 충격, 성도 주사의 열린 부채꼴·단일 활성선, 네 패턴 비교 구성을 확인했다. 좌표 도약 그림의 과거 본체는 회색 설명용 잔상으로 두 번째 보스가 아니다. 부채꼴 전체 피해와 이동 경로 피해는 없다. 그림의 배경 곡선은 투과 구조 설명용이다. 실제 가독성·회피 시간·피해 판정은 검증 전이다.

