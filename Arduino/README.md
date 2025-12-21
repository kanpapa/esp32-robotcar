# Program Revision History

このリポジトリのArduinoスケッチは、以下の順序で機能追加・修正を行いました。

| Ver | 変更概要 | 技術的な変更点 |
| :--- | :--- | :--- |
| **0** | **[初期バージョン](esp32_mouse2_move_to_4_direction_and_rolate_0)** | ・ZIGZAG動作（前後左右・回転）<br>・距離測定とプログレスバー表示 |
| **1** | **[動作ステータスの英語表示化](esp32_mouse2_move_to_4_direction_and_rolate_1)** | ・OLEDに現在の動作（"FORWARD", "TURN LEFT"など）を表示するように変更<br>・グローバル変数 `current_movement` の導入 |
| **2** | **[表示の最大化と動作確認モード](esp32_mouse2_move_to_4_direction_and_rolate_2)** | ・動作時間を1秒→5秒に変更<br>・距離表示を一時停止し、動作ステータスを最大フォントで画面中央に表示 |
| **3** | **[90度回転テスト](esp32_mouse2_move_to_4_direction_and_rolate_3)** | ・ZIGZAG動作を廃止<br>・「右に90度回転（仮設定1000ms）」を4回繰り返すループに変更 |
| **4** | **[四角形描画 (テスト1)](esp32_mouse2_move_to_4_direction_and_rolate_4)** | ・直進(1000ms) ＋ 右回転(400ms) を繰り返して四角形を描く動作に変更<br>・回転時間を短縮して調整 |
| **5** | **[四角形描画 (テスト2)](esp32_mouse2_move_to_4_direction_and_rolate_5)** | ・直進(2000ms) ＋ 右回転(600ms) に時間を変更<br>・動作確認用の調整バージョン |
| **6** | **[迷路脱出 (右手法・ブロッキング)](esp32_mouse2_move_to_4_direction_and_rolate_6)** | ・壁検知ロジックの実装（15cm以下なら右回転、それ以外は直進）<br>・`delay()`を使用した実装のため、直進中の衝突検知は不可 |
| **7** | **[迷路脱出 (非ブロッキング制御)](esp32_mouse2_move_to_4_direction_and_rolate_7)** | ・**大幅な構造変更**<br>・`delay()`を廃止し、`millis()`とステートマシンを導入<br>・移動中も常時距離測定を行い、即座に壁回避が可能に |
| **8** | **[回転精度の最終調整 (完成版)](esp32_mouse2_move_to_4_direction_and_rolate_8)** | ・回転時間を600ms→**300ms**に修正<br>・過剰な回転（180度）を防ぎ、適切な90度回転に調整<br>・距離表示機能の復帰 |

---
**補足:**
* **Ver 7以降**が、より実践的で安全なロボット制御コード（非ブロッキング処理）になっています。
* 実際の迷路探索には **Ver 8** のコードを使用してください。

# 開発情報
* [開発ログ](DEVLOG.md)
* [アイデア集](NEXTSTEP.md)