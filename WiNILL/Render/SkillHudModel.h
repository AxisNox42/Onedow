#pragma once
// ─────────────────────────────────────────────────────────────
// Skill HUD view model: plain game values → what each key and passive
// shows (cooldown wedge, effect frame, ready pulse). No GL and no game
// headers, so tests/skill_hud_model_test.cpp can build it alone.
//   slot 0 = SHIFT (dash), slots 1..3 = Q/E/R (inputs index 0..2)
//   passives: cooldown-type only (RAIN). On/off passives and debuffs
//   are listed in the owned-augment panel, not here.
// ─────────────────────────────────────────────────────────────
#include <algorithm>
#include <cmath>

namespace skill_hud {

enum class SkillGlyph { Dash, Flash, Burst, Focus, TimeStop };
enum class PassiveGlyph { Rain };
enum class HudColor { Dash, Time, Close, Focus, Rain, Empty };

inline constexpr float kPulseDur = 0.45f;

struct SkillHudInputs {
    bool       equipped[3];
    SkillGlyph glyph[3];
    float      cd[3];
    float      cdMax[3];
    float dashCd, dashCdMax;
    bool  dashActive;
    float dashProgress;
    bool  dashUpgrade;
    float hyperFocusTimer, hyperFocusDur;
    float timeStopTimer, timeStopDur;
    bool  rainOwned;
    float rainTimer, rainCooldown;
};

struct SkillSlotView {
    bool empty;
    SkillGlyph glyph;
    HudColor color;
    float cdRemain, cdMax;
    float elapsedFrac;   // 1 = ready
    float effectFrac;    // remaining effect time, 0 = none
    float pulse;         // remaining pulse time, 0 .. kPulseDur
    const wchar_t* key;
    bool showSeconds;    // cooldown seconds drawn over the icon
    bool iconBright;     // full-strength icon; dimmed whenever seconds show
};

struct PassiveView {
    PassiveGlyph glyph;
    HudColor color;
    float chargeFrac;
    float pulse;
};

struct SkillHudView {
    SkillSlotView slots[4];
    PassiveView passives[1];
    int passiveCount;
};

inline float Frac01(float num, float den) {
    if (!(den > 0.0f)) return 0.0f;
    return std::clamp(num / den, 0.0f, 1.0f);
}

inline float ElapsedFrac(float cd, float cdMax) {
    if (cd <= 0.0f || !(cdMax > 0.0f)) return 1.0f;
    return std::clamp(1.0f - cd / cdMax, 0.0f, 1.0f);
}

// Ready pulses: a key whose cooldown crosses zero while it keeps the same
// skill, and the rain timer rewinding (a volley fired). Reset() at a run
// start; the first Update after it only records the previous values.
struct SkillHudPulse {
    float slot[4];
    float rain;
    float prevCd[4];
    bool prevEquipped[4];
    SkillGlyph prevGlyph[4];
    float prevRain;
    bool primed;

    void Reset() { *this = SkillHudPulse{}; }

    void Update(const SkillHudInputs& in, float delta) {
        if (delta <= 0.0f) return;
        for (float& p : slot) p = std::max(0.0f, p - delta);
        rain = std::max(0.0f, rain - delta);

        float cd[4];
        bool equipped[4];
        SkillGlyph glyph[4];
        cd[0] = in.dashCd;
        equipped[0] = true;
        glyph[0] = in.dashUpgrade ? SkillGlyph::Flash : SkillGlyph::Dash;
        for (int i = 0; i < 3; ++i) {
            cd[i + 1] = in.cd[i];
            equipped[i + 1] = in.equipped[i];
            glyph[i + 1] = in.glyph[i];
        }
        if (primed) {
            for (int i = 0; i < 4; ++i) {
                const bool same = equipped[i] && prevEquipped[i] &&
                    (i == 0 || glyph[i] == prevGlyph[i]);
                if (same && prevCd[i] > 0.0f && cd[i] <= 0.0f) slot[i] = kPulseDur;
            }
            if (in.rainOwned && prevRain > in.rainTimer + 0.5f) rain = kPulseDur;
        }
        for (int i = 0; i < 4; ++i) {
            prevCd[i] = cd[i];
            prevEquipped[i] = equipped[i];
            prevGlyph[i] = glyph[i];
        }
        prevRain = in.rainTimer;
        primed = true;
    }
};

inline HudColor ColorFor(SkillGlyph g) {
    switch (g) {
    case SkillGlyph::Burst:    return HudColor::Close;
    case SkillGlyph::Focus:    return HudColor::Focus;
    case SkillGlyph::TimeStop: return HudColor::Time;
    default:                   return HudColor::Dash;
    }
}

inline SkillHudView BuildSkillHudView(const SkillHudInputs& in, const SkillHudPulse& pulse) {
    static const wchar_t* kKeys[4] = { L"SHIFT", L"Q", L"E", L"R" };
    SkillHudView v{};

    SkillSlotView& dash = v.slots[0];
    dash.glyph = in.dashUpgrade ? SkillGlyph::Flash : SkillGlyph::Dash;
    dash.color = HudColor::Dash;
    dash.cdRemain = std::max(0.0f, in.dashCd);
    dash.cdMax = in.dashCdMax;
    dash.elapsedFrac = ElapsedFrac(in.dashCd, in.dashCdMax);
    dash.effectFrac = in.dashActive ? std::clamp(1.0f - in.dashProgress, 0.0f, 1.0f) : 0.0f;
    dash.pulse = pulse.slot[0];
    dash.key = kKeys[0];

    for (int i = 0; i < 3; ++i) {
        SkillSlotView& s = v.slots[i + 1];
        s.key = kKeys[i + 1];
        if (!in.equipped[i]) {
            s.empty = true;
            s.color = HudColor::Empty;
            s.elapsedFrac = 1.0f;
            continue;
        }
        s.glyph = in.glyph[i];
        s.color = ColorFor(s.glyph);
        s.cdRemain = std::max(0.0f, in.cd[i]);
        s.cdMax = in.cdMax[i];
        s.elapsedFrac = ElapsedFrac(in.cd[i], in.cdMax[i]);
        if (s.glyph == SkillGlyph::Focus)
            s.effectFrac = Frac01(in.hyperFocusTimer, in.hyperFocusDur);
        else if (s.glyph == SkillGlyph::TimeStop)
            s.effectFrac = Frac01(in.timeStopTimer, in.timeStopDur);
        s.pulse = pulse.slot[i + 1];
    }
    // Seconds sit on the icon, so the icon dims under them even while an
    // effect runs (its outer frame already shows the effect).
    for (SkillSlotView& s : v.slots) {
        s.showSeconds = !s.empty && s.elapsedFrac < 1.0f && s.cdRemain > 0.0f;
        s.iconBright = !s.empty && !s.showSeconds;
    }

    if (in.rainOwned) {
        PassiveView& p = v.passives[v.passiveCount++];
        p.glyph = PassiveGlyph::Rain;
        p.color = HudColor::Rain;
        p.chargeFrac = Frac01(in.rainTimer, in.rainCooldown);
        p.pulse = pulse.rain;
    }
    return v;
}

}  // namespace skill_hud
