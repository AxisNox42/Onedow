#pragma once
#include "Augment.h"

enum class SkillType { NONE, CLOSE_WINDOW, OVERCLOCK, TIME_STOP };

struct SkillSlot {
    SkillType type = SkillType::NONE;
    float     cd   = 0.0f;
};

extern SkillSlot g_Skills[3];
extern int       g_SkillReplaceIdx;

SkillType SkillForAug(AugType a);
void      EquipSkill(SkillType t);
