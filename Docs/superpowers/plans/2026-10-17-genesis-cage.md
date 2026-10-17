# GENESIS Cage (포위 진형) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give GENESIS its elite second pattern: its live thrown Swarms become the vertices of a polygon around the player that locks and shrinks.

**Architecture:** Pure geometry and tuning go in a new header `WiNILL/Entity/GenesisCage.h` (same split as `GravisField.h`). The pattern runs as a new Genesis `hivePhase 4` inside `MonsterManager::UpdateGenesisLaunchers`. Vertex Swarms use a new `thrownPhase 3`, so the existing early return in `Monster::Update` keeps them from chasing or dealing contact damage. Drawing is one manager-level pass, `DrawGenesisCages`, modeled on `DrawRegulusBeams`.

**Tech Stack:** C++17, MSVC `cl`, OpenGL batch renderer (`drawLineQuad`), headless assert tests run by PowerShell scripts.

**Spec:** `Docs/superpowers/specs/2026-10-17-genesis-cage-design.md`

## Global Constraints

- Tuning values, verbatim from the spec: COMMAND_TIME 0.4, DEPLOY_TIME 0.8, LOCK_TIME 1.5, RADIUS 240, MIN_NODES 3, MAX_NODES 6, RECRUIT_RANGE 600, COOLDOWN 12 (a fresh Genesis also starts at 12), EDGE_HIT_RADIUS 14, EDGE_DAMAGE 12.
- Visuals: solid lines only. No dashed or dotted lines, no filled faces, no backdrop plates, no new projectiles.
- Pattern 1 (charge → throw 3) must behave exactly as before. `tests/genesis_launcher_test.cpp` must keep passing.
- Comments in English, matching the surrounding density; in-game copy in ko/en/ja.
- **Precondition:** the working tree already holds uncommitted Gravis field work in `Monster.h`, `EntityDraw.cpp`, `main.cpp`, `Codex.h`, `WiNILL.vcxproj(.filters)` and `CMakeLists.txt`. Do not start Task 1 until the user has decided how that work is committed. Never commit hunks this plan did not write.

## Review Focus

1. **Player dashes across a locked edge.** Expected: same rule as Swarm contact damage. `Monster::ContactDamage` path has no dash gate, so the cage has none either. Confirm by hand in Task 3 that dashing through a Swarm and through an edge feel the same.
2. **Two Genesis cage at once.** Expected: each cage only uses its own Swarms (`thrownOwnerId`), and edges never connect across cages. Test in Task 2: `CheckTwoGenesisKeepTheirOwnNodes`.
3. **Vertex killed during the command or deploy stage.** Expected: its slot stays empty, the other vertices keep their slots, and the cage ends at once if fewer than 2 are alive. Test in Task 2: `CheckKilledNodeOpensGap` kills one vertex during deploy.
4. **Vertex Swarms drawn as landing-stunned.** `drawMob` treats every `thrownPhase != 0` as flight or stun (shake, landing ring, ghosts). Expected: vertices look like ordinary Swarms. Fixed in Task 3, Step 2.
5. **Long launcher tests now reach the 12 s cooldown.** Expected: existing assertions are unchanged. If a launcher test fails only because a cage started, set `g->cageCooldown = 1e9f;` in that test's setup. Do not touch the assertions. Covered in Task 2, Step 6.

---

### Task 1: Cage geometry header

**Files:**
- Create: `WiNILL/Entity/GenesisCage.h`
- Create: `tests/genesis_cage_test.cpp`
- Create: `scripts/test_genesis_cage.ps1` (copy of `scripts/test_genesis_launcher.ps1` with `genesis_launcher` → `genesis_cage` and the output dir `build\tests\genesis-cage`)
- Modify: `WiNILL/WiNILL.vcxproj`, `WiNILL/WiNILL.vcxproj.filters` (add a `ClInclude` for `Entity\GenesisCage.h` next to `Entity\GravisField.h`; header-only, so no CMake change)

**Interfaces:**
- Produces, in `namespace GenesisCage`:
  - `inline constexpr float COMMAND_TIME, DEPLOY_TIME, LOCK_TIME, RADIUS, RECRUIT_RANGE, COOLDOWN, EDGE_HIT_RADIUS, EDGE_DAMAGE;` `inline constexpr int MIN_NODES, MAX_NODES;`
  - `float SlotAngle(float a0, int slot, int count)` = a0 + slot·2π/count
  - `float DeployBlend(float t)` = smoothstep of clamp(t / DEPLOY_TIME, 0, 1)
  - `float LockRadius(float t)` = RADIUS · (1 − clamp(t / LOCK_TIME, 0, 1))
  - `float SegmentDistSq(float px, float py, float ax, float ay, float bx, float by)`: squared distance from point to segment (clamped)

- [ ] **Step 1: Write the failing test** `CheckGeometry()` in `tests/genesis_cage_test.cpp` (include `MonsterManager.h`, `<cassert>`, `<cmath>`, `<cstdio>`; reuse a `Near(a, b, eps = 1e-3f)` helper like `gravis_field_test.cpp`):

```cpp
using namespace GenesisCage;
assert(Near(SlotAngle(0.3f, 2, 5), 0.3f + 2.0f * 6.2831853f / 5.0f));
assert(Near(DeployBlend(0.0f), 0.0f) && Near(DeployBlend(DEPLOY_TIME * 0.5f), 0.5f));
assert(Near(DeployBlend(DEPLOY_TIME), 1.0f) && Near(DeployBlend(5.0f), 1.0f));
assert(Near(LockRadius(0.0f), 240.0f) && Near(LockRadius(0.75f), 120.0f));
assert(Near(LockRadius(1.5f), 0.0f) && Near(LockRadius(9.0f), 0.0f));
assert(Near(SegmentDistSq(0, 5, -10, 0, 10, 0), 25.0f));
assert(Near(SegmentDistSq(20, 0, -10, 0, 10, 0), 100.0f));   // past the end
assert(Near(SegmentDistSq(3, 4, 0, 0, 0, 0), 25.0f));        // zero-length
```
plus `int main()` that calls it and prints `genesis cage regression passed`.

- [ ] **Step 2: Run** `powershell -File scripts/test_genesis_cage.ps1`. Expected: build fails, `GenesisCage` is undeclared.
- [ ] **Step 3: Implement `WiNILL/Entity/GenesisCage.h`** with the constants from Global Constraints and the four functions above. Top-of-file comment in the style of `GravisField.h`.
- [ ] **Step 4: Run** `powershell -File scripts/test_genesis_cage.ps1`. Expected: `genesis cage regression passed`, exit 0.
- [ ] **Step 5: Commit**

```bash
git add WiNILL/Entity/GenesisCage.h tests/genesis_cage_test.cpp scripts/test_genesis_cage.ps1 WiNILL/WiNILL.vcxproj WiNILL/WiNILL.vcxproj.filters
git commit -m "feat(genesis): add cage geometry header for the elite second pattern"
```
(If the `.vcxproj` files still hold Gravis WIP, follow the precondition and do not commit them here.)

---

### Task 2: Cage state machine

**Files:**
- Modify: `WiNILL/Entity/Monster.h`: include `GenesisCage.h`; fields; `UpdateThrownSwarm`
- Modify: `WiNILL/Entity/MonsterManager.h`: `UpdateAll`, `UpdateGenesisLaunchers`, `ResolveMonsterCrowding`, new members
- Test: `tests/genesis_cage_test.cpp`

**Interfaces:**
- Consumes: everything in `GenesisCage` (Task 1).
- Produces:
  - `Monster` Genesis fields: `int cageStage = 0;` (0 command / 1 deploy / 2 lock; the stage clock is `spawnTimer`), `float cageCooldown = GenesisCage::COOLDOWN;`, `float cageCX = 0, cageCY = 0, cageA0 = 0;`, `int cageCount = 0;`
  - `Monster` vertex fields: `int cageSlot = -1; float cageFromX = 0, cageFromY = 0;`, plus `thrownPhase == 3` meaning "held as a cage vertex".
  - `hivePhase == 4` meaning "cage".
  - `MonsterManager::UpdateGenesisLaunchers(float dt, float playerCX, float playerCY, float& playerHP)`. `UpdateAll` passes its `playerHP`.
  - `template <class F> void ForEachGenesisCageEdge(const Monster* g, F f) const`: calls `f(ax, ay, bx, by)` for each slot pair k ↔ (k+1) % cageCount where both vertices are alive with `thrownPhase == 3` and owner `g->genesisId`. For `cageCount == 2` there is one edge, not two.
  - `const Monster* GenesisCageNode(const Monster* g, int slot) const`: the alive vertex in that slot, or nullptr.

Behavior (spec sections 1–8):
- `cageCooldown` counts down every frame in every phase except 4.
- When regroup (phase 3) ends: if `cageCooldown <= 0` and at least MIN_NODES of the Genesis' own Swarms are `alive && genesisThrown && thrownPhase == 0` within RECRUIT_RANGE of the player, call `BeginGenesisCage`. Otherwise go to phase 0 as now.
- `BeginGenesisCage`: take the nearest MAX_NODES; center = player position now; sort them by `atan2` around the center; slot k = k-th; `cageA0` = angle of slot 0; set `thrownPhase = 3`, `cageFromX/Y` = current position; `hivePhase = 4`, `cageStage = 0`, `spawnTimer = 0`.
- Stage 0, for COMMAND_TIME: vertices hold still. `genesisShellSpin` makes one eased full turn (same smoothstep form as phase 0).
- Stage 1, for DEPLOY_TIME: vertex position = lerp(from, slot point at RADIUS, `DeployBlend(t)`).
- Stage 2, for LOCK_TIME: vertex position = slot point at `LockRadius(t)`. If any edge has `SegmentDistSq(player, edge) < EDGE_HIT_RADIUS²`, call `HurtPlayer(playerHP, EDGE_DAMAGE)` once and end the cage.
- In any stage: fewer than 2 alive vertices → end.
- `EndGenesisCage`: own vertices go to `thrownPhase = 0, cageSlot = -1`; `hivePhase = 0, spawnTimer = 0, hiveSpawnCount = 0, cageCooldown = COOLDOWN`.
- At the start of `UpdateGenesisLaunchers`, any `thrownPhase == 3` Swarm whose owner is not an alive Genesis in phase 4 is released the same way.
- `Monster::UpdateThrownSwarm` returns false at once when `thrownPhase == 3` (no timer, no stun release, no trail).
- `ResolveMonsterCrowding` skips `thrownPhase == 3` in the same places it skips `thrownPhase == 1`.

Test fixture: a helper `Monster* AddOwnSwarm(MonsterManager&, const Monster* g, float x, float y)` builds `new Monster(x, y, 1.0f)`, `MakeKind(SWARM)`, `genesisThrown = true`, `thrownOwnerId = g->genesisId`, `thrownPhase = 0`. A helper `ArmCage(Monster* g)` sets `hivePhase = 3`, `spawnTimer = Monster::GENESIS_RECOVER_TIME`, `cageCooldown = 0`. Genesis at (0,0), player at (400,0) unless stated, `DT = 1/120`, the `Step` helper copied from `genesis_launcher_test.cpp`.

- [ ] **Step 1: Write the failing tests** (each prints one `... ok` line and ends with `mm.Clear()`):
  - `CheckTriggerNeedsThreeNodes`: 2 own Swarms near the player → after `ArmCage` and one `DT` step, `hivePhase == 0`. With 3 → `hivePhase == 4` and all 3 have `thrownPhase == 3`.
  - `CheckCooldownBlocksTrigger`: 3 Swarms, `ArmCage`, then `cageCooldown = 5` → `hivePhase == 0`. A freshly made Genesis has `cageCooldown == GenesisCage::COOLDOWN`.
  - `CheckRecruitRangeAndCap`: 8 Swarms within 200 px of the player, 1 at 900 px, 1 with `thrownPhase = 1` → `cageCount == 6`. The far one and the in-flight one are not `thrownPhase == 3`.
  - `CheckDeployReachesRing`: 4 Swarms, trigger, then move the player to (700,0) and step COMMAND_TIME + DEPLOY_TIME + DT → `cageCX/Y == (400,0)`, and every vertex is within 1.5 px of distance 240 from it.
  - `CheckLockHitsOnceAndEnds`: player stays at the center, `hp = 100` → after stepping 3.0 s, `hp == 88` exactly, `hivePhase == 0`, `cageCooldown` within one frame of 12, no Swarm has `thrownPhase == 3`.
  - `CheckEscapeOutsideNoDamage`: during deploy, move the player to (1000,0). Then step 3.0 s → `hp == 100`. The cage ended once the lock finished.
  - `CheckKilledNodeOpensGap`: 3 Swarms. During deploy, set `alive = false` on `GenesisCageNode(g, 1)`. In the lock stage the edge count from `ForEachGenesisCageEdge` == 1 (it was 3 before the kill). Kill slot 2 as well → next step `hivePhase == 0`.
  - `CheckGenesisDeathReleasesNodes`: trigger, then `g->alive = false` during deploy → after one step, no Swarm has `thrownPhase == 3`.
  - `CheckTwoGenesisKeepTheirOwnNodes`: two Genesis, 3 Swarms each, both armed → each `cageCount == 3`. Every edge reported for Genesis A joins A's vertices only.
- [ ] **Step 2: Run** `powershell -File scripts/test_genesis_cage.ps1`. Expected: build fails on missing `cageCooldown` / `ForEachGenesisCageEdge`.
- [ ] **Step 3: Add the fields and the `thrownPhase == 3` early return to `Monster.h`.** Initialize `cageCooldown = GenesisCage::COOLDOWN` in the GENESIS branch of `MakeKind` (next to `hivePhase = 0`).
- [ ] **Step 4: Implement the phase 4 behavior, `BeginGenesisCage(Monster*, float px, float py)`, `EndGenesisCage(Monster*)`, the orphan release, `ForEachGenesisCageEdge` and `GenesisCageNode` in `MonsterManager.h`; change the `UpdateGenesisLaunchers` signature; update the crowding skips.** Extend the comment above `UpdateGenesisLaunchers` with one sentence on phase 4.
- [ ] **Step 5: Run** `powershell -File scripts/test_genesis_cage.ps1`. Expected: every `ok` line, then `genesis cage regression passed`.
- [ ] **Step 6: Run** `powershell -File scripts/test_genesis_launcher.ps1` and `powershell -File scripts/test_thrown_swarm_kill.ps1`. Expected: both pass. If a launcher test fails only because a cage started, apply Review Focus 5.
- [ ] **Step 7: Commit**

```bash
git add tests/genesis_cage_test.cpp WiNILL/Entity/MonsterManager.h WiNILL/Entity/Monster.h
git commit -m "feat(genesis): cage live thrown Swarms into a shrinking polygon"
```
(`Monster.h` has Gravis WIP. Apply the precondition.)

---

### Task 3: Draw, codex text, game wiring

**Files:**
- Modify: `WiNILL/Render/EntityDraw.cpp`, `WiNILL/Render/EntityDraw.h`
- Modify: `WiNILL/main.cpp` (next to `DrawRegulusBeams(g_MonsterManager, g_GameTime);`, ~line 3372)
- Modify: `WiNILL/Core/Codex.h` (GENESIS entry, lines 283–287)

**Interfaces:**
- Consumes: `hivePhase == 4`, `cageStage`, `spawnTimer`, `ForEachGenesisCageEdge`, `GenesisCageNode`, `thrownPhase == 3` (Task 2).
- Produces: `void DrawGenesisCages(const MonsterManager& manager, float visualTime);` declared in `EntityDraw.h`.

- [ ] **Step 1: Implement `DrawGenesisCages`** next to `DrawRegulusBeams`, for each alive GENESIS with `hivePhase == 4`. Color is production red (1.0, 0.24, 0.20).
  - Stage 0: a line from the core to each vertex, width 1.0, alpha 0.55 · (1 − t / COMMAND_TIME).
  - Stage 1: each edge, width 1.0, alpha 0.35.
  - Stage 2: each edge as a 7.0 px line at alpha 0.18 under a 2.6 px line at alpha 0.95.
  - Use only `drawLineQuad`; no circles or fills.
- [ ] **Step 2: Fix the Swarm draw** in `drawMob` (~line 2130): the ghost / spin / landing-shake block runs only for `thrownPhase == 1 || thrownPhase == 2`, so a phase 3 vertex draws like an ordinary Swarm.
- [ ] **Step 3: Genesis body in phase 4.** In `DrawGenesisLauncher`, treat stage 0 like a fully heated core (`gapHeat = 1`) so the core flares while the shells spin. The shells draw empty because `GenesisStackFill()` returns 0 for phase 4.
- [ ] **Step 4: Call `DrawGenesisCages(g_MonsterManager, g_GameTime);`** right after `DrawRegulusBeams` in `main.cpp`.
- [ ] **Step 5: Update the GENESIS codex entry.**
  - Threat label `L"HIGH"` → `L"ELITE"`.
  - Summary prefix `[엘리트] ` / `[ELITE] ` / `[エリート] `.
  - Damage string: append `, 12 / cage edge`.
  - Ability line, appended with ` · ` to each language:
    - ko `포위 진형: 투척 스웜 3마리 이상이면 플레이어 둘레에 다각형을 짜고 좁혀옴 (꼭짓점 처치 시 틈)`
    - en `Cage: with 3+ thrown Swarms alive, forms a closing polygon around the player (kill a vertex to open a gap)`
    - ja `包囲陣形：投擲SWARMが3体以上いると、プレイヤーの周囲に多角形を組んで狭める（頂点を倒すと隙間）`
- [ ] **Step 6: Build** with `powershell -File scripts/build_windows.ps1`. Expected: exit 0, no new warnings in the touched files.
- [ ] **Step 7: Manual check in a Debug run** (spawn a Genesis with the debug toolkit). Confirm each of these:
  - (a) Command lines flash, then dim edges, then bright edges shrink.
  - (b) Shooting a vertex removes its two edges.
  - (c) Stepping out during deploy takes no damage.
  - (d) Standing still takes one 12 hit and the cage ends.
  - (e) Vertices do not shake or show landing rings.
  - (f) Dashing through an edge and through a Swarm feel the same (Review Focus 1).

  Report each one as pass or fail.
- [ ] **Step 8: Commit**

```bash
git add WiNILL/Render/EntityDraw.cpp WiNILL/Render/EntityDraw.h WiNILL/main.cpp WiNILL/Core/Codex.h
git commit -m "feat(genesis): draw the cage and document it in the codex"
```
(Gravis WIP is in all four files. Apply the precondition.)
