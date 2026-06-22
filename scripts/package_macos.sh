#!/usr/bin/env bash
# macOS 빌드 → bin/Zip/Onedow/macOS/Onedow 만
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ONEDOW="$ROOT/bin/Zip/Onedow"
MAC_OUT="$ONEDOW/macOS"

"$ROOT/scripts/build_macos.sh"

mkdir -p "$ONEDOW/WindowsOS" "$MAC_OUT"
cp -R "$ROOT/WiNILL/Resource" "$ONEDOW/Resource"
cp -R "$ROOT/WiNILL/Font" "$ONEDOW/Font"
if [ -d "$ROOT/WiNILL/Icons" ]; then cp -R "$ROOT/WiNILL/Icons" "$ONEDOW/Icons"; fi
rm -f "$MAC_OUT/Onedow" "$MAC_OUT/README.txt"
cp "$ROOT/build/WiNILL" "$MAC_OUT/Onedow"
chmod +x "$MAC_OUT/Onedow"

echo "macOS -> $MAC_OUT/Onedow"
