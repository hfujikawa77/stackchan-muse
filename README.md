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
- `muse-gadget-sdk` を clone 済み（既定は隣のディレクトリ。`SDK_DIR` で変更可。WSL では `~/muse-gadget-sdk` を推奨）

## 使い方（WSL2 + Windows）
ビルドは WSL2、書き込みは Windows の COM ポートから行う（usbipd・管理者権限は不要）。

### WSL 側の準備（初回のみ）
- ESP-IDF v6.0.3 を `~/esp/esp-idf-v6` に導入（`install.sh esp32s3`）
- `~/muse-gadget-sdk`（上流のクローン）と `~/stackchan-muse`（このリポジトリ）を clone
- SDK は /mnt/c ではなく WSL のネイティブ領域に置く（速く、OneDrive 同期とも干渉しない）

### ビルド + 書き込み（1コマンド）
```powershell
.\tools\flash.ps1                # WSL でビルド → build-out/ にコピー → COM7 へ書き込み
.\tools\flash.ps1 -NoBuild       # 前回のビルドをそのまま書き込む
.\tools\flash.ps1 -Port COM5     # ポート指定
```
- WSL 内で `git pull` してからビルドするので、Windows 側の変更は先にコミットしておく
- シリアルモニタを開いたままだと書き込みに失敗する。先に閉じる
- 接続できないときは RST ボタンを約3秒（緑 LED が点くまで）押してブートローダーに入れる
- 書き込み後もトークン・ペアリングは NVS に残る
- PlatformIO 同梱の esptool は古く、オプションはアンダースコア形式（`default_reset` / `write_flash`）

### 手動でビルドだけ行う場合（WSL）
```sh
. ~/esp/esp-idf-v6/export.sh
SDK_DIR=~/muse-gadget-sdk ~/stackchan-muse/tools/build.sh build
```
出力: `~/muse-gadget-sdk/esp32/build-muse-m5stack-cores3-stackchan/`

## SDK トークンの設定（初回）
トークンは gadgets.muse.ai で取得する。**チャットやリポジトリに貼らない。** デバイスのシリアルコンソールへ、LF 付き1行で送る（端末の Enter は CR のみで効かないことがある）:
```powershell
python -c "import serial,getpass,time; t=getpass.getpass('token: '); s=serial.Serial('COM7',115200); s.write(('>sdk_token='+t+'\n').encode()); time.sleep(2); print(s.read(1500).decode(errors='replace'))"
```
`@sdk_token ok` で再起動する。そのあと Muse アプリ（設定 → デバイス → Developer mode）で `MuseGadget-XXXXXX` をペアリング。

## 使い方（Stack-chan）
- 返事の音声: `sdkconfig.stackchan` で `CONFIG_MUSE_REALTIME_VOICE=y`（ライブ音声通話）を有効化済み
  - PWR を**短くタップ**: 通話の開始/終了（Muse が声で返事する）
  - PWR を**長押し**: 音声メモ（返事は画面のテキストのみ。Muse はガジェットへの返事を読み上げない）
- 元の HermesLLM に戻すときは、PlatformIO で `stackchan-arduino/examples/HermesLLM` を書き込む

## ライセンス
自作部分は Apache-2.0 を想定。m5stack-avatar (MIT) を参考/流用する場合は、ヘッダーとライセンス表記を残す。
Jollybot（SDK 同梱アバター）は Apache の対象外なので触らない。
