# macOS 빌드 (WiNILL / Onedow)

Windows는 `WiNILL/WiNILL.vcxproj` 로 빌드합니다.  
macOS는 **CMake + GLFW** 로 같은 소스를 빌드합니다.

## 요구 사항

- macOS 11+
- Xcode Command Line Tools (`xcode-select --install`)
- [Homebrew](https://brew.sh)
- `cmake`, `glfw` (`brew install cmake glfw`)

## 빌드

```bash
chmod +x scripts/build_macos.sh
./scripts/build_macos.sh
cd build && ./WiNILL
```

수동 빌드:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ./WiNILL
```

## 폰트 (필수)

`WiNILL/Font/` 에 아래 **2개**만 있으면 됩니다.

- `Jua-Regular.ttf` — 한글
- `KosugiMaru-Regular.ttf` — 일본어·라틴 폴백

빌드 시 `build/Font/` 로 자동 복사됩니다.

패키징 (권장):

```bash
./scripts/package_macos.sh
# → bin/Onedow/macOS/Onedow
```

## Windows와의 차이

| 항목 | Windows | macOS |
|------|---------|-------|
| 투명 오버레이 | DWM 합성 | `GLFW_TRANSPARENT_FRAMEBUFFER` |
| 폰트 | EXE 임베드 | `Resource/Font/*.ttf` |
| 아이콘 | EXE 임베드 우선 | `Icons/*.png` |
| 작업 표시줄 | Win32 work area | `glfwGetMonitorWorkarea` (Dock) |

게임 플레이·가짜 창 렌더링은 동일한 OpenGL 경로를 사용합니다.

## 문제 해결

- **창이 안 뜸 / 검은 화면**: 터미널에서 `./WiNILL` 실행 후 셰이더 에러 로그 확인
- **글자 안 보임**: `Resource/Font/` 폰트 3종 확인
- **소리 없음**: `build/Resource/Audio/` 가 실행 파일 옆에 복사됐는지 확인 (CMake POST_BUILD)
