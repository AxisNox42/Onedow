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

VERSION="$(sed -n 's/.*ONEDOW_VERSION\[\] = L"\([0-9.]*\)";.*/\1/p' "$ROOT/WiNILL/Core/GameVersion.h")"
test -n "$VERSION"
sed "s/^Version: .*/Version: $VERSION/" "$ONEDOW/README.md" > "$ONEDOW/README.md.tmp"
mv "$ONEDOW/README.md.tmp" "$ONEDOW/README.md"

echo "macOS -> $MAC_OUT/Onedow"
