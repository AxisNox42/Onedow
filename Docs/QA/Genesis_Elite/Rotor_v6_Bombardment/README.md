# Genesis Rotor — reference appearance and area bombardment

2026-10-18.

- Orange square outline, hollow corner nodes, orange center ring and white core, matching the supplied reference. Low flat glow; no strong gradient. The same unit still forms while moving through the body over 0.24s and fits between the guide shells.
- Six launch attempts at 0.4s intervals, approximately 1.0 / 1.4 / 1.8 / 2.2 / 2.6 / 3.0s after preparation starts. Preparation remains 1s, Rotor pause 0.25s, recovery 0.55s.
- Each shot fixes its landing point from the current aim. Flight distance uses target distance clamped to 120–1100px, replacing the former overshoot. The displayed 80px landing circle matches the damage radius.
- Landing deals 10 damage once inside that radius. Flight contact no longer damages or consumes the unit. A 0.25s shockwave is visual only after that one hit, followed by ordinary Rotor chase. Shot-down ammunition and canceled flights do not detonate.

## Validation

Release build passed with existing APIENTRY macro warnings. `test_genesis_rotor.ps1`, `test_genesis_launcher.ps1` and `test_thrown_swarm_kill.ps1` passed. Rotor assertions cover 0.4s cadence, current aim, area inside/outside boundaries, exactly one hit, absence of direct flight damage, immunity, time freeze, cancellation at the pending blast position, summon limits, continuous formation and aperture clearance. Original SWARM throw and no-farm summon rewards remain covered.

Native smoke: `genesis_rotor_bombardment_v6`, 1440x900, `GENESISROTOR=0.00` through `3.78` in 0.06s steps. GIF frame durations use the capture interval; no synthetic movement or slow motion. Inspected first-shot flight and impact captures at 1.20s and 1.32s for the frame, core, warning and shockwave.

- [Bombardment closeup](Bombardment_Closeup.gif)
- [Formation closeup](Production_Closeup.gif)
- [Full cycle](Rotor_Preview.gif)
- [Motion contact sheet](Motion_ContactSheet.png)
