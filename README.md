# stackchan-muse

Stack-chan (M5Stack CoreS3) を Muse ガジェットとして動かすための拡張。
上流の [muse-gadget-sdk](https://github.com/facebookincubator/muse-gadget-sdk) は直接編集せず、
このリポジトリの内容を `tools/apply.sh` で SDK のクローンへ反映してビルドする。

## 構成
- `avatar/muse_pixel.c` ... ｽﾀｯｸﾁｬﾝ風アバターのレンダラ（SDK の `components/muse/avatar/` へコピー。上流側では gitignore 対象）
- `components/stackchan_hw/` ... サーボ・頭タッチ・LED のドライバ（ESP-IDF コンポーネント）
- `components/stackchan_tts/` ... Google Cloud TTS による読み上げ
- `patches/` ... SDK への最小パッチ（`0001` 追加 sdkconfig、`0002` Google TTS）
- `sdkconfig.stackchan` ... CoreS3 向けの追加 sdkconfig
- `tools/apply.sh` / `tools/build.sh` ... 反映とビルド（WSL）
- `tools/flash.ps1` ... ビルド → 書き込みを1コマンドで行う（Windows）
- `tools/make-tts-patch.py` ... `patches/0002` の再生成
- `build-out/` ... ビルド成果物（gitignore 対象）

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
- 返事の音声: Google Cloud Text-to-Speech で読み上げる（`components/stackchan_tts` + `patches/0002`）
  - PWR を**長押し**して話しかける → 返事の文章を Google TTS で MP3 にして再生
  - 失敗・12 秒超過のときは、従来どおり画面のテキストのみ
- `sdkconfig.stackchan` で `CONFIG_MUSE_REALTIME_VOICE=y`（ライブ音声通話、実験的）も有効化済み。PWR の短いタップで開始するが、動作は未確認

## Google TTS の設定
Cloud Text-to-Speech API を有効にした API キーを、シリアルから NVS に保存する（キーはリポジトリ・ビルドに入らない。チャットにも貼らない）:
```powershell
python -c "import serial,getpass,time; k=getpass.getpass('tts key: '); s=serial.Serial('COM7',115200); s.write(('>tts_key='+k+'\n').encode()); time.sleep(2); print(s.read(1500).decode(errors='replace'))"
```
`@tts_key ok` で保存済み（再起動不要）。声は `>tts_voice=ja-JP-Neural2-C` のように変更できる（既定 `ja-JP-Neural2-B`）。キーの消去は `>tts_key=`。

遅延対策として、Google への接続を保持し、PWR 押下時に直近 5 秒以内に使っていなければ先に接続を温める（prewarm）。最初のチャンクは短く切って再生を早める。チャンクごとの Wi-Fi RSSI がログに出る。

## トラブルシュート
### Wi-Fi に繋がらない
シリアルログ（115200 bps）の `link.wifi` を見る。ハートビートが `wifi=down` のままなら未接続。
- `disconnected reason=2`（AUTH_EXPIRE）が繰り返される ... 認証失敗。パスワード変更、AP 側が 5GHz 専用や WPA3 専用、接続台数の上限などを疑う。スマホのテザリングは 2.4GHz・WPA2 にする。RSSI が -70 dBm より良ければ電波は問題ない
- 保存済みネットワークは Muse アプリ（BLE セットアップ）から設定し直せる

### PC に COM ポートが出ない
データ線のないケーブル、または別の USB ポートで解決することが多い。それでもだめなら RST ボタン長押しでブートローダーに入れる。

## 元のファームに戻す
HermesLLM に戻すときは、PlatformIO で `stackchan-arduino/examples/HermesLLM` を書き込む。

## ライセンス
自作部分は Apache-2.0（`LICENSE`）。`patches/` は上流 muse-gadget-sdk（Apache-2.0）への差分で、上流の著作権表示はそのまま残る。m5stack-avatar (MIT) を参考/流用する場合は、ヘッダーとライセンス表記を残す。
Jollybot（SDK 同梱アバター）は Apache の対象外なので触らない。
