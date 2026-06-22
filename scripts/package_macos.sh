#!/usr/bin/env bash
# macOS 빌드 후 bin/Onedow/macOS/ 로 패키징
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/bin/Onedow/macOS"

"$ROOT/scripts/build_macos.sh"

if [[ ! -f "$ROOT/build/WiNILL" ]]; then
  echo "오류: build/WiNILL 없음"
  exit 1
fi

echo "==> Packaging -> $OUT"
rm -rf "$OUT"
mkdir -p "$OUT"

cp "$ROOT/build/WiNILL" "$OUT/Onedow"
chmod +x "$OUT/Onedow"
cp -R "$ROOT/WiNILL/Resource" "$OUT/Resource"
cp -R "$ROOT/WiNILL/Font" "$OUT/Font"
if [[ -d "$ROOT/WiNILL/Icons" ]]; then
  cp -R "$ROOT/WiNILL/Icons" "$OUT/Icons"
fi

echo ""
echo "완료: $OUT/Onedow"
