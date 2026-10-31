#!/usr/bin/env bash
# bin/Asteria -> bin/Asteria.zip
# 포함 파일은 scripts/package_manifest.txt 가 정한다 (zip_asteria.ps1 / macOS CI 와 같은 기준).
# 인자: [원본 폴더] [출력 ZIP] — 검증용 임시 fixture 를 위한 선택 인자다.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ASTERIA="${1:-$ROOT/bin/Asteria}"
ZIP="${2:-$ROOT/bin/Asteria.zip}"
MANIFEST="$ROOT/scripts/package_manifest.txt"

if [[ ! -d "$ASTERIA" ]]; then
  echo "bin/Asteria 없음"
  exit 1
fi
if [[ ! -f "$ASTERIA/macOS/Asteria" ]]; then
  echo "macOS/Asteria 없음 — package_macos.sh 먼저"
  exit 1
fi

# Stage only the manifest items; local saves and stale build outputs may live
# beside the executable after testing the package.
STAGE_ROOT="$(mktemp -d)"
trap 'rm -rf "$STAGE_ROOT"' EXIT
STAGE="$STAGE_ROOT/Asteria"
mkdir -p "$STAGE/WindowsOS" "$STAGE/macOS"

while IFS= read -r line || [[ -n "$line" ]]; do
  line="${line%$'\r'}"
  read -r kind path flag _ <<< "$line" || true
  [[ -z "${kind:-}" || "$kind" == \#* ]] && continue
  rel="${path%/}"
  if [[ ! -e "$ASTERIA/$rel" ]]; then
    if [[ "$kind" == required ]]; then
      echo "$path 없음 — package_macos.sh 먼저"
      exit 1
    fi
    continue
  fi
  mkdir -p "$(dirname "$STAGE/$rel")"
  cp -R "$ASTERIA/$rel" "$STAGE/$rel"
  if [[ "${flag:-}" == exec ]]; then
    chmod 755 "$STAGE/$rel"
  fi
done < "$MANIFEST"

mkdir -p "$(dirname "$ZIP")"
ZIP_ABS="$(cd "$(dirname "$ZIP")" && pwd)/$(basename "$ZIP")"
rm -f "$ZIP_ABS"
(cd "$STAGE_ROOT" && zip -r -q "$ZIP_ABS" Asteria)
echo ""
echo "LTS ZIP: $ZIP_ABS"
