# 성좌 점화 — 이미지 생성 기록 v1

2026-10-19 · 내장 `image_gen` 사용 · CLI/API fallback 미사용

출력: `Boss_Constellation_Ignition_v1.png`

참조: `Quasar_Elite_PatternComparison_v2.png` — 도표 구성, 선 표현, 팔레트만 참고. 기존 보스 문서 미참조.

```text
Use case: infographic-diagram.
Asset type: Korean game boss first-pattern exploration storyboard, wide landscape, thin geometric concept art.
Primary request: Develop ONE proposed first boss attack from these two starting properties: an OPEN LINE STRUCTURE with the background visible through it, and a CONSTELLATION theme. Proposed working attack name "성좌 점화". This is new exploratory planning, not a finalized boss or gameplay screenshot. The attached Quasar board is ONLY a reference for typography, panel borders, thin wireframe style, dark palette; do not copy its body, ellipses, orbital rings or attacks.
Style: near-black background #080910, fine blue/periwinkle SOLID lines, white tiny star nodes, restrained glow, readable Korean sans serif, thin dark-grey panel borders. Flat orthographic top-down diagrams. No filled polygons, no solid armor, no creature portrait, no starfield decoration, no planetary backdrop, no rings, no sectors, no explosions.
Important transparency illustration: behind each boss draw the SAME barely visible large soft blue-grey diagonal BACKGROUND contour strokes, at very low contrast, continuously visible through ALL open spaces between nodes and connecting edges. These background strokes must remain continuous, never masked by a body silhouette, clearly background rather than attack. No rectangle pretending to be a transparent body. Only core and line strokes occlude tiny parts of background.
New schematic boss: small WHITE HEXAGON core, SIX little PERIWINKLE star-like nodes arranged as an asymmetric OPEN branching constellation with 5 fine blue connector edges, not a closed polygon or humanoid. The six nodes belong to this boss; no additional monsters. The core stays identifiable at upper-left anchor throughout. Cyan HOLLOW DIAMOND represents player, clearly distinct from white star nodes. Grey arrows are explanatory only.
Title exact "첫 패턴 후보 · 성좌 점화"
Subtitle exact "배경이 비치는 몸체 → 별의 배치 → 연결선의 순차 점화"
Top 4 equal storyboard panels, lower two broad detail panels.
Panel 1 heading "① 별자리 본체". Draw compact asymmetric open constellation body with hexagon core at upper-left connected to six little nodes. Background contour strokes remain visible through body. Small cyan player far lower-right. Caption "면을 채우지 않고 별과 선으로 구성".
Panel 2 heading "② 별을 펼친다". SAME boss core in same upper-left place, six attached star nodes have spread farther right and down, as the same open branching tree at a much wider scale, no closed shapes. Lines are very thin dim blue, nodes small but readable. Player lower-right below all lines. A single SMALL grey outward arrow near outermost node shows deployment. Caption "별의 이동은 준비 동작 · 아직 피해 없음".
Panel 3 heading "③ 연결을 예고한다". EXACT SAME fixed six node positions and same 5 OPEN-tree edges as panel2. All edges thin dim blue solid lines. Put a small white brighter notch at both ends of ONE upcoming edge to indicate imminent activation. Do not draw extra lines or enclosed areas. Player still safely away. Caption "별의 위치 고정 · 다음 선의 양끝 점등".
Panel 4 heading "④ 한 선씩 점화". EXACT SAME positions and edge graph as panel3. ONE AND ONLY ONE edge bright WHITE with restrained periwinkle outline; other four edges very dim. Cyan player safely away from ALL lines, short grey arrow points into empty lower-right open background. No ray from central core to player. Caption "밝은 선에만 피해 · 지나간 선은 소등".
Bottom LEFT detail heading "열린 구조 · 배경은 계속 보인다". Draw a magnified irregular OPEN chain of four star nodes connected by three thin lines with soft background contours visible through the spaces. Caption "보스의 존재감은 면적보다 별의 간격과 움직임".
Bottom RIGHT detail heading "한 연결선의 상태". Three clear small diagrams in a row separated by plenty of space: first two blue nodes with very dim thin blue solid line, second same two nodes with bright white line slightly thicker, third same nodes with NO line. Exact labels beneath "예고 · 피해 없음", "점화 · 선에만 피해", "소등 · 피해 없음". No cyan player here.
Footer exact "첫 패턴 탐색안 · 닫힌 감옥 형태 없음 · 위치 고정 후 점화 · 시간과 피해량은 미정".
Precise legible Korean. The main idea is a constellation's existing stars expand to define the battlefield, then edges illuminate one by one. Never render all edges active at once, never fill interior, never show beam fired from existing Quasar body. Maintain same node layout and open graph across panels 2 3 4.
```

검토: 네 단계, 열린 선 구조, 배경의 연속 노출, 단일 활성 구간을 확인했다. 노드 개수와 좌표는 패널마다 미세하게 달라 정밀 동작 도면으로 사용할 수 없다. 본문의 위치 고정 규칙을 우선한다. 실제 배경 대비와 게임 내 회피 가능성은 검증 전이다.

