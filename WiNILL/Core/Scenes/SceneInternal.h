#pragma once

#include "SceneContext.h"
#include <string>
#include <vector>

inline constexpr float kWideSceneLinearWidth = 0.58f;
inline constexpr float kWideSceneLinearAlpha = 0.68f;

extern float g_MainMenuEntryT;
extern bool s_MainMenuSettingsPanel;
extern bool s_MainMenuRunConfigPanel;
extern bool s_MainMenuResumeFromPanel;
extern float g_SettingsEntryT;
extern bool s_SettingsInlineNeedsReset;

const wchar_t* MainMenuRouteLabel(int language, int index);
std::vector<std::wstring> TarotWrap(const wchar_t* src, float scale,
                                    float maxWidth);
void DrawArchiveConstellation(float cx, float cy, float radius,
                              int category, int key, float now,
                              float r, float g, float b,
                              float alpha, float uiS,
                              bool drawNodeFields = true);
void ResetRunConfigUi(float entryStart = 0.0f);
void ResetSettingsUi(float entryStart = 0.0f);
void Scene_RunConfigInline(const SceneCtx& c);
void Scene_SettingsInline(const SceneCtx& c);
void FinalizeLoadout(const SceneCtx& c, int wIdx);
int FixedWeaponForSelectedJob();

float UiApproach(float value, float target, float dt, float speed);
void DrawMenuBackground(float sw, float sh, float delta,
                        float darkenAmount = 1.0f);
float LogoClamp01(float value);
void SetSceneTextureReveal(float reveal);
void LogoLine(float x1, float y1, float x2, float y2,
              float thick, float r, float g, float b, float alpha);
void DrawPersistentSceneSideVignettes(float sw, float sh, float alpha);
void DrawSceneLeftVignette(float sw, float sh, float alpha);
void DrawSceneRadialVignette(float cx, float cy, float radius, float alpha);
void DrawSettingsOrbitArc(float cx, float cy, float radiusX, float radiusY,
                          float startAngle, float endAngle,
                          float r, float g, float b, float thickness, float alpha);
float MainMenuCommandStartY(float sh, float uiScale = 1.0f);
float MainMenuButtonRailStartY(float sh, float uiScale = 1.0f);
void DrawVisibleConstellLine(float x1, float y1, float x2, float y2,
                             float thick, float r, float g, float b, float alpha);
void DrawConstellationDisc(float x, float y, float radius,
                           float r, float g, float b, float alpha,
                           bool darkenBlend = true);
void DrawVisibleConstellNode(float x, float y, float size,
                             float r, float g, float b, float alpha,
                             bool whiteSpark = true,
                             bool drawField = true);
void DrawMainOnedowLogo(float sw, float sh, float alpha, float reveal,
                        float centerXOverride = -1.0f,
                        float heightMul = 1.0f);
