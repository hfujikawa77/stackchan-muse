#!/usr/bin/env bash
# 使い方: tools/build.sh build|flash [PORT]
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
SDK_DIR="${SDK_DIR:-$HERE/../muse-gadget-sdk}"
"$HERE/tools/apply.sh"
cd "$SDK_DIR/esp32"
exec tools/muse/board.sh "${1:-build}" cores3 "${@:2}"
