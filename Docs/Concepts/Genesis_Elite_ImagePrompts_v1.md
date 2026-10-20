# GENESIS 엘리트 — 이미지 생성 프롬프트

작성일: 2026-10-18

내장 `image_gen` 사용. 실제 게임 캡처가 아닌 컨셉 레퍼런스다.

## 1. 두 패턴 비교 초안

출력: `Genesis_Elite_PatternComparison_v1.png`

참조: `Quasar_Elite_PatternComparison_v2.png` — 패널 구성과 선화 스타일.

```text
Use case: infographic-diagram
Asset type: Korean game enemy elite design comparison sheet, landscape, detailed polished neon line-art technical concept board.
Primary request: GENESIS elite, an upgrade of an existing summoner, two unmistakably different combat patterns. Match the input reference boards' graphic design: nearly black background, thin gray panel borders, white Korean headings, crisp luminous wireframe geometry, restrained glow, cyan diamond player, gray explanatory arrows. This is a new GENESIS board, not a Quasar board. Enemy color crimson red, hot coral edges, pale warm core. Top-down 2D gameplay schematic, no perspective environment, no organic monsters.
Subject invariant: same Genesis body in both panels, small solid hexagonal central core with exactly THREE broad faceted curved shell cradles arranged around it, radial spokes, one front muzzle gap. Each cradle carries a small triangular SWARM. No big conventional gun, no star symbol.
Layout: wide title strip, two equally sized large comparison panels, four compact bottom comparison cells. All Korean labels readable, minimal text, strong margins.
Title verbatim: "GENESIS 엘리트 · 두 패턴 구분 시안"
Subtitle: "기존 소환체 유지 · 투척과 분대 돌격을 다르게 읽기"
Left header: "A. 기존 패턴 · SWARM 투척"
Left scene: Genesis upper left, front muzzle aiming diagonally right. Three narrow red aim lines end at three small red landing circles near cyan diamond player. Show three triangular Swarms flying one by one from ONE muzzle, short tails. Body shells tightly clamped. Circle impacts spatially separated. Gray arrow shows cyan player moving out of landing circles.
Left bottom: "좁은 조준선 + 착지 원" and "착지 지점에서 이탈"
Right header: "B. 추가 제안 · 협공 출격"
Right scene: same Genesis at top center but THREE shells spread away from small core like an opening three-petal cradle, no forward aim lines or landing circles. Six small triangular SWARMs have emerged and moved along thin GRAY non-damaging deployment arrows to LEFT and RIGHT flanks of a cyan player. Exactly 3 triangles on the left and 3 on the right, open chevron squads. Each squad aims DOWN the board along its own parallel locked broad rectangular corridor. Two long transparent red outlined corridors, clear central safe gap, cyan player can move into central gap. SWARMs visibly fly downward, short triangle tails, no lasers or connecting damage beams. The corridors are telegraphs only; damage is from moving Swarm bodies. Gray escape arrow into central gap. Do not surround the player with a closed cage. Show small gray guide paths from Genesis to each squad to communicate summoned units physically deploy.
Right bottom: "좌우 분대 + 평행 돌격 경로" and "경로 사이로 이동 · SWARM 격추"
Bottom comparison cells titles and pairs:
"본체" / "전방 결속 ↔ 세 외피 개방"
"예고" / "가느다란 선·원 ↔ 넓은 평행 경로"
"위협" / "개별 투척·착지 ↔ 분대 동시 돌격"
"대응" / "지점 이탈 ↔ 경로 회피·격추"
Small footer: "컨셉 제안 · 회색 화살표는 설명용 · 경로 전체에 피해 없음 · 수치는 후속 검증"
Constraints: accurate two-dimensional tactical diagram, plenty of black negative space, visual hierarchy like reference. No Quasar blue orbital disk, no force field, no damaging floor laser, no cage, no spaceship realism.
```

## 2. 협공 진행 스토리보드

출력: `Genesis_Elite_Pincer_Storyboard_v1.png`

참조: 비교 초안 — 본체·색·실루엣. `Quasar_Elite_Barrage_v3.png` — 스토리보드 구성.

```text
Use case: infographic-diagram
Asset type: Korean top-down 2D combat pattern storyboard, landscape, polished technical neon line art on near-black background.
Reference image 1 is exact Genesis visual subject and palette, reference image 2 is storyboard panel layout only; do not draw Quasar.
Primary request: explain Genesis summoner elite's proposed "협공 출격" with four large numbered panels across the top and a small tactical clarification strip below. Clean white Korean typography, fine gray borders, red/coral enemy geometry, cyan diamond player, gray nondamaging explanatory arrows. Strong negative space, no environment texture.
Genesis has ONE small hexagonal center and exactly THREE broad faceted shell cradles around it on radial spokes. Keep identical body identity across frames. SWARM units are simple small hollow red triangles.
Title: "GENESIS 엘리트 · 협공 출격"
Subtitle: "생산 → 좌우 전개 → 경로 잠금 → 동시 돌격"
Panel 1 header: "① 생산·개방 · 1.2초"
Caption: "세 외피에서 두 마리씩 · 총 여섯 마리"
Show the three shells opening like three separated broad curved petals. Each cradle has exactly TWO tiny Swarm triangles showing successive production, 6 total. Two small ticks per shell, no third tick. No laser muzzle.
Panel 2 header: "② 좌우 전개 · 0.8초 이상"
Caption: "세 마리씩 두 분대 · 전개 중 격추 가능"
Same Genesis top. Exactly three red Swarm triangles in a left open-chevron squad and three in a right open-chevron squad. Thin GRAY curved physical deployment paths from Genesis to squads. Cyan diamond further down. No red damage trails on deployment.
Panel 3 header: "③ 경로 잠금 · 1.0초"
Caption: "위치·방향 고정 · 잠금 후 추적 없음"
Show two side-by-side VERTICAL PARALLEL thin red outlined rectangular corridors extending downward from the two squads, three triangles per squad at upper ends. Give no area fill, just outlines and tiny directional chevrons. Cyan diamond INITIAL player position inside the LEFT corridor near its inner edge, gray arrow to central gap. Central gap wide and clear. Important one corridor passes through original player position, NOT two lanes that miss a stationary player. No attack connecting the two squads. Body small top center. Both squads face DOWN in same direction, never opposite head-on collision.
Panel 4 header: "④ 동시 돌격 · 약 0.7초"
Caption: "몸체만 충돌 피해 · 생존체는 추적으로 전환"
Show the six triangular units halfway down the TWO PARALLEL corridors, short motion tails. Cyan player now in central safe gap. Put hollow thin gray diamond at its OLD position inside left lane and a small gray arrow to new cyan diamond position. Same body above closing shells. No impact rings, no explosions, no beam filling the lanes.
Bottom strip has 3 compact explanatory cells:
cell 1 "피해 판정" shows one bright red triangular Swarm beside a thin outline route, text "띠는 이동 예고 · 실제 SWARM만 피해".
cell 2 "회피와 격추" shows cyan diamond safely between lanes and one crossed-out red triangle leaving an empty formation slot, text "통로로 이동 · 격추한 빈자리 유지".
cell 3 "생산 수" shows exactly three small shell icons each labeled "2", formula "2 + 2 + 2 = 6", text "두 분대에 세 마리씩 배치".
Footer: "컨셉 제안 · 수치는 후속 검증 · 회색 화살표는 설명용"
Constraints: six total Swarms, always 3+3 in deployed panels, exactly three body shells; no field damage, no cages, no new enemy species, no color-only pattern distinction. Readable large text.
```

## 3. 비교표 문구 교정 — 최종 비교 이미지

출력: `Genesis_Elite_PatternComparison_v2.png`

입력: 비교 초안. 총 여섯 마리임을 명확히 하고 플레이어가 회피 후 위치에 있음을 표시했다.

```text
Edit this existing GENESIS comparison board. Preserve the entire composition, line-art style, two panels, diagrams, and all other text. Fix ONLY the small paragraph below the right-panel light-blue label "좌우 분대 + 평행 돌격 경로": replace its incorrect production-count wording with exactly these two Korean lines:
"총 여섯 마리가 좌우에 세 마리씩 모여"
"평행한 경로로 동시에 돌격한다."
Also add one small gray label "회피 후 위치" next to the cyan diamond in the central safe gap of the large right diagram, so this is clearly the player's location AFTER dodging.
Do not change the six triangles, three shells, other labels or layout. Clean readable Korean text, no extra content.
```

스토리보드의 돌격 컷에는 네 마리만 보인다. 이는 생존체 예시로 읽으며, 생산·전개 수량은 총 여섯 마리(3+3)가 기준이다. 하단 생산 수 도해의 반복 본체 아이콘은 생산 단위 설명이며 제네시스 세 기가 등장한다는 의미가 아니다. 구현 시 수량·판정은 설계 문서를 기준으로 검증한다.

