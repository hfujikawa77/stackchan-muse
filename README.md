# stackchan-muse

Stack-chan (M5Stack CoreS3) を Muse ガジェットとして動かすための拡張。
上流の [muse-gadget-sdk](https://github.com/facebookincubator/muse-gadget-sdk) は直接編集せず、
このリポジトリの内容を `tools/apply.sh` で SDK のクローンへ反映してビルドする。

## 構成
- `avatar/muse_pixel.c` ... ｽﾀｯｸﾁｬﾝ風アバターのレンダラ（SDK の `components/muse/avatar/` へコピー。上流側では gitignore 対象）
- `components/stackchan_hw/` ... サーボ・頭タッチ・LED のドライバ（ESP-IDF コンポーネント）
- `patches/` ... SDK への最小パッチ（Muse コマンド登録など）
- `sdkconfig.stackchan` ... CoreS3 向けの追加 sdkconfig
- `tools/apply.sh` / `tools/build.sh` ... 反映とビルド

## 前提
- ESP-IDF **v6.0.3**（WSL2 Ubuntu に導入）
- SDK トークン（gadgets.muse.ai）と利用規約への同意
- `muse-gadget-sdk` を隣のディレクトリに clone 済み（`SDK_DIR` で変更可）

## 使い方（WSL2）
```sh
. ~/esp/esp-idf-v6/export.sh
tools/build.sh build      # apply → CoreS3 ビルド
tools/build.sh flash PORT
```
SDK が /mnt/c 配下だとビルドが遅い。WSL のネイティブ領域に clone して `SDK_DIR` を指定するのを推奨。

## ライセンス
自作部分は Apache-2.0 を想定。m5stack-avatar (MIT) を参考/流用する場合は、ヘッダーとライセンス表記を残す。
Jollybot（SDK 同梱アバター）は Apache の対象外なので触らない。
