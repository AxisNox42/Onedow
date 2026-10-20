# GENESIS 로터 집속·반동 v2 — 이미지 생성 기록

작성일: 2026-10-18 · 도구: 내장 `image_gen`

최종 출력: `Genesis_Elite_Rotor_GatherRecoil_v2.png`

입력: `Genesis_Elite_Rotor_Storyboard_v1.png` — 본체 정체성과 단색 선화 구성. 외피 개방을 전방 집속으로, 작은 로터를 대형 로터로, 약한 발사 반응을 큰 후퇴·복귀 동작으로 수정했다.

## 구조 변경 프롬프트

```text
Edit/re-design the attached Genesis ROTOR storyboard into version 2 following the user's specific mechanical correction. The input is an edit target for its subject and visual board style. Keep flat dark charcoal background, thin gray panel borders, white Korean typography, solid coral-red outlines, cyan player, gray explanatory arrows. NO bloom, NO gradient glow, NO haze. Wide landscape technical 2D game concept board. Highly readable.
CRITICAL NEW MECHANISM: Do NOT open the three shell petals outward into a symmetric circle during firing. Genesis starts with three distinct curved faceted cradles around a small hex core. During preparation ALL THREE original shell pieces physically swing/slide to ONE forward side and gather in a tightly layered stack around a single launch axis, like three nested curved jaws bracing one barrel. The rear hex core remains visible behind the converged three-shell assembly. Side-biased asymmetric firing silhouette; do not replace the shells with a new conventional gun. Show the same three original pieces and gray arrows explaining how they gather.
It launches LARGE square ROTOR frames with four corner nodes and a central dot; about 1.6x the small triangular SWARM's width, and visibly heavier. Strong recoil every shot: whole core and gathered shell stack visibly kick BACK opposite the launch direction, shell braces compress/backslide and oscillate slightly, then return forward before next shot. A few thin straight motion streaks only. No explosions or dismemberment. Recoil is displacement, not just bright flash.
Title: "GENESIS 엘리트 · 외피 집속과 중량 출격"
Subtitle: "세 외피를 한 방향으로 모아 발사 · 큰 로터와 강한 반동"
FOUR TOP PANELS:
1 header "① 외피 집속 · 0.4초"
caption "세 외피가 전방 한곳으로 모임"
Show two small states connected by arrow: original three-shell ring body, then clearly ASYMMETRIC assembled body with all THREE curved shell components nested on the RIGHT of the small hex core, shared muzzle toward right. Gray curved assembly arrows, no outward-petal pose in final state.
2 header "② 중량 로터 발사"
caption "본체 후퇴 + 집속 외피 압축"
Large assembled body launching ONE large square Rotor to RIGHT. Hex core and nested shell assembly kicked visibly LEFT relative to a thin gray ghost outline of their pre-shot position. Fat gray arrow labeled "반동" pointing LEFT; coral thin arrow points RIGHT with Rotor. Keep shell pieces gathered at the front even while compressed. In lower corner show small triangle and larger square with text "SWARM ↔ 대형 ROTOR". Square width around 1.6x triangle width, not giant relative to body.
3 header "③ 복귀와 재조준"
caption "반동 복귀 중 다음 위치 조준"
Same assembled body returning forward from its recoil offset via gray arrow; all three shells stay stacked at front, entire launch assembly turns as one rigid unit to new cyan player position. Old gray diamond above and cyan new diamond below, two fixed straight flight trajectories for previous and new square rotors, NO homing curves. Small label "0.3초마다 반복".
4 header "④ 연속 출격 종료"
caption "마지막 반동 정리 → 원래 배치 복귀"
Assembled body at upper part finishing recoil; gray arrows lead three shell pieces from gathered front stack back to original three-around-core resting arrangement shown below as second state. No new shell pieces. A surviving large Rotor at side with text "비행 종료 후 일반 추적".
BOTTOM left 65%: a small 3-frame recoil motion study of the ASSEMBLED front-stack body pointing RIGHT throughout, identical baseline hex center ghost cross so displacement is readable:
Heading "한 발의 반동 · 0.3초 안의 리듬"
frame labels "발사 직전" / "강한 후퇴" / "완만한 복귀"
At middle frame core and shell stack shift left, braces compress; last frame returns right near baseline. Draw thin gray origin vertical reference lines. Under frames exact label "빠르게 밀림 → 천천히 복귀 → 다음 발"
BOTTOM right 35%:
Heading "유지하는 공격 규칙"
Text exactly four short lines:
"0.3초마다 1기 · 총 6기"
"매 발사 직전 현재 위치 조준"
"발사된 로터는 직진"
"착지 원·착지 폭발 없음"
Footer: "컨셉 v2 · 단색 선화 · 반동은 외형 연출 기준 · 수치는 초기 검증값"
Important avoid old open flower silhouette in firing panels. Strong visible asymmetric gathering, big square rotors, exaggerated but readable mechanical backward recoil. No glow/gradients.
```

## 검토 후 교정 프롬프트

발사 반동의 이전 위치 잔상을 현재 몸체 오른쪽에 두어 실제 후퇴 방향과 맞췄다. 크기 비교의 작은 SWARM 아이콘도 삼각형으로 교정했다.

```text
Edit this Genesis recoil storyboard with ONLY two precise corrections, preserving all other drawings, layout, Korean text and flat coral outline style.
1) In panel ② "중량 로터 발사", the thin GRAY ghost of the pre-shot Genesis position must be to the RIGHT of the solid coral Genesis, because firing RIGHT pushes the current solid body LEFT. The gray arrow labeled "반동" continues pointing LEFT. Put the solid coral gathered-shell Genesis visibly LEFT of its gray pre-shot ghost. Keep the large Rotor flying RIGHT. This is backward recoil, not forward lunging.
2) In the little size-comparison inset at the bottom of that same panel, beneath "SWARM ↔ 대형 ROTOR", replace ONLY the SMALL LEFT square icon with a simple small hollow TRIANGLE (SWARM). Keep the LARGE RIGHT square Rotor with its four corner nodes. The visual ratio is approximate; no need to change its caption.
Do not alter the three gathered shell components, no open petals in firing state, do not add glow or gradients. All other panels unchanged.
```

