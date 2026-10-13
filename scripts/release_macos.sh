#!/usr/bin/env bash
set -euo pipefail
# Scripts are stored without the execute bit (100644); run them through bash.
bash "$(dirname "$0")/package_macos.sh"
bash "$(dirname "$0")/zip_onedow.sh"
