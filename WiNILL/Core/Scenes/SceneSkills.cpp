#include "SceneSkills.h"

SkillSlot g_Skills[3];
int       g_SkillReplaceIdx = 0;

SkillType SkillForAug(AugType a) {
    if (a == AugType::SKILL_CLOSE)     return SkillType::CLOSE_WINDOW;
    if (a == AugType::SKILL_OVERCLOCK) return SkillType::HYPER_FOCUS;
    if (a == AugType::SKILL_TIMESTOP)  return SkillType::TIME_STOP;
    if (a == AugType::SKILL_FOCUS)     return SkillType::FOCUS_AIM;
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

void ClearEquippedSkills() {
    for (int i = 0; i < 3; i++) g_Skills[i] = { SkillType::NONE, 0.0f };
    g_SkillReplaceIdx = 0;
}

void ReequipSkillsFromOwned(const int* indices, int count) {
    ClearEquippedSkills();
    if (!indices || count <= 0) return;
    for (int k = 0; k < count; k++) {
        int idx = indices[k];
        if (idx < 0 || idx >= AUG_TOTAL) continue;
        EquipSkill(SkillForAug(ALL_AUGS[idx].type));
    }
}
