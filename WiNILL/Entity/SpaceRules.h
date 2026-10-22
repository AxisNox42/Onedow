#pragma once

// ─────────────────────────────────────────────────────────────
// Window spaces for the NORMA boss: every open pane is a separate space.
//   Movement and aggro cross pane borders; visible attacks and effects do not.
//   Space 0 is the open field and a pane's space id is its pane id. Regions
//   are world rectangles, front-most first, so overlapping panes give the
//   overlap to the one in front. With no regions every check passes and the
//   rest of the game is unaffected.
// ─────────────────────────────────────────────────────────────
#include <algorithm>
#include <vector>

namespace space {

struct Region {
    int id = 0;
    float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
};

inline std::vector<Region> g_Regions;
inline int g_PlayerSpace = 0;

constexpr int kPlayerSource = -2;    // "whatever space the player is in"
constexpr int kUntracked = -1;       // source not tracked: always allowed

inline bool Active() { return !g_Regions.empty(); }

// Space of an entity whose centre is at (x, y).
inline int SpaceAt(float x, float y) {
    for (const Region& r : g_Regions)
        if (x >= r.x0 && x < r.x1 && y >= r.y0 && y < r.y1) return r.id;
    return 0;
}

inline int Resolve(int source) {
    return source == kPlayerSource ? g_PlayerSpace : source;
}

// Can an attack or effect from `source` touch something centred at (x, y)?
inline bool Reaches(int source, float x, float y) {
    if (!Active() || source == kUntracked) return true;
    return Resolve(source) == SpaceAt(x, y);
}

inline bool SameSpace(float ax, float ay, float bx, float by) {
    return !Active() || SpaceAt(ax, ay) == SpaceAt(bx, by);
}

// Enemy damage reaches the player through HurtPlayer, which has no attacker
// argument; the monster update loop names the attacker's space here.
inline int g_HurtSource = kUntracked;

struct HurtSourceScope {
    int previous;
    explicit HurtSourceScope(int source) : previous(g_HurtSource) { g_HurtSource = source; }
    ~HurtSourceScope() { g_HurtSource = previous; }
    HurtSourceScope(const HurtSourceScope&) = delete;
    HurtSourceScope& operator=(const HurtSourceScope&) = delete;
};

inline bool PlayerExposed() {
    return !Active() || g_HurtSource == kUntracked || g_HurtSource == g_PlayerSpace;
}

// A shot stopped by a pane border this frame. The pane it hit loses
// durability; the open field (0) never absorbs.
struct Absorb {
    int paneId = 0;
    float x = 0.0f, y = 0.0f;
};
inline std::vector<Absorb> g_Absorbed;

// Crossing from one space to another is stopped by the pane being entered,
// or by the pane being left when stepping out into the open field.
inline int AbsorbingPane(int from, int to) { return to != 0 ? to : from; }

// A shot keeps the space it was fired in. True when it now stands in another
// space (it crossed a border, or a pane was dragged over it or closed), with
// the pane that stops it.
inline bool CrossedBorder(int shotSpace, float x, float y, int& absorbingPane) {
    const int now = SpaceAt(x, y);
    if (now == shotSpace) return false;
    absorbingPane = AbsorbingPane(shotSpace, now);
    return true;
}

// Pane durability: hits a pane can stop before it self-destructs. Smaller
// panes are sturdier, up to three times the base.
constexpr float kBaseDurability = 60.0f;
constexpr float kReferenceArea = 520.0f * 360.0f;

inline float MaxDurability(float area) {
    if (area <= 1.0f) return kBaseDurability * 3.0f;
    return kBaseDurability * std::clamp(kReferenceArea / area, 0.5f, 3.0f);
}

} // namespace space
