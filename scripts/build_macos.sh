#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew 가 필요합니다: https://brew.sh"
  exit 1
fi
if ! brew list cmake >/dev/null 2>&1; then
  echo "cmake 설치 중..."
  brew install cmake
fi
if ! brew list glfw >/dev/null 2>&1; then
  echo "glfw 설치 중..."
  brew install glfw
fi

# 폰트 — WiNILL/Font/ (Jua + KosugiMaru)
if [[ ! -f "$ROOT/WiNILL/Font/Jua-Regular.ttf" || ! -f "$ROOT/WiNILL/Font/KosugiMaru-Regular.ttf" ]]; then
  echo "오류: WiNILL/Font/ 에 Jua-Regular.ttf, KosugiMaru-Regular.ttf 가 필요합니다."
  exit 1
fi

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo ""
echo "빌드 완료. 실행:"
echo "  cd build && ./WiNILL"
