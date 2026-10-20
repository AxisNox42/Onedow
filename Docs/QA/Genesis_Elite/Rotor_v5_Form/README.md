# Genesis Rotor — moving production

2026-10-18. Completed ammunition no longer appears behind the shell. One actual Rotor starts inside the body at 15% scale, builds its four edges and nodes while moving forward for 0.24s, and leaves the muzzle at its final scale. There is no separate preview entity.

Final scale comes from the shell aperture, including the rotating square diagonal and corner nodes, with 12 world px total clearance. Standard Genesis produces approximately scale 1.05 instead of 1.35. Preparation remains 1s, cadence 0.3s, six attempts, pause 0.25s. Original SWARM throwing remains enabled.

## Validation

- Release build passed; existing APIENTRY macro warnings remain.
- `test_genesis_rotor.ps1` passed: continuous scale, one entity through launch, aperture clearance, six shots, current aim, cadence, collision, cap, owner death and alternation.
- `test_genesis_launcher.ps1` passed: original three SWARM throw behavior.
- `test_thrown_swarm_kill.ps1` passed: original SWARM and summoned Rotor rewards.
- Native `GENESISROTOR=<age>` smoke captures at 1440x900, sampled every 0.04s. Preview GIFs use actual captures and 40ms per frame. No synthetic movement or slow motion.

The initial 96-frame capture batch reached its 90s process timeout after 52 images. The remaining 44 ages were captured in a second batch. This is a capture throughput limit; the game did not crash.

## Artifacts

- [Moving production closeup](Production_Closeup.gif)
- [Preparation closeup](Prep_Closeup.gif)
- [Whole Rotor cycle](Rotor_Preview.gif)
- [Production frame sequence](Production_ContactSheet.png)

Inspected the sequence from 0.76–1.04s: frame grows from inside the core, traverses the space between front guides, then continues into flight with recoil. Formation is harmless; live mixed-wave balance remains a gameplay tuning task.
