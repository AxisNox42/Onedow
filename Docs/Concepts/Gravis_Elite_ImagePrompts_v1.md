# GRAVIS 엘리트 이미지 생성 프롬프트 v1

작성일: 2026-10-18 · 도구: 내장 `image_gen` · 모드: 레퍼런스를 사용한 신규 생성

## 비교 보드

출력: `Gravis_Elite_PatternComparison_v1.png`

스타일 레퍼런스: `Quasar_Elite_PatternComparison_v2.png`, `Genesis_Elite_Rotor_PatternComparison_v1.png`. 두 이미지를 먼저 열어 확인한 뒤 전달했다.

```text
Use case: infographic-diagram
Asset type: Korean game enemy design reference sheet, wide landscape high resolution.
Primary request: Create GRAVIS elite two-pattern comparison concept board matching the supplied references' structure: near-black background, thin grey panel borders, crisp luminous geometric game line art, large legible Korean white typography, two big side-by-side combat panels and a bottom four-column comparison strip. No photorealism, no illustrated fantasy creature, no gradients obscuring mechanics.
Reference images: first reference supplies layout only (Quasar comparison), second supplies crisp diagram style only (Genesis comparison). DO NOT use their enemy designs.
GRAVIS invariant design: purple hexagonal frame with EXACTLY six small circular vertex nodes, bright large central hexagonal core, ONE tilted thin ellipse orbit inside frame with tiny white satellite, lavender pale halo. Same enemy body in both panels. Cyan outlined diamond is player. No spawned monsters, no meteors, no beam weapons.
Exact main title: "GRAVIS 엘리트 · 두 패턴 제안"
Subtitle: "기존 중력장 유지 · 원형 붕괴 ↔ 양측 협착"
Left large panel title: "A. 중력정 붕괴"
Small subtitle: "원형 예고 → 중심 수축 → 국소 붕괴"
Scene: Gravis at upper left, to its lower right one large circular purple gravity well at a previously targeted player location, hollow dark center and fine concentric arcs with short radial inward arrows. Diagram inset shows charging body: six nodes contract symmetrically. Cyan player diamond escapes OUTSIDE circle to lower right with cyan arrow away from well. Small grey hollow diamond inside is previous player position. Well is separate from body with ONE thin curved NON-DAMAGING explanatory energy connector, NOT a projectile trajectory. Small separate impact inset shows single filled purple disk flash at the same center (no expanding shockwave). Label "고정된 원 밖으로".
Right large panel title: "B. 조석 협착"
Small subtitle: "양측 예고 → 두 판 접근 → 중앙 압착"
Scene: identical Gravis at upper left of its panel, six nodes spread into clearly opposing left and right groups of THREE but keep central hexagonal core with one thin orbit. At lower right remote short rectangular combat zone: two thick VERTICAL purple gravity slabs/brackets on LEFT and RIGHT moving HORIZONTALLY toward center with inward arrows. Grey thin rectangle shows entire swept danger footprint, OPEN ESCAPE ENDS at TOP and BOTTOM. Cyan player diamond escapes DOWN beyond bottom end, straight cyan downward arrow, no closed cage or top/bottom walls. Small grey previous player diamond at center. This is a short local pincer, NOT a long laser. Label "열린 위·아래 끝으로".
Bottom four columns titles and exact paired text:
"본체" / "여섯 노드 수축 ↔ 좌우 3+3 분리"
"예고" / "원형 영역 ↔ 짧은 사각 영역"
"위험" / "한 번의 원형 붕괴 ↔ 이동하는 두 판"
"대응" / "반경 밖 이탈 ↔ 열린 끝 이탈"
Footer: "설계 시안 · 소환 없음 · 색을 빼도 구별 · 수치는 플레이 검증"
Ensure clean flat 2D top-down gameplay schematics and strong spatial distinction. No tiny long paragraphs. No spurious numbers.
```

## 조석 협착 스토리보드

출력: `Gravis_Elite_Tidal_Storyboard_v1.png`

레퍼런스: 위에서 생성한 그라비스 비교 보드.

```text
Use case: infographic-diagram
Asset type: Korean GRAVIS elite combat storyboard, landscape reference board.
Input image: latest GRAVIS two-pattern comparison is a STYLE AND IDENTITY REFERENCE. Preserve its black background, fine gray frames, purple luminous geometric lines, six-node hexagonal enemy with bright core and ONE thin tilted orbit, cyan diamond player, legible Korean white typography.
Primary request: A precise four-panel left-to-right storyboard for ONLY the tidal pincer attack. All combat panels show EXACTLY TWO parallel VERTICAL gravity plates, always left and right of a stationary marked targeting center, moving HORIZONTALLY inward. Open ends ALWAYS top and bottom. No top/bottom walls. No closed cages. No circles or explosions in combat area. Enemy itself at upper left of each panel, smaller than gameplay action, nodes split 3+3.
Title: "GRAVIS · 조석 협착 스토리보드"
Subtitle: "예고 위치 고정 · 두 판만 이동 · 열린 끝으로 회피"
Four equal framed panels, SAME scale and SAME short danger rectangle location in each. Thin grey rectangular outline with faint fill means total swept region. A hollow grey diamond at center shows original target location in all panels.
Panel 1 title: "01 · 예고 1.5초"
Two thin ghost plates at left/right outer boundaries of short rectangle. Cyan player diamond near center just below grey mark. Bright cyan down arrow pointing through open bottom end. Small top and bottom outward arrows show both exits. Text below diagram: "위치·방향 고정" and "전체 이동 범위를 먼저 표시".
Panel 2 title: "02 · 접근 시작"
Both plates now solid purple with white inner edges, slightly closer together. Horizontal arrows inward on both. Cyan player moving downward, just inside bottom end. SAME gray rectangle and center mark. Text: "판이 서로 접근" and "별도 당김 없음".
Panel 3 title: "03 · 중앙 압착"
Two solid parallel plates have almost met at center with tiny gap, not touching the escaped player. Cyan player clearly BELOW and OUTSIDE rectangle. Grey diamond original target center can be partially obscured by plates. No lightning connecting player. Text: "판에 닿을 때만 피해" and "중앙에 남으면 피격".
Panel 4 title: "04 · 회복 0.9초"
Meeting plates now very faint dissolving slim lines at same center; no explosion, no lingering damage. Cyan player below remains unharmed. Enemy nodes restore hexagonal positions. Text: "판 소멸 · 본체 복귀" and "추가 폭발 없음".
Bottom strip three cells:
"소환체 아님" / "중력 효과 자체가 공격"
"예고와 판정" / "사각형은 예고 · 판만 실제 피해"
"회피 방향" / "두 판의 열린 위·아래 끝"
Footer exact: "설계 시안 · 협착 이동 0.7초 · 수치는 초기 검증값"
Avoid cinematic perspective, extraneous enemies, long beams, horizontal plates, moving whole target rectangle, duplicated player at center except grey hollow historical marker. Fine broken grey explanatory lines are allowed; actual plate edges solid. Keep typography crisp.
```

## 시각 검토

비교 보드는 원형 중력정과 양측 중력판, 두 회피 방향을 구별한다. 스토리보드는 판의 접근과 열린 끝으로의 이탈을 보여준다. 스토리보드 본체는 노드 분리 동작을 충분히 반영하지 않아 위치·판 이동 설명용으로 사용한다. 본체 준비 자세는 비교 보드와 설계 문서를 기준으로 한다. 그림의 선 두께와 상대 크기는 실제 월드 px 판정값을 측정하는 용도가 아니다.

