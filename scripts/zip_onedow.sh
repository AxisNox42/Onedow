#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ZIPDIR="$ROOT/bin/Zip"
ONEDOW="$ZIPDIR/Onedow"

if [[ ! -d "$ONEDOW" ]]; then
  echo "bin/Zip/Onedow 없음"
  exit 1
fi

rm -f "$ZIPDIR/Onedow.zip"
(cd "$ZIPDIR" && zip -r Onedow.zip Onedow)
echo ""
echo "배포 ZIP: $ZIPDIR/Onedow.zip"
