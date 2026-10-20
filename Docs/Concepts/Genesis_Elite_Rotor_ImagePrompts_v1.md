# GENESIS 로터 연속 출격 — 이미지 생성 기록

작성일: 2026-10-18 · 도구: 내장 `image_gen`

## 패턴 비교

출력: `Genesis_Elite_Rotor_PatternComparison_v1.png`

참조: 기존 `Genesis_Elite_PatternComparison_v2.png`의 패널 구성과 본체 정체성. 광번짐은 줄이고 평면적인 단색 선으로 변경.

```text
Use case: infographic-diagram
Asset type: Korean game enemy pattern comparison concept board, wide landscape, crisp readable raster design reference.
Input image 1: layout and Genesis identity reference ONLY. Replace its right-side squad rush idea with the new continuously re-aimed ROTOR launch pattern. Generate a new board; do not retain the old squad rush or corridors.
CRITICAL STYLE CHANGE requested by user: far LESS gradient and glow. Flat uniform near-black charcoal background (#101217). Flat coral-red strokes (#EF766D), flat off-white text, cyan player. Nearly diagrammatic technical line illustration. NO bloom, NO light haze, NO radial gradient, NO shiny highlights, NO soft glowing tails, NO vignette, NO red tinted area fills. Preserve brightness/readability by crisp 2px contours and 3px silhouettes, not light blur. A tiny flat white core is okay. Broad empty spacing. Clear dark gray panel borders.
Title verbatim: "GENESIS 엘리트 · 로터 연속 출격"
Subtitle: "기존 투척과 구별 · 매 발사 시점에 현재 위치 조준"
Layout: two equal large panels across top-middle; four compact comparison cells at bottom. Readable Korean text, short labels only.
Subject invariant: Genesis is ONE small central hexagonal core, exactly THREE broad faceted curved shell cradles on radial spokes, one front gap. Use simple flat outline geometry derived from reference, fewer decorative contours. Same body in both panels.
Left header: "A. 기존 패턴 · SWARM 투척"
Small line: "외피 결속 → 세 마리 투척 → 착지"
Scene: Genesis at left aims one front muzzle toward cyan diamond player on right. Three small triangular SWARM units with short straight flat tails, three thin aim lines, three thin landing circles. No bloom anywhere. Show gray escape arrow leading player out of circle.
Left bottom label: "삼각형 SWARM · 착지 원 · 지점 회피"
Right header: "B. 교체 제안 · ROTOR 연속 출격"
Small line: "0.4초 개방 → 0.3초 간격 × 6기 → 회복"
Scene: same Genesis at upper-left of panel with its three shells modestly open. An opened front launch channel points toward CURRENT cyan diamond player at lower-right. Draw player movement as a light gray curved dotted path with two hollow gray diamond previous positions labeled "이전 위치", and solid cyan CURRENT diamond labeled "현재 위치". Clearly show exactly THREE example ROTOR units already in flight as small SQUARE FRAMES with FOUR corner nodes and small center, not triangular, not round bullets. Each flies along its own distinct straight diverging trajectory from Genesis: old two projectiles heading toward old player positions, newest projectile heading to CURRENT position. Every projectile has just two short thin solid tail strokes; these are flight paths, NOT beams. Old trajectories thin GRAY explanatory arrows, newest thin red aiming segment. Show small gray curved arrow near Genesis front meaning launcher turns between shots.
Prominent right-side two-line callout: "매 발사 직전 조준 갱신" / "발사 후에는 직진"
Important: no homing curved projectile paths, no future player prediction, no six simultaneously spread warning lines. No landing circles on right, no exploding floor, no paired squad lanes, no swarm formations. Three displayed Rotors are examples from six total, not six all at once.
Right bottom label: "사각형 ROTOR · 연속 직선 출격 · 횡이동 / 격추"
Bottom four cells, no tiny paragraphs:
"본체" / "외피 결속 ↔ 외피 개방·조준 회전"
"소환체" / "삼각형 SWARM ↔ 사각형 ROTOR"
"조준" / "투척 전 고정 ↔ 매 발사마다 갱신"
"위협" / "착지 지점 ↔ 연속 비행 몸체"
Footer: "컨셉 제안 · 6기는 초기 검증값 · 회색 선은 설명용 · 비행 중 격추 가능"
No unrelated diagrams, no glossy spaceships, no unreadable decorative technical data.
```

## 진행 스토리보드

출력: `Genesis_Elite_Rotor_Storyboard_v1.png`

참조: 새 패턴 비교 이미지의 본체·소환체·단색 선화 스타일.

```text
Use case: infographic-diagram
Asset type: Korean game enemy attack storyboard, wide landscape. New image matching input board's clear flat technical illustration style and exact Genesis/ROTOR identities.
Input image 1: visual subject, palette and graphic style reference. Produce a separate FOUR-STAGE storyboard, not a copy of its comparison layout.
CRITICAL user request: reduce gradients. Uniform flat charcoal (#101217) background, SOLID CORAL outlines (#EF766D), off-white typography, cyan player diamond, gray explanatory arrows. NO glow or bloom, no haze, no soft gradients, no vignette, no filled light cones, no area glow. Crisp readable contours and large Korean labels. Maintain plenty of dark empty space between objects.
Genesis body: small hexagonal central core with exactly THREE large faceted curved shell cradles on spokes. ROTOR: small square FRAME with four round corner nodes and center dot, never triangle or disk. Every depiction keeps this identity.
Title: "GENESIS 엘리트 · 로터 연속 출격"
Subtitle: "짧게 열고 바로 발사 · 0.3초마다 현재 위치를 새로 조준"
Layout: top row FOUR equal storyboard panels across 75% of height, lower row clean large timeline and small aim-rule diagram.
Panel 1:
Header "① 짧은 준비 · 0.4초"
Caption "세 외피 개방 · 전방 출격구 정렬"
Large Genesis with shells opening slightly outwards, two gray short outward motion arrows. One square Rotor forming inside central core only, no six units pre-deployed, no swarm squads. Cyan diamond distant.
Panel 2:
Header "② 첫 출격 · 0.4초 시점"
Caption "현재 위치 조준 → 1기 발사"
Genesis at upper-left, cyan player diamond lower-right. One square Rotor halfway along thin straight coral shot ray from single front port toward cyan diamond. Exactly one gray forward flight arrow. Minimal tails, no beam effect, no circular impact target.
Panel 3:
Header "③ 연속 출격 · 0.3초 간격"
Caption "이동한 위치로 갱신 · 총 6기"
Same Genesis, its front direction turned toward CURRENT cyan diamond at bottom right. Show two hollow gray previous player diamonds above it connected by dotted gray player movement path. Exactly three example square Rotors: #1 along straight gray old trajectory toward first old diamond, #2 along straight gray trajectory toward second old diamond, #3 along straight coral newest trajectory toward cyan current diamond. Label "이전 발" gray and "새 발" coral. Old projectile paths never bend toward new position. The three are sequential snapshots/examples, not a squad. No ground circles. Small callout "발사 후 재유도 없음".
Panel 4:
Header "④ 종료 · 감속 후 추적"
Caption "외피 복귀 · 생존 로터는 일반 추적"
Same Genesis with shells returning to compact form, small inward gray arrows. Two surviving square Rotors nearer player, short gray curved arrows indicate normal chase ONLY in this panel after launch flight ends. Cyan diamond shows sideways escape arrow. One hollow broken square with small gray cross labeled "격추 가능". No explosions, no floor circles, no cage.
Lower-left timeline section 65% width:
Heading "발사 시간표 · 준비 시작 기준"
Horizontal thin white time axis, one gray bracket for "준비 0.4초" from 0.0 to 0.4, then SIX evenly spaced coral firing ticks and small square icons. Exact times below ticks: "0.4" "0.7" "1.0" "1.3" "1.6" "1.9초". Exact labels above tick sequence: "1기" "2기" "3기" "4기" "5기" "6기". Underline bracket from first to last: "첫 발부터 마지막 발까지 1.5초". Each gap is 0.3, do not draw seven shots.
Lower-right small aim-rule section:
Heading "조준 규칙"
Large simple two-line statement:
"본체: 다음 발의 조준을 갱신"
"비행 로터: 정해진 방향으로 직진"
Diagram of two square rotors on two diverging straight arrows, cyan player moved between old gray diamond and new cyan diamond. No curved homing flight in this diagram.
Footer: "컨셉 제안 · 시간·수량은 초기 검증값 · 회색 선은 설명용"
Make text accurate and large, do not add paragraphs. Flat infographic, not luminous fantasy painting.
```

