#pragma once

void DrawPlayerWeaponShell(float cx, float cy, float size, float aimAngle, float visualTime);
void DrawInGameSignalFrame(float x, float y, float width, float height,
                           float r, float g, float b, float alpha);
// Lobby preview of a player shell. kind: 0 = rifle, 1 = static field.
void DrawPlayerShellPreview(int kind, float cx, float cy, float size, float time,
                            float r, float g, float b, float alpha);
