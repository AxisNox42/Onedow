#!/usr/bin/env bash
# bin/Onedow -> bin/Onedow.zip
# 포함 파일은 scripts/package_manifest.txt 가 정한다 (zip_onedow.ps1 / macOS CI 와 같은 기준).
# 인자: [원본 폴더] [출력 ZIP] — 검증용 임시 fixture 를 위한 선택 인자다.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ONEDOW="${1:-$ROOT/bin/Onedow}"
ZIP="${2:-$ROOT/bin/Onedow.zip}"
MANIFEST="$ROOT/scripts/package_manifest.txt"

if [[ ! -d "$ONEDOW" ]]; then
  echo "bin/Onedow 없음"
  exit 1
fi
if [[ ! -f "$ONEDOW/macOS/Onedow" ]]; then
  echo "macOS/Onedow 없음 — package_macos.sh 먼저"
  exit 1
fi

# Stage only the manifest items; local saves and stale build outputs may live
# beside the executable after testing the package.
STAGE_ROOT="$(mktemp -d)"
trap 'rm -rf "$STAGE_ROOT"' EXIT
STAGE="$STAGE_ROOT/Onedow"
mkdir -p "$STAGE/WindowsOS" "$STAGE/macOS"

while IFS= read -r line || [[ -n "$line" ]]; do
  line="${line%$'\r'}"
  read -r kind path flag _ <<< "$line" || true
  [[ -z "${kind:-}" || "$kind" == \#* ]] && continue
  rel="${path%/}"
  if [[ ! -e "$ONEDOW/$rel" ]]; then
    if [[ "$kind" == required ]]; then
      echo "$path 없음 — package_macos.sh 먼저"
      exit 1
    fi
    continue
  fi
  mkdir -p "$(dirname "$STAGE/$rel")"
  cp -R "$ONEDOW/$rel" "$STAGE/$rel"
  if [[ "${flag:-}" == exec ]]; then
    chmod 755 "$STAGE/$rel"
  fi
done < "$MANIFEST"

mkdir -p "$(dirname "$ZIP")"
ZIP_ABS="$(cd "$(dirname "$ZIP")" && pwd)/$(basename "$ZIP")"
rm -f "$ZIP_ABS"
(cd "$STAGE_ROOT" && zip -r -q "$ZIP_ABS" Onedow)
echo ""
echo "LTS ZIP: $ZIP_ABS"
