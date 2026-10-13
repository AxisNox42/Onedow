#pragma once

void InitializeGameFonts(int screenWidth, int screenHeight);

// Releases the GL objects of every game font (g_TextL/S/XL). Call while the
// GL context is still current, before glfwTerminate.
void ShutdownGameFonts();

// Per-glyph dark halo that keeps text readable on any wallpaper without a
// backdrop plate. Stronger when the desktop behind the window is not
// blurred. Call once per frame before UI text is drawn.
void UpdateGameTextHalo(int screenHeight, bool backdropBlurEnabled);