#!/usr/bin/env bash
# このリポジトリの内容を muse-gadget-sdk のクローンへ反映する（冪等）。
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
SDK_DIR="${SDK_DIR:-$HERE/../muse-gadget-sdk}"
ESP="$SDK_DIR/esp32"
[ -d "$ESP" ] || { echo "SDK not found: $ESP (set SDK_DIR)" >&2; exit 2; }

# アバター（上流では gitignore 対象）
if [ -f "$HERE/avatar/muse_pixel.c" ]; then
  cp "$HERE/avatar/muse_pixel.c" "$ESP/components/muse/avatar/muse_pixel.c"
  echo "avatar: copied"
fi

# 独自コンポーネント
for c in "$HERE"/components/*/; do
  n="$(basename "$c")"
  rm -rf "$ESP/components/$n"
  cp -r "$c" "$ESP/components/$n"
  echo "components: $n copied"
done

# 追加 sdkconfig（patches/0001 で board.sh が読み込む）
cp "$HERE/sdkconfig.stackchan" "$ESP/devices/sdkconfig.stackchan"
echo "sdkconfig: copied"

# 3D アバターの素材（上流 PR #513 の muse3d.bin。パッチ 0000 が埋め込む）。Jollybot の絵なので
# このリポジトリには入れず、PR の固定コミットから取って SHA-256 を確かめる（#16）。
MUSE3D_URL="https://raw.githubusercontent.com/gpulkitg/muse-gadget-sdk/f34ff31eaf87d32433ddec49fbe98306e1804df2/esp32/avatar/3d/muse3d.bin"
MUSE3D_SHA256="36427c5dde3ba11ad080318ccc010d4a635c8e0e9afb987bc2dddd1f53918cda"
MUSE3D="$ESP/avatar/3d/muse3d.bin"
if ! echo "$MUSE3D_SHA256  $MUSE3D" | sha256sum -c --status 2>/dev/null; then
  mkdir -p "$(dirname "$MUSE3D")"
  curl -fsSL -o "$MUSE3D.tmp" "$MUSE3D_URL"
  echo "$MUSE3D_SHA256  $MUSE3D.tmp" | sha256sum -c --status || { rm -f "$MUSE3D.tmp"; echo "muse3d.bin: SHA-256 mismatch" >&2; exit 3; }
  mv "$MUSE3D.tmp" "$MUSE3D"
  echo "muse3d.bin: fetched"
fi

# パッチ（適用済みならスキップ）
# パッチは重なる（0003 は 0002 と同じファイルを触る）ので、適用済みのものを後ろから外してから、前から入れ直す。
shopt -s nullglob
patches=("$HERE"/patches/*.patch)
for ((i=${#patches[@]}-1; i>=0; i--)); do
  p="${patches[$i]}"
  if git -C "$SDK_DIR" apply --reverse --check "$p" >/dev/null 2>&1; then
    git -C "$SDK_DIR" apply --reverse "$p"
  fi
done
for p in "${patches[@]}"; do
  git -C "$SDK_DIR" apply "$p"
  echo "patch: applied $(basename "$p")"
done
