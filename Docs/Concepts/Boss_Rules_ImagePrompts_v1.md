# 보스 공통 규칙 — 이미지 생성 기록 v1

2026-10-19 · 내장 `image_gen` 사용 · CLI/API fallback 미사용

기존 보스 문서는 참고하지 않았다. 참조 이미지는 시각 스타일과 패널 구성에만 사용했다.

## 규칙 보드

출력: `Boss_Rules_Board_v1.png`

참조: `Quasar_Elite_PatternComparison_v2.png`, `Elite_Rules_Board_v1.png`.

```text
Use case: infographic-diagram.
Asset type: Korean game boss planning rules board, wide landscape, high-resolution.
Primary request: Generate a NEW concept design reference sheet matching the restraint, graphic layout and theme of the provided LOCAL reference images. References are ONLY graphic style references, not enemy identities to reuse. This is planning, not gameplay screenshot.
Reference images: Quasar_Elite_PatternComparison_v2.png and Elite_Rules_Board_v1.png, both graphic layout references.
Style: almost-black #080910 background, exceptionally fine solid blue/periwinkle and white geometric wireframe lines, tiny luminous nodes, subtle minimal glow, flat top-down orthographic game diagrams, thin dark grey rectangular panel borders, crisp readable Korean sans-serif typography, generous empty space. No painterly sci-fi, no stars, no 3D armor, no monster faces, no filled hazard fields.
Title exact "보스 몹 · 공통 규칙 시안"
Subtitle exact "새로운 본체 · 패턴 3~4가지 이상 · 서로 다르게 읽히는 공격"
Layout: two large equal main columns, a bottom full-width verification strip.
LEFT main column heading "규칙 1 · 패턴 3~4가지 이상". Smaller line "최소 3개 · 4개 구성 권장". Show a small original schematic boss in upper center: white hollow hexagonal core suspended between TWO disconnected squared U-shaped periwinkle frames opening toward each other, four small corner nodes; NO orbital ellipses. Four thin GREY explanatory branches lead to FOUR separate small attack cards in 2x2 layout numbered A B C D. A icon narrow diagonal corridor with body at its front; B icon broad fan-shaped sector; C icon two separated polygonal barriers leaving an obvious open corridor; D icon one broken circular wavefront with a large missing wedge. Use exact labels "A · 관통 돌진", "B · 편각 휩쓸기", "C · 격자 압착", "D · 결손 파동". Footer within left column "준비 → 실행 → 회복은 한 패턴의 단계".
RIGHT main column heading "규칙 2 · 같은 패턴으로 보이지 않게". Smaller line "네 번째 이후 패턴에도 동일하게 적용". Show FOUR spacious horizontal rows of paired mini-icons comparing shapes with a small white bidirectional arrow: row labels "본체 자세", "예고 도형", "위험 형태", "움직임". First compare compressed closed frame body vs spread asymmetric frame body. Second compare narrow parallel rails vs broad sector outline. Third compare fan outline vs two disconnected angular wall segments. Fourth compare straight motion arrow vs expanding broken ring. Avoid all decorative arrows resembling gameplay attacks. Footer within right column "색과 이름을 가려도 구별되는가?"
Bottom strip heading "패턴으로 세지 않는 변형". Four short captions separated by fine vertical dividers: "색만 변경", "피해·속도만 변경", "탄 수·크기만 증가", "등장·사망 연출".
Tiny bottom footer exact "기획 시안 · 보스 외형과 개별 공격은 확정 전 · 추가 판정 기준은 제안"
All four attacks are examples to explain rule, not committed boss roster. New original body, no existing Quasar/Gravis/Genesis body. Maintain precise Korean text, clean technical reference layout.
```

## 네 패턴 비교

출력: `Boss_Pattern_VisualLanguages_v1.png`

참조: `Quasar_Elite_PatternComparison_v2.png`, 위에서 생성한 `Boss_Rules_Board_v1.png`.

```text
Use case: infographic-diagram.
Asset type: Korean game boss FOUR DISTINCT ATTACKS comparison sheet, wide landscape technical reference.
Generate a NEW reference board with FOUR equal vertical columns A B C D and a bottom comparison strip. Style references are attached existing Quasar comparison sheet and rules board: near-black #080910 background, very thin solid blue-periwinkle and white geometric wireframe, tiny nodes, dark grey dividers, Korean sans-serif, spare technical game diagrams, no stars, no painted 3D, no creatures or armor. No filled hazard regions. All paths solid, never dashed. References are style only.
Title exact "보스 패턴 · 네 가지 시각 언어"
Subtitle "임시 본체 예시 · 외형과 개별 패턴은 확정 전"
Consistent original boss body in all columns: hollow hexagonal white core between two disconnected squared U frames facing inward, four periwinkle corner nodes; no orbital ellipses and no Quasar body. Cyan HOLLOW diamond is player, boss core is WHITE hollow hexagon. Grey arrows for movement explanation, blue/white for attacks.
Each column has top small preparation diagram, middle large execution diagram, and short captions beneath. Two diagrams separated by subtle horizontal line.
COLUMN A heading "A · 관통 돌진". Top caption "본체를 접고 직선 통로 고정". Prep body compressed narrow at LEFT of short diagonal corridor indicated by TWO THIN PARALLEL SOLID LINES, no center beam line. Cyan player already outside corridor. Main execution: draw large boss AT FAR END of corridor on RIGHT, same body obviously moved, with TWO faint ghost outlines of boss at earlier points LEFT. Ghosts are grey thin outlines. Only ONE SHORT GREY movement arrow behind boss, NO glowing long beam or continuous luminous trail. Caption exact "본체가 이동한다". Bottom response "대응 · 통로 옆으로 이탈". Boss has no laser weapon, corridor is empty except body.
COLUMN B heading "B · 편각 휩쓸기". Top caption "한쪽 프레임 전개 · 부채꼴 예고". Body at left opens one squared U frame into an angled arm, fan-shaped outline towards right. Main: body stationary at fan vertex left; ONE broad 80-degree sector outlined thin periwinkle, ONE bright WHITE RADIAL EDGE on upper side, one small GREY CURVED ARROW outside sector indicates angular sweep from lower side toward upper side. Cyan player safely behind boss outside sector. Caption "본체 고정 · 각도 회전". Response "대응 · 부채꼴 바깥으로".
COLUMN C heading "C · 격자 압착". Top caption "양쪽 프레임 분리 · 벽 두 개". Body at top. Below, LEFT and RIGHT narrow wireframe segmented polygonal walls. Main: same separated walls shifted INWARD, with short GREY inward arrows outside the walls. Walls stop with a clearly wide traversable OPEN VERTICAL CORRIDOR down the middle. Cyan player inside that clear corridor, short grey downward arrow along safe corridor. No solid connecting bar between walls, no rays from boss. Caption "벽만 이동 · 중앙 틈 유지". Response "대응 · 열린 통로로 이동".
COLUMN D heading "D · 결손 파동". Top caption "외곽 노드 정렬 · 빠진 고리 예고". Body centered, one thin C-shaped ring outline with LARGE OPEN WEDGE on right. Main ONE active bright C-shaped circular wavefront farther from stationary boss, a faint SMALLER ghost C ring for past position. ALL rings have IDENTICAL 90-degree missing wedge on right, draw NO sector boundary rays across gap. Cyan player located safely in missing wedge right, short grey movement arrow pointing radially OUTWARD through open wedge. Wave FRONT only is danger, empty interior unfilled and dark. Caption "틈을 유지하며 바깥으로 확장". Response "대응 · 고리의 틈 통과".
Bottom full-width strip: heading "예고부터 구별". Four aligned labels "좁은 통로", "넓은 부채꼴", "분리된 두 벽", "틈 있는 고리". Tiny footer exact "색보다 자세·도형·움직임으로 구별 · 회색 화살표는 설명용 · 수치와 밸런스는 미정".
Ample blank dark space, distinct silhouettes at small scale, precise Korean text. No timing numbers, no damages, no boss name. This is illustrative planning only.
```

## 시각 검토 기록

- 두 이미지의 제목, 두 규칙, 네 패턴 명칭과 패널 구성을 확인했다.
- 비교 시트 A는 본체의 과거 위치와 이동 후 위치를 함께 보여줘 돌진임을 구체화했다. 규칙 보드의 작은 아이콘만으로 빔으로 오해하지 않도록 본문에 비교 시트 우선 해석을 명시했다.
- B의 부채꼴과 D의 동심 윤곽은 전체 면적의 피해를 뜻하지 않는다. 본문에서 실제 위험은 회전선과 파면임을 명시했다.
- 첫 보드에는 일부 보조 점선과 약한 면 표현이 생성되었다. 이는 프롬프트의 선 중심 의도와 차이가 있으며, 최종 게임 표현 지침으로 채택하지 않는다. 비교 시트와 본문의 기획을 우선한다.
- 실제 플레이 크기, 흑백 인지, 충돌 범위, 회피 가능성은 검증 전이다.

