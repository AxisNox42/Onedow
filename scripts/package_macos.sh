#!/usr/bin/env bash
# macOS 빌드 → bin/Asteria/macOS/Asteria
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ASTERIA="$ROOT/bin/Asteria"
MAC_OUT="$ASTERIA/macOS"

bash "$ROOT/scripts/build_macos.sh"

mkdir -p "$ASTERIA/WindowsOS" "$MAC_OUT"
rm -rf "$ASTERIA/Resource"
cp -R "$ROOT/WiNILL/Resource" "$ASTERIA/Resource"
rm -f "$MAC_OUT/Asteria" "$MAC_OUT/README.txt"
cp "$ROOT/build/WiNILL" "$MAC_OUT/Asteria"
chmod +x "$MAC_OUT/Asteria"

VERSION="$(sed -n 's/.*ASTERIA_VERSION\[\] = L"\([0-9.]*\)";.*/\1/p' "$ROOT/WiNILL/Core/GameVersion.h")"
test -n "$VERSION"
sed "s/^Version: .*/Version: $VERSION/" "$ASTERIA/README.md" > "$ASTERIA/README.md.tmp"
mv "$ASTERIA/README.md.tmp" "$ASTERIA/README.md"

echo "macOS -> $MAC_OUT/Asteria"
