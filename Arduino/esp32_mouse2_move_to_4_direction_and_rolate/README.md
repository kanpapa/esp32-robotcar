# XIAO ESP32C6 Micro-Mouse Robot Control

このプロジェクトは、**Seeed Studio XIAO ESP32C6** を搭載した小型二輪ロボット（マイクロマウス）向けの制御プログラムです。

ロボットはあらかじめ定義されたパターン（前後左右・回転）で走行し、同時にバックグラウンド処理で超音波センサーによる距離計測を行い、その結果をOLEDディスプレイにリアルタイム表示します。

## 📝 概要

ファイル名: `esp32_mouse2_move_to_4_direction_and_rolate.ino`

このスケッチは以下の2つのタスクを並行して実行します：
1.  **動作制御 (Main Loop)**: 前進、後退、旋回、回転、停止を組み合わせた「ZIGZAG」パターンを繰り返します。
2.  **距離測定と表示 (Ticker)**: `Ticker`ライブラリを使用し、0.3秒ごとに超音波センサーで距離を計測し、OLEDに数値とバーで表示します。

## 🛠 ハードウェア構成

* **マイコン**: Seeed Studio XIAO ESP32C6
* **ディスプレイ**: SSD1306 0.96インチ OLED (I2C接続)
* **センサー**: HC-SR04 超音波距離センサー
* **モータードライバ**: Hブリッジモータードライバ (2モーター制御用)
* **モーター**: DCモーター x 2 (左車輪・右車輪)

## 🔌 ピン配置 (Pin Configuration)

### モーター制御 (Motor Driver)
| 変数名 | 機能 | XIAO GPIO | 備考 |
| :--- | :--- | :--- | :--- |
| `LP` | 左モーター (+) | **GPIO 0** | (AIN1) |
| `LM` | 左モーター (-) | **GPIO 1** | (AIN2) |
| `RP` | 右モーター (-) | **GPIO 2** | (BIN1) |
| `RM` | 右モーター (+) | **GPIO 21** | (BIN2) |

### 超音波センサー (HC-SR04)
| 機能 | XIAO GPIO |
| :--- | :--- |
| Trigger | **GPIO 18** |
| Echo | **GPIO 20** |

### OLEDディスプレイ (I2C)
| 機能 | 接続 | アドレス |
| :--- | :--- | :--- |
| SDA | I2C SDA | `0x3C` |
| SCL | I2C SCL | |

## 📚 必須ライブラリ

Arduino IDEのライブラリマネージャから以下のライブラリをインストールしてください。

* **Adafruit GFX Library**
* **Adafruit SSD1306**
* **Ticker** (ESP32標準搭載)

## 🚀 動作仕様

### 1. 走行パターン (ZIGZAG Sequence)
`loop()`関数内で以下の順序で動作を繰り返します（各動作間隔: 1秒）。

1.  **TTL (Turn To Left)**: 左へ緩やかに旋回
2.  **TTR (Turn To Right)**: 右へ緩やかに旋回
3.  **FWD (Forward)**: 直進
4.  **BAK (Back)**: 後退
5.  **RTL (Rotate To Left)**: その場で左回転（超信地旋回）
6.  **RTR (Rotate To Right)**: その場で右回転（超信地旋回）
7.  **STOP**: 停止

### 2. ディスプレイ表示 (Display)
`Ticker`割り込みにより0.3秒ごとに更新されます。
* **数値表示**: 測定した距離（cm）を表示します。
* **バー表示**: 距離（0〜200cm）に応じたプログレスバーを表示します。
* **範囲外**: 測定不能な場合は "Out of Range" を表示します。

## 📦 インストール方法

1.  Arduino IDEを開き、`esp32_mouse2_move_to_4_direction_and_rolate.ino` を作成します。
2.  ボードマネージャで **"XIAO ESP32C6"** を選択します。
3.  必要なライブラリをインストールします。
4.  ハードウェアを配線図（ピン配置）に従って接続します。
5.  スケッチをコンパイルし、書き込みます。

## 📜 License
This project is open source.