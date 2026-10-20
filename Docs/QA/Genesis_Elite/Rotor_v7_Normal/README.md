# Ordinary Rotor summons and unlimited bombardment range

2026-10-18. Summoned Rotors now use ordinary Rotor color, base HP, final scale 1.0, and shared frame/node/core rendering. Formation still grows one actual unit through the body. The aperture calculation includes the ordinary nodes. The subdued summon halo remains.

Removed both distance clamps. The actual release position determines the direction to the saved player position, and every shot reaches that exact position. A numerical epsilon only prevents zero-duration division. Cadence remains 0.4s; landing radius 80px and one-time damage 10 remain.

Validation:

- Release build passed with existing APIENTRY macro warnings.
- `scripts/test_genesis_rotor.ps1` passed, including ordinary color/HP/scale and landing AoE at 0, 20, 2500 and 10000px. Existing formation, cadence, cancellation, cap, AoE and alternation checks passed.
- Native smoke passed at 1440x900: `GENESISROTOR=0.96;GENESISROTOR=1.14;GENESISROTOR=1.32;GENESISROTOR=1.62;GENESISROTOR=3.24`, tag `genesis_rotor_normal_v7`. Outputs are in `build/smoke/genesis_rotor_normal_v7/`. Inspected the 1.32s landing capture for ordinary body rendering and the retained area pulse.
