#!/usr/bin/env bash
# macOS 빌드 → bin/Onedow/macOS/Onedow (에셋은 Onedow 루트 공용)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ONEDOW="$ROOT/bin/Onedow"
MAC_OUT="$ONEDOW/macOS"

"$ROOT/scripts/build_macos.sh"

if [[ ! -f "$ROOT/WiNILL/Font/Jua-Regular.ttf" ]]; then
  echo "오류: WiNILL/Font/ 폰트 필요"
  exit 1
fi

mkdir -p "$ONEDOW"
rm -rf "$ONEDOW/Resource" "$ONEDOW/Font" "$ONEDOW/Icons"
cp -R "$ROOT/WiNILL/Resource" "$ONEDOW/Resource"
cp -R "$ROOT/WiNILL/Font" "$ONEDOW/Font"
[[ -d "$ROOT/WiNILL/Icons" ]] && cp -R "$ROOT/WiNILL/Icons" "$ONEDOW/Icons"

mkdir -p "$MAC_OUT"
cp "$ROOT/build/WiNILL" "$MAC_OUT/Onedow"
chmod +x "$MAC_OUT/Onedow"

echo "macOS -> $MAC_OUT/Onedow"
