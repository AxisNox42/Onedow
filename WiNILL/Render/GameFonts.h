#pragma once

void InitializeGameFonts(int screenWidth, int screenHeight);

// Per-glyph dark halo that keeps text readable on any wallpaper without a
// backdrop plate. Stronger when the desktop behind the window is not
// blurred. Call once per frame before UI text is drawn.
void UpdateGameTextHalo(int screenHeight, bool backdropBlurEnabled);