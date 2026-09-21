#!/usr/bin/env bash
# macOS 빌드 → bin/Onedow/macOS/Onedow
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ONEDOW="$ROOT/bin/Onedow"
MAC_OUT="$ONEDOW/macOS"

"$ROOT/scripts/build_macos.sh"

mkdir -p "$ONEDOW/WindowsOS" "$MAC_OUT"
rm -rf "$ONEDOW/Resource"
cp -R "$ROOT/WiNILL/Resource" "$ONEDOW/Resource"
rm -f "$MAC_OUT/Onedow" "$MAC_OUT/README.txt"
cp "$ROOT/build/WiNILL" "$MAC_OUT/Onedow"
chmod +x "$MAC_OUT/Onedow"

echo "macOS -> $MAC_OUT/Onedow"
