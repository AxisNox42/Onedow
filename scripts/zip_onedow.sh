#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ONEDOW="$ROOT/bin/Onedow"
ZIP="$ROOT/bin/Onedow.zip"

if [[ ! -d "$ONEDOW" ]]; then
  echo "bin/Onedow 없음"
  exit 1
fi

rm -f "$ZIP"
(cd "$ROOT/bin" && zip -r Onedow.zip Onedow)

echo ""
echo "배포 ZIP: $ZIP"
