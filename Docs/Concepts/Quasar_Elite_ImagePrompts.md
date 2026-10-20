# QUASAR 엘리트 이미지 생성 기록

2026-10-17 · 내장 image_gen 사용. CLI/API fallback 미사용.

## 최종 산출물

- Quasar_Elite_Fall_Motion_v2.png — 최신 궤적 상세: 짧은 곡선 분리 후 직선 급강하.
- Quasar_Elite_PatternComparison_v2.png — 두 패턴 비교, 회피 화살표 수정본.
- Quasar_Elite_Fall_Storyboard.png — 패턴 2 동작 시트.
- Quasar_Elite_PatternComparison.png — 최초 생성 초안, 최종 문서에서는 v2 사용.

## 비교 시트 최초 생성 프롬프트

참조: Quasar_v3_States.png, Quasar_Charge_Storyboard.png, Genesis_Elite_PatternOptions.png.

```text
Use case: infographic-diagram
Asset type: Korean game enemy elite design proposal sheet, landscape 1800x1100 approximately.
Primary request: Create a precise technical concept comparison board for an EXISTING game enemy QUASAR gaining a second attack, matching the provided reference sheets. Image 1 and 2 are visual identity and graphic layout references; image 3 is comparison-layout reference. These are style references, not edit targets.
Style: near-black #080910 background, extremely thin blue/periwinkle and white geometric wireframe line art, flat orthographic top-down game diagrams, subtle minimal halo only, fine dark grey panel borders, bold crisp Korean sans serif headings with legible Korean text. This is a restrained technical drawing, NOT painterly space art. No 3D shading, no starfield, no decorative UI, no fill areas, no dotted lines. Existing enemy: bright small diamond core inside two tilted orbital ellipses, four small orbiting debris nodes, white clamps. Player is a small cyan hollow diamond clearly separate from enemy.
Layout: header, then two equally large columns A and B, then bottom comparison strip. Title verbatim "QUASAR 엘리트 · 두 패턴 구분 시안". Subtitle "같은 본체 · 다른 예고 · 다른 위험 형태 · 다른 회피".
Left heading "패턴 1 · 양방향 빔". Short text "원반 압축 → 축 고정 → 직선 발사". Draw an orbital quasar on left-middle of its panel, its tilted ellipse compressed around the fixed aiming axis. Long straight diagonal blue-white beam extends in BOTH directions from its core across panel, clipped at borders. Two thin parallel telegraph rails in a small inset above. Cyan player clearly OUTSIDE beam with a short sideways grey movement arrow. Caption "긴 직선 / 옆으로 이탈".
Right heading "패턴 2 · 강착 파편 낙하 (제안)". Text "노드 분리 → 지점 고정 → 순차 낙하". Quasar upper left in this panel, intact round-looking ellipses, no beam, no polar jets. Three existing blue debris nodes detach from outer orbit. Show three isolated thin circular landing telegraphs away from enemy in staggered arrangement lower-right, NON-overlapping, labeled 1, 2, 3. Each a single clean blue circle with four tiny inward notch marks. One circle is currently active with a bright white inner impact rim and a tiny blue diamond fragment at its center, the other two circles dim, NO connecting rays, no straight damage trails from enemy to circles, NO arrows that resemble beams. Player cyan diamond in clearly empty safe gap between circles, small grey movement arrow pointing out of a circle. Caption "분산된 원 / 빈 공간으로 이동".
Bottom strip exact headings "구분 기준", "실루엣", "예고", "위험 형태", "회피". Compare "납작한 원반 ↔ 열린 원반", "평행선 ↔ 독립 원 3개", "직선 ↔ 국소 원형", "횡이동 ↔ 지점 이탈". Small footer "색을 빼도 구별되어야 한다 · 사망 초신성은 전투 패턴으로 세지 않는다 · 수치는 후속 검증".
Keep ample negative space and clear small diagrams. All captions must be accurate Korean. Do not introduce a new creature or turret. The proposed falling debris are temporary attack effects, not new monsters. Reference design restraint and panel structure closely.
```

## 비교 시트 수정 프롬프트

수정 대상: Quasar_Elite_PatternComparison.png.

```text
Use case: precise-object-edit. Edit this existing Korean QUASAR comparison sheet with exactly ONE correction. In the large RIGHT panel B, a cyan diamond player appears between the three landing circles. There is a small thin GREY diagonal movement arrow immediately ABOVE AND RIGHT of that cyan diamond, currently pointing up-right into landing circle 1. REMOVE this grey movement arrow completely, leaving a clean black background there. Do not remove the cyan player diamond. Keep every other pixel, every heading, all captions, all blue curved debris arrows from the enemy, every circle, the beam left panel, and bottom comparison strip as unchanged as possible. No replacement arrow needed. The small grey arrow in question is around x=80%, y=51% of the full image, BETWEEN the central cyan diamond and circle 1. This corrects a misleading dodge arrow. Preserve the image framing and all Korean typography.
```

## 동작 시트 프롬프트

참조: Quasar_v3_States.png, Quasar_Charge_Storyboard.png.

```text
Use case: infographic-diagram
Asset type: Korean QUASAR elite falling-debris attack storyboard, wide landscape technical concept sheet.
Reference images 1 and 2: visual identity and thin wireframe storyboard style only. Match near-black background, blue-white fine solid lines, restrained orbital geometry, dark grey panel dividers, Korean sans-serif type. Not an edit. No painterly rendering, stars, 3D, large bloom, filled hazard areas or dotted lines.
Title exact "QUASAR 엘리트 · 강착 파편 낙하 동작 시안"
Subtitle "제안 패턴 2 · 예고 후 위치 고정 · 원 내부가 위험 · 착탄 뒤 잔류 피해 없음"
Five equal tall panels in one row, all same top-down scale and same scene coordinates. In each panel QUASAR stays UPPER LEFT, identifiable diamond white core, two blue thin orbital ellipses and four small debris nodes at rest. Three fixed landing circles arranged on the right/bottom; clearly separate, plenty of blank safe space; cyan hollow diamond player initially at landing circle 1, later OUTSIDE ALL CIRCLES. Explain using exact text:
Panel 1 heading "① 분리 · 0–0.65초", caption "원반이 열리고 파편 3개가 분리". Draw round orbital silhouette and three small nodes just outside orbit. No rays, no laser, no landing circles yet.
Panel 2 heading "② 고정 · 0.65초", caption "현재 위치와 주변 두 지점을 고정". Draw three thin blue isolated circles labelled 1 2 3, cyan player inside circle 1 ONLY IN THIS PANEL. A short grey arrow starts at this player and points OUTWARD into empty space outside all circles. Quasar remains top left, three detached nodes.
Panel 3 heading "③ 예고 · 0.65–1.65초", caption "세 원을 먼저 표시 · 아직 피해 없음". Same circles and enemy coordinates. Player has moved to empty space outside all three circles, short grey arrow points FURTHER INTO EMPTY SPACE, not toward a circle. Inside each circle show a tiny blue diamond fragment and four small inward notch marks, no beam connection. Circle 1 slightly brighter than others.
Panel 4 heading "④ 착탄 · 1.65–2.25초", caption "1 → 2 → 3 · 0.3초 간격". Circle 1 bright white inner impact rim plus tiny fragment at center, circles 2 and 3 dim with inward ticks. Cyan player safely OUTSIDE all circles. These are sequential impact instants, not simultaneously active. Small caption "각 착탄 순간에만 원 내부 피해".
Panel 5 heading "⑤ 회복 · 마지막 착탄 후 0.8초", caption "표식 소멸 · 노드 복원". Three circles completely gone, no lingering hazards. Enemy has regained four orbiting nodes. Player remains safe.
Bottom full width thin bordered annotation band with three short statements, each with a tiny wireframe icon:
"직선 빔 없음" with orbital core icon with no beam.
"고정 뒤 추적 없음" with circle icon and small fixed-center cross.
"사망 시 미착탄 취소" with fading tiny fragment icon.
Footer "실제 게임 캡처가 아닌 설계 시안 · 시간과 반경은 플레이 검증 후 조정".
No more text than requested. Diagrams medium sized, ample empty space, captions readable and correct Korean. Enemy is alive throughout, no collapse, supernova, or expanding global shockwave. Grey arrows are explanatory only. No arrows pointing player into hazard in panel 3 or 4.
```

## 궤적 상세 시트 — 후속 디자인

최종 이미지: `Quasar_Elite_Fall_Motion_v2.png`.

요청: 원반에서 분리될 때만 짧게 휘고, 고정된 착탄점으로 접근한 뒤 빠르게 직선 낙하하는 동작. 내장 `image_gen` 사용.

참조: `Quasar_Elite_Fall_Storyboard.png` (구조·팔레트), `Quasar_v3_States.png` (기존 본체).

```text
Use case: infographic-diagram.
Asset type: production game motion-design reference sheet in Korean. Generate a NEW refined motion sheet, not a pixel edit.
Input images: image 1 is a layout and palette reference (previous storyboard); image 2 is the source QUASAR creature identity. Match thin blue/periwinkle and white solid geometric wireframe on almost-black #080910, crisp Korean sans serif labels, restrained minimal glow, flat top-down diagrams with fine grey panel borders. Keep existing diamond core, tilted accretion ellipses, four orbit nodes. Not painterly, not 3D sci-fi artwork, no starfield, no filled circles or dotted lines.
Main subject: QUASAR elite releases existing three debris nodes for three separate local circular impacts. The KEY change to communicate unmistakably: debris follows a SHORT LOCAL CURVE only while peeling away from its rotating disc, then moves toward FIXED target locations and finishes with a FAST STRAIGHT DESCENT. It never gracefully circles all the way to the target, never spirals around the player, never follows the player after targets lock. Motion trails are very short and temporary, not persistent damaging beams.

Layout landscape approx 1900x1200. Header exact "QUASAR 엘리트 · 파편 낙하 궤적 설계"
Subtitle exact "짧은 곡선 분리 → 위치 고정 → 빠른 직선 낙하 → 국소 착탄"
Top row FOUR storyboard panels about 65% of height, bottom row TWO detail panels about 25% of height.

Top panel 1:
heading "① 짧게 휘어 분리"
caption "0–0.65초 · 원반의 회전을 이어받음"
Draw quasar left lower-middle with THREE blue diamond debris nodes visibly outside the orbital rim, only ONE node left ON the rim. Beside each separated node draw a very SHORT curved grey explanatory arrow tangent to disc (less than one-half body radius, arc no more than 45 degrees). Curve confined immediately outside disc, not spanning the panel. No full orbit paths outside body. Label "분리 순간에만 곡선".

Top panel 2:
heading "② 착탄점 고정"
caption "0.65초 · 원 3개 동시 예고"
Quasar left upper area, three thin blue separated circular markers in right/lower area labeled 1 2 3. Player CYAN HOLLOW diamond starts inside circle 1; short grey arrow from player points OUTSIDE ALL CIRCLES into clear space. Small caption "고정 뒤 플레이어 추적 없음". No connections from enemy to circles. Three debris shown as small faint diamonds near the upper side of each target, representing approach at height.

Top panel 3:
heading "③ 빠르게 직선 낙하"
caption "각 착탄 직전 0.25초"
Show the same scene as panel2 but cyan player safely OUTSIDE circles. Circle1 with a medium blue-white diamond fragment just ABOVE its exact center, a SHORT near-vertical tapered WHITE trail ABOVE the fragment, length under the marker radius. A short grey straight downward explanatory arrow adjacent to fragment points exactly toward circle1 center. No long curved arrows, no line from enemy to landing points. Markers 2/3 dim with tiny waiting debris. Caption "이동 경로에는 피해 없음".

Top panel 4:
heading "④ 원 안에서만 착탄"
caption "1.65 / 1.95 / 2.25초"
Same circle positions, white narrow ring at circle1, small 3-4 angular fragments spreading only INSIDE circle1. No huge explosion, no filled area, no global shockwave. Cyan player safely outside all circles. Circles2/3 dim yet pending. Caption "0.3초 간격 · 착탄 뒤 잔류 피해 없음".

Bottom left detail panel heading "궤적 확대 · 한 파편 기준".
Draw a small orbital rim on far left, ONE short blue curved path peeling 30 degrees from rim, then a GREY straight explanatory arrow leading right toward a floating small fragment, then a SHORT blue-white straight downward trail ending in a thin circular landing marker at far right. Use generous space. Only the short path near orbital rim is curved. Main scene paths never form a smooth long arc. Label beneath each of these three phases exactly "짧은 곡선" "표식으로 접근" "직선 급강하". Footer within panel "회색 화살표 = 설명용 / 실제 잔상은 짧게".

Bottom right detail panel heading "착탄 전후 · 같은 지점".
Three small side-by-side circle diagrams with labels "예고: 피해 없음" "착탄: 원 내부 피해" "종료: 즉시 소멸".
First a single thin blue circle with inward ticks and tiny blue diamond above center.
Second same circle with small white central impact and white outline, no fill.
Third circle COMPLETELY absent, just a few tiny fading chips and empty dark space.
No cyan player in detail icons, use blue/white fragments only.
Footer whole sheet exact "기존 본체 유지 · 빔 없음 · 파편 경로 피해 없음 · 사망 시 미착탄 취소".
All Korean type accurate, high readability. Distinguish cyan hollow PLAYER diamond from smaller filled blue-white DEBRIS diamonds. This is motion design reference, not gameplay screenshot. Keep diagrams clean and plenty of negative space.
```


