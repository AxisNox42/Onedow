# 조율자 — 성도 주사 명중 효과 이미지 생성 기록 v1

2026-10-19 · 내장 `image_gen` 사용 · CLI/API fallback 미사용

출력: `Boss_Tuner_Scan_HitEffects_v1.png`

참조: `Boss_Tuner_Patterns_3_4_v1.png`, `Boss_Tuner_Ignition_Control_v1.png`. 조율자 임시 본체, 열린 선 구조, 도표 구성을 참고했다. 좌표 도약은 보류하고 현재 성도 주사를 세 번째 패턴으로 표시했다. 기존 보스 문서 미참조.

```text
Use case: infographic-diagram.
Asset type: Korean game boss skill-hit-effects planning board, wide landscape technical reference.
Primary request: Refine boss "조율자" active third pattern "성도 주사" (formerly fourth) so a successful hit deals damage AND applies a SHORT slow AND TEMPORARY vulnerability to damage from IGNITED MONSTERS ONLY. Active set is now three patterns: ignite monsters, command ignited monsters, scan attack. The old teleport pattern "좌표 도약" is ON HOLD, do not draw it.
Use attached image for SAME boss identity and graphic style only: black #080910, fine solid blue-periwinkle/white line art, hollow geometric body, tiny white core, THREE disconnected angular graduated frames, thin dark gray panel borders, legible Korean sans serif, minimal glow, low-contrast background contour lines visible through body. No painterly sci-fi, no starfield, no filled hazard sectors, no opaque body, no dotted lines.
Cyan HOLLOW DIAMOND is player. IGNITED MONSTER = small hollow SQUARE mob with tiny FOUR white star rays around its core. Player's vulnerability marker is TWO small corner brackets ABOVE player's icon with SMALL partial progress arc, clearly not a star and never attached to boss. Timer arc only; no unreadable HUD.
Title exact "성도 주사 · 명중 후 점화 몹과 연계"
Subtitle exact "직접 피해 + 둔화 + 일정 시간 점화 몹에게 받는 피해 증가"
Layout THREE equal vertical panels above, TWO short bottom horizontal strips.
Panel 1 heading "① 예고 → 한 줄 주사".
Draw boss on LEFT of panel middle, with THREE separated frames aligned as a long measuring axis. ONE narrow white ray extends toward right across player's cyan diamond, which is slightly offset but visibly touching active ray. Two thin blue boundary rays and outer arc illustrate wide open fan, dark interior unfilled. Single small grey curved arrow outside fan shows sweep direction. One tiny white hit spark at player indicates single valid hit, not explosion. Caption "현재 밝은 선에 닿았을 때만 명중". Small bottom line "한 번의 주사당 최대 한 번 적용".
Panel 2 heading "② 피해와 상태 적용".
Large cyan hollow player diamond in middle, with TWO small WHITE corner brackets ABOVE it and one small incomplete blue timer arc beside brackets. A tiny short GREY movement arrow below player and two faint short grey tail marks convey slowed movement, no ice or opaque effects. Three well-spaced text labels with small fine icons: "피해 · 즉시", "둔화 · 짧게", "관측 노출 · 일시". Caption "둔화와 피해 증가의 강도는 중첩하지 않는다". Smaller text "지속시간·수치는 미정".
Panel 3 heading "③ 점화 몹의 공격 연계".
An ignited hollow square mob LEFT, white permanent four-ray star marker; cyan player RIGHT with temporary corner brackets ABOVE. One SHORT GREY curved movement arrow near mob suggests coming attack but no damaging long beam or literal linking wire. A small simple hollow projectile originating next to mob may indicate its attack, tiny periwinkle dot no trail. Main caption "점화 몹의 피해만 증가". Second line "일반 몹·보스 피해는 그대로". Under diagram one small square plain mob icon and one boss mini icon, each grey and clearly separate from legend text, no attack arrows.
Bottom upper strip heading "연계 중에도 대응할 시간". Exact sequence using plain text and short GREY arrows "주사 종료 → 회복 → 조율 예고 → 점화 몹의 행동". Caption underneath "둔화가 풀린 뒤 다음 조율 공격에 대응하는 안부터 검토".
Bottom lower strip heading "현재 구성". Three compact labels "1 · 성좌 점화", "2 · 성좌 조율", "3 · 성도 주사". Separate tiny right note "좌표 도약 · 보류".
Footer exact "기획 시안 · 공격 수를 늘리기보다 연계를 먼저 검토 · 실제 난이도와 가독성은 검증 전".
All labels correct Korean. This illustrates a successful HIT rather than global unavoidable status; dodging ray applies none of these effects. Permanent ignition exists ONLY on monster; player debuffs temporary. No arbitrary numbers or percentages. Preserve open geometry and visible background.
```

검토: 주사선 명중, 직접 피해·둔화·관측 노출, 점화 몹의 피해만 증가하는 연계와 회복·예고 순서를 확인했다. 가운데 효과 설명의 마름모 아이콘은 설명용이며 플레이어의 변신·점화 표현이 아니다. 아래 ‘회복’은 보스 패턴 사이의 대기 구간으로 체력 자동 회복을 뜻하지 않는다. 실제 배경 가독성·효과 시간·연속 피격 가능성은 검증 전이다.

