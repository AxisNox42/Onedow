// Run scripts/test_skill_hud_model.ps1. Skill HUD view model: game state →
// what each slot and passive shows (cooldown wedge, effect frame, pulse).
#include "SkillHudModel.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cwchar>

using namespace skill_hud;

static SkillHudInputs Base() {
    SkillHudInputs in{};
    in.hyperFocusDur = 5.0f;
    in.timeStopDur = 1.5f;
    in.rainCooldown = 15.0f;
    in.dashCdMax = 2.9f;
    return in;
}
static bool Near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void empty_slots() {
    auto v = BuildSkillHudView(Base(), SkillHudPulse{});
    assert(!v.slots[0].empty && v.slots[1].empty && v.slots[2].empty && v.slots[3].empty);
    assert(v.slots[1].color == HudColor::Empty);
    assert(v.passiveCount == 0);
}
static void cooldown_frac() {
    auto in = Base();
    in.equipped[0] = true; in.glyph[0] = SkillGlyph::Burst;
    in.cd[0] = 6.0f; in.cdMax[0] = 16.0f;
    auto v = BuildSkillHudView(in, SkillHudPulse{});
    assert(Near(v.slots[1].elapsedFrac, 0.625f));
    assert(v.slots[1].color == HudColor::Close);
    assert(std::wcscmp(v.slots[1].key, L"Q") == 0);
    assert(Near(v.slots[1].effectFrac, 0.0f));
}
static void frac_clamped() {
    auto in = Base();
    in.equipped[0] = true; in.glyph[0] = SkillGlyph::Burst;
    in.cd[0] = 5.0f; in.cdMax[0] = 0.0f;
    in.dashCd = 9.0f; in.dashCdMax = 2.9f;
    auto v = BuildSkillHudView(in, SkillHudPulse{});
    assert(v.slots[1].elapsedFrac >= 0.0f && v.slots[1].elapsedFrac <= 1.0f);
    assert(Near(v.slots[0].elapsedFrac, 0.0f));
}
static void effect_frac() {
    auto in = Base();
    in.equipped[1] = true; in.glyph[1] = SkillGlyph::Focus;
    in.hyperFocusTimer = 2.5f;
    in.equipped[0] = true; in.glyph[0] = SkillGlyph::Burst;
    auto v = BuildSkillHudView(in, SkillHudPulse{});
    assert(Near(v.slots[2].effectFrac, 0.5f));
    assert(Near(v.slots[1].effectFrac, 0.0f));
}
// Review #1: FOCUS cooling down while its effect runs shows two-digit
// seconds; the icon must dim so the number reads (the outer frame already
// says the effect is on).
static void seconds_dim_icon_during_effect() {
    auto in = Base();
    in.equipped[1] = true; in.glyph[1] = SkillGlyph::Focus;
    in.cd[1] = 11.0f; in.cdMax[1] = 20.0f; in.hyperFocusTimer = 2.0f;
    in.equipped[2] = true; in.glyph[2] = SkillGlyph::TimeStop;
    auto v = BuildSkillHudView(in, SkillHudPulse{});
    assert(v.slots[2].showSeconds && !v.slots[2].iconBright);
    assert(v.slots[2].effectFrac > 0.0f);
    assert(!v.slots[3].showSeconds && v.slots[3].iconBright);
    assert(!v.slots[1].showSeconds && !v.slots[1].iconBright);
}
static void flash_glyph() {
    auto in = Base();
    in.dashUpgrade = true;
    assert(BuildSkillHudView(in, SkillHudPulse{}).slots[0].glyph == SkillGlyph::Flash);
}
// The HUD shows cooldowns only: RAIN stays, LSTP (on/off) and DEATH (a
// debuff) live in the owned-augment list instead.
static void rain_is_the_only_passive() {
    auto in = Base();
    in.rainOwned = true; in.rainTimer = 9.0f;
    auto v = BuildSkillHudView(in, SkillHudPulse{});
    assert(v.passiveCount == 1);
    assert(v.passives[0].glyph == PassiveGlyph::Rain);
    assert(v.passives[0].color == HudColor::Rain);
    assert(Near(v.passives[0].chargeFrac, 0.6f));
}
static void pulse_on_ready_and_pause() {
    auto in = Base();
    in.equipped[2] = true; in.glyph[2] = SkillGlyph::TimeStop;
    in.cd[2] = 0.1f;
    SkillHudPulse p{}; p.Reset();
    p.Update(in, 0.016f);
    in.cd[2] = 0.0f;
    p.Update(in, 0.016f);
    assert(Near(p.slot[3], kPulseDur));
    p.Update(in, 0.0f);
    assert(Near(p.slot[3], kPulseDur));
    p.Update(in, 0.2f);
    assert(Near(p.slot[3], kPulseDur - 0.2f));
    assert(Near(BuildSkillHudView(in, p).slots[3].pulse, kPulseDur - 0.2f));
}
static void pulse_not_on_swap() {
    auto in = Base();
    in.equipped[0] = true; in.glyph[0] = SkillGlyph::Burst; in.cd[0] = 8.0f;
    SkillHudPulse p{}; p.Reset();
    p.Update(in, 0.016f);
    in.glyph[0] = SkillGlyph::Focus; in.cd[0] = 0.0f;
    p.Update(in, 0.016f);
    assert(Near(p.slot[1], 0.0f));
}
static void pulse_rain_fire() {
    auto in = Base();
    in.rainOwned = true; in.rainTimer = 14.9f;
    SkillHudPulse p{}; p.Reset();
    p.Update(in, 0.016f);
    in.rainTimer = 0.0f;
    p.Update(in, 0.016f);
    assert(Near(p.rain, kPulseDur));
}
static void pulse_reset_no_spurious() {
    auto in = Base();
    in.rainOwned = true; in.rainTimer = 12.0f;
    in.equipped[0] = true; in.glyph[0] = SkillGlyph::Burst; in.cd[0] = 3.0f;
    SkillHudPulse p{}; p.Reset();
    p.Update(in, 0.016f);
    p.Reset();
    in.rainTimer = 0.0f; in.cd[0] = 0.0f;
    p.Update(in, 0.016f);
    assert(Near(p.rain, 0.0f) && Near(p.slot[1], 0.0f));
}

int main() {
    empty_slots();
    cooldown_frac();
    frac_clamped();
    effect_frac();
    seconds_dim_icon_during_effect();
    flash_glyph();
    rain_is_the_only_passive();
    pulse_on_ready_and_pause();
    pulse_not_on_swap();
    pulse_rain_fire();
    pulse_reset_no_spurious();
    std::puts("skill_hud_model: ok");
    return 0;
}
