# stackchan-muse

Stack-chan (M5Stack CoreS3) を Muse ガジェットとして動かすための拡張。
上流の [muse-gadget-sdk](https://github.com/facebookincubator/muse-gadget-sdk) は直接編集せず、
このリポジトリの内容を `tools/apply.sh` で SDK のクローンへ反映してビルドする。

## 構成
- `avatar/muse_pixel.c` ... ｽﾀｯｸﾁｬﾝ風アバターのレンダラ（SDK の `components/muse/avatar/` へコピー。上流側では gitignore 対象）
- `components/stackchan_hw/` ... サーボ・LED・頭タッチのドライバ（ESP-IDF コンポーネント）
- `components/stackchan_tts/` ... Google Cloud TTS による読み上げ
- `patches/` ... SDK への最小パッチ（`0001` 追加 sdkconfig、`0002` Google TTS、`0003` サーボ・LED のコマンド、`0004` `stackchan.say`）。上流 cc852b4（2026-11-09）で適用確認
- `sdkconfig.stackchan` ... CoreS3 向けの追加 sdkconfig
- `tools/apply.sh` / `tools/build.sh` ... 反映とビルド（WSL）
- `tools/flash.ps1` ... ビルド → 書き込みを1コマンドで行う（Windows）
- `tools/make-tts-patch.py` / `tools/make-hw-patch.py` / `tools/make-say-patch.py` ... `patches/0002` / `0003` / `0004` の再生成
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

## サーボ・LED
Muse からは `stackchan.led`（`r`,`g`,`b`）と `stackchan.servo`（`x`,`y`,`ms`。x/y を省くと正面）で動かす。
シリアルからも試せる（`>` で始め、LF 付き1行で送る）:

| コマンド | 内容 |
|---|---|
| `>led=R,G,B` | 12 個の LED を同じ色に（`0,0,0` で消灯） |
| `>servo=X,Y[,MS]` | 首を動かす。X 60〜240（150 が正面）、Y 70〜90（90 が水平）。範囲外は丸める |
| `>servo=center` / `off` / `on` | 正面へ / トルクを切る / 入れる |
| `>anim=0` / `1` | 発話中の LED・首振りを止める / 戻す |
| `>sway=DEG,HALF_MS[,UP]` | 首振りの振れ幅、片道の時間、見上げる角度を変える（既定 15°, 700ms, 8°。再起動で戻る） |
| `>swaytest=SEC` | 話しかけずに、首振りを SEC 秒だけ動かす（調整用） |
| `>pet=0` / `1` | ナデナデの反応を止める / 戻す |
| `>ledfx=NAME[,SPEED[,R,G,B[,R2,G2,B2]]]` | LED の演出。rainbow / breath / blink / strobe / chase / wave / alternate / solid、`off` で止める |
| `>ledpreset=NAME` | party / ocean / christmas / rainbow / calm_breath / alert_red / warm_white / cool_blue |
| `>uifx=0` / `1` | 聞いている・考え中の LED を止める / 戻す |
| `>servo=off` → 手で正面に合わせる → `>servo=zero` → `>servo=on` | 正面（X 150, Y 90）の校正。NVS に保存。`>servo=zero_reset` で消す |
| `>servo=trim=DX,DY` | 正面を DX/DY 度だけずらして保存（Y はマイナスで上向き） |
| `>servo=pos` / `>touch` | デバッグ用。サーボの実位置とばねの状態 / 頭タッチの生の値 |
| `>py32r=REG[,LEN]` / `>i2cr=ADDR,REG[,LEN]` | デバッグ用のレジスタ読み出し |

- 発話中は、音量に合わせて LED が光り、少し見上げて（8°）首が左右に揺れる。終わると元の色・位置に戻る
- LED は上の層から表示する: 発話（声に合わせてオレンジ）> ナデナデ（ピンク）> 聞いている（青の呼吸）・考え中（黄色の光が回る）> 演出 > 基本色。Muse からは `stackchan.led` の `effect` / `preset` で演出を指定できる
- 頭を前後になでると（タップは無視）、LED がピンクになり、見上げるか首をかしげる。手を離して 3 秒で元に戻る。発話中は反応しない
- Y は小さいほど見上げる（Y 70〜90 = 水平から最大 20° 上）
- サーボは公式 StackChan ファームと同じく、ESP32 側のばねモデルを 50Hz で計算し、20ms の移動として送る
- サーボの電源は PY32 のピン0（VM EN）、LED のデータ線は PY32 のピン13
- LED は CoreS3 から本体への 5V（AW9523 の BUS_EN・BOOST_EN）で光る。esp-bsp がこの2ビットを消すので、ドライバが 300ms ごとに立て直している

## トラブルシュート
### Wi-Fi に繋がらない
シリアルログ（115200 bps）の `link.wifi` を見る。ハートビートが `wifi=down` のままなら未接続。
- `disconnected reason=2`（AUTH_EXPIRE）が繰り返される ... 認証失敗。パスワード変更、AP 側が 5GHz 専用や WPA3 専用、接続台数の上限などを疑う。スマホのテザリングは 2.4GHz・WPA2 にする。RSSI が -70 dBm より良ければ電波は問題ない
- 保存済みネットワークは Muse アプリ（BLE セットアップ）から設定し直せる

### PC に COM ポートが出ない
データ線のないケーブル、または別の USB ポートで解決することが多い。それでもだめなら RST ボタン長押しでブートローダーに入れる。

## 元のファームに戻す
HermesLLM に戻すときは、PlatformIO で `stackchan-arduino/examples/HermesLLM` を書き込む。

## Muse コマンド `stackchan.say`
Muse（エージェント）から任意のテキストをスタックチャンに喋らせる（`components/stackchan_tts/stackchan_say.c` + `patches/0004`）。
- 引数: `text`（文字列・必須）。1023 バイトを超える分は UTF-8 の文字境界で切り捨て
- 受け付けた時点で即座に `{"ok":true,"payload":{"queued":true,"bytes":N,"truncated":false}}` を返す（再生完了は待たない）
- 本体がアイドルになったら Google TTS で合成して再生。返事と同じ経路なので、アバターの speaking 表示・字幕・発話中の LED と首振り（`stackchan_hw`）もそのまま連動する
- チャットには投稿しない（履歴に残らない）。再生前に新しい say が来たら置き換え。会話中などで 60 秒以内に始められなければ破棄
- 再生中に PWR を押すと中断して通常の録音に移る
- TTS キー未設定なら `tts_not_configured` エラーを返す

## ライセンス
自作部分は Apache-2.0（`LICENSE`）。`patches/` は上流 muse-gadget-sdk（Apache-2.0）への差分で、上流の著作権表示はそのまま残る。m5stack-avatar (MIT) を参考/流用する場合は、ヘッダーとライセンス表記を残す。
Jollybot（SDK 同梱アバター）は Apache の対象外なので触らない。
