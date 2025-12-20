# XIAO ESP32C6 Micro-Mouse Robot Control

このプロジェクトは、**Seeed Studio XIAO ESP32C6** を搭載した小型二輪ロボット（マイクロマウス）向けの制御プログラムです。

## ハードウェア構成

* **マイコン**: Seeed Studio XIAO ESP32C6
* **ディスプレイ**: SSD1306 0.96インチ OLED (I2C接続)
* **センサー**: HC-SR04 超音波距離センサー
* **モータードライバ**: Hブリッジモータードライバ (2モーター制御用)
* **モーター**: DCモーター x 2 (左車輪・右車輪)

## ソフトウェア構成

### Arduino IDE

### 必須ライブラリ
Arduino IDEのライブラリマネージャから以下のライブラリをインストールしてください。

* **Adafruit GFX Library**
* **Adafruit SSD1306**
* **Ticker** (ESP32標準搭載)
