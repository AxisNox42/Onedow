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

# 폰트 (Windows 빌드와 동일 파일명) — 없으면 텍스트 일부만 안 보일 수 있음
FONT_DIR="$ROOT/WiNILL/Resource/Font"
mkdir -p "$FONT_DIR"
for f in Jua-Regular.ttf KosugiMaru-Regular.ttf Oswald-VariableFont_wght.ttf; do
  if [[ ! -f "$FONT_DIR/$f" && -f "$ROOT/WiNILL/Font/$f" ]]; then
    cp "$ROOT/WiNILL/Font/$f" "$FONT_DIR/"
  fi
done

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo ""
echo "빌드 완료. 실행:"
echo "  cd build && ./WiNILL"
