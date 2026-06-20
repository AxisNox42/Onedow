#include "SceneSkills.h"

SkillSlot g_Skills[3];
int       g_SkillReplaceIdx = 0;

SkillType SkillForAug(AugType a) {
    if (a == AugType::SKILL_CLOSE)     return SkillType::CLOSE_WINDOW;
    if (a == AugType::SKILL_OVERCLOCK) return SkillType::OVERCLOCK;
    if (a == AugType::SKILL_TIMESTOP)  return SkillType::TIME_STOP;
    return SkillType::NONE;
}

void EquipSkill(SkillType t) {
    if (t == SkillType::NONE) return;
    for (int i = 0; i < 3; i++) if (g_Skills[i].type == t) return;
    for (int i = 0; i < 3; i++) if (g_Skills[i].type == SkillType::NONE) {
        g_Skills[i] = { t, 0.0f };
        return;
    }
    g_Skills[g_SkillReplaceIdx] = { t, 0.0f };
    g_SkillReplaceIdx = (g_SkillReplaceIdx + 1) % 3;
}
