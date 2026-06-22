#!/usr/bin/env bash
set -euo pipefail
"$(dirname "$0")/package_macos.sh"
"$(dirname "$0")/zip_onedow.sh"
