#!/usr/bin/env bash
# 使い方: tools/build.sh build|flash [PORT]
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
SDK_DIR="${SDK_DIR:-$HERE/../muse-gadget-sdk}"
"$HERE/tools/apply.sh"
cd "$SDK_DIR/esp32"
# A build directory keeps its sdkconfig, and ESP-IDF applies defaults files only to options
# that sdkconfig lacks, so an option added to sdkconfig.stackchan later would be ignored.
# Drop those options from the old sdkconfig; the build then takes them from sdkconfig.stackchan.
for cfg in build-*-stackchan/sdkconfig; do
  [ -f "$cfg" ] || continue
  grep -o '^CONFIG_[A-Z0-9_]*' "$HERE/sdkconfig.stackchan" | while read -r key; do
    sed -i "/^${key}=/d; /^# ${key} is not set\$/d" "$cfg"
  done
done
exec tools/muse/board.sh "${1:-build}" cores3 "${@:2}"
