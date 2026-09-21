# Blur regression checks

The window compositor blurs the desktop behind every scene when the backdrop
setting is ON. The OpenGL blur only processes game pixels behind modal UI and
SHOP; it cannot sample the desktop. Keep gameplay entities and HUD sharp.

Windows uses Acrylic first, then legacy accent blur if the request is rejected.
Do not treat DwmEnableBlurBehindWindow as successful blur on Windows 8+:
https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmenableblurbehindwindow
Do not disable that API as part of the setting: GLFW uses it for transparency.

Run `powershell -ExecutionPolicy Bypass -File scripts/test_blur_windows.ps1`.
The hidden OpenGL 3.3 test checks actual output pixels, an unrelated read FBO,
128/256-pixel resizing, 4x MSAA resolve, return to the default framebuffer,
restoration of framebuffer/culling/color-mask state, and GL errors.

Verified locally on AMD Radeon 780M Graphics. Release build passed. Desktop
screenshots confirmed blurred backgrounds with sharp menu UI and active gameplay
entities/HUD. Intel and NVIDIA hardware have not been tested in this session.

Manual checks on each target machine:
- ON/OFF in settings; return to main menu and active gameplay.
- SHOP, pause, augment/debuff/replacement selection, boss intermission, game over.
- Resize/display change, repeated menu/game transitions, and settings from pause.
- Confirm no old scene flashes, sharp popup text, and no GL/capture failures.

Windows desktop effects remain subject to compositor support and system
transparency settings. macOS/Linux desktop blur is not implemented by WindowFx;
the OpenGL modal blur is independent of this Windows-only compositor path.
