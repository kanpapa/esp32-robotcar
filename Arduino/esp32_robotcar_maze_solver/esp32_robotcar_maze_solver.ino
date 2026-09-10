//-----------------------------------------------------
// esp32_robotcar_maze_solver.ino
//
// 対象ボード: XIAO ESP32C3
//
// 概要:
//   超音波センサーで前方の壁までの距離を測り、
//   壁が無ければ前進、壁があれば右に回転する
//   「右手法」で迷路を進むロボットのプログラムです。
//
//   OLEDディスプレイに現在の距離と動作状態を表示します。
//
// 2026/03/15 - 非ブロッキング衝突検知に対応した迷路走行版
//-----------------------------------------------------

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ticker.h>

// =====================================================
// 定数定義（ピン番号・タイミング・しきい値など）
// =====================================================

// --- OLEDディスプレイ関連 ---
#define SCREEN_WIDTH   128   // ディスプレイの幅（ピクセル）
#define SCREEN_HEIGHT  64    // ディスプレイの高さ（ピクセル）
#define OLED_RESET     -1    // リセットピンは使わない（-1）
#define SCREEN_ADDRESS 0x3C  // OLEDのI2Cアドレス

// --- 超音波距離センサー（HC-SR04など）のピン ---
const int TRIG_PIN = D8;  // 超音波を発信するピン
const int ECHO_PIN = D10; // 反射波を受信するピン

// 距離センサーのタイムアウト用（これ以上遠いものは測らない）
const unsigned long MAX_DISTANCE_TIMEOUT_US = 23200 * 1.5;

// --- モータードライバーのピン（左右それぞれ+/-の2本ずつ） ---
const int LEFT_MOTOR_PLUS   = D0; // 左モーター  正転側
const int LEFT_MOTOR_MINUS  = D1; // 左モーター  逆転側
const int RIGHT_MOTOR_PLUS  = D2; // 右モーター  正転側
const int RIGHT_MOTOR_MINUS = D3; // 右モーター  逆転側

// モーターの出力(PWM値 0〜255)。値が大きいほど速く回る
const int MOTOR_SPEED = 250;
const int MOTOR_SPEED_SLOW = 100; // 緩やかに曲がるときの弱い方の出力

// --- ロボットの動作タイミング ---
const int ROTATE_90_DEGREES_MS = 300;   // 右に90度回転するのにかかる時間(ms)
const int FORWARD_DURATION_MS  = 2000;  // 1回で前進する時間(ms)
const float WALL_THRESHOLD_CM  = 15.0;  // これより近いと「壁がある」と判断する距離(cm)

// ディスプレイを更新する間隔(秒)
const float DISPLAY_UPDATE_INTERVAL_SEC = 0.3;

// =====================================================
// グローバル変数
// =====================================================

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Ticker displayUpdateTimer; // 一定間隔でディスプレイを更新するタイマー

// 現在測定されている距離(cm)。ディスプレイ表示用に保持しておく
float latestDistanceCm = 0;

// 画面に表示する現在の動作名（例: "MAZE: FORWARD"）
String currentActionLabel = "INITIALIZING";

// ロボットの状態（今何をしているか）を表す
enum RobotState {
  STATE_STOPPED,        // 停止中（次の動作を判断するタイミング）
  STATE_FORWARD,        // 前進中
  STATE_TURNING_RIGHT   // 右回転中
};
RobotState robotState = STATE_STOPPED;

// 現在の動作（前進 or 回転）を開始した時刻(ms)。経過時間の計算に使う
unsigned long actionStartTimeMs = 0;

// =====================================================
// セットアップ
// =====================================================

void setup() {
  Serial.begin(99600);

  setupDistanceSensorPins();
  setupMotorPins();
  setupDisplay();
  stopMotors();

  // 0.3秒ごとに updateDisplay() を自動で呼び出す
  displayUpdateTimer.attach(DISPLAY_UPDATE_INTERVAL_SEC, updateDisplay);
}

void setupDistanceSensorPins() {
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);
}

void setupMotorPins() {
  // モーター用のピンはanalogWriteでそのまま使うため、
  // 明示的なpinMode設定は不要（元コードの挙動を踏襲）
}

void setupDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true) {
      // ディスプレイの初期化に失敗した場合はここで停止する
    }
  }

  display.display();
  delay(1000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Maze Solver Ready."));
  display.display();
  delay(1000);
}

// =====================================================
// メインループ（ステートマシンで非ブロッキングに動作）
// =====================================================

void loop() {
  // 毎回、前方までの距離を測定して保存しておく
  latestDistanceCm = measureDistanceCm();

  // 現在の状態に応じて処理を分岐する
  switch (robotState) {
    case STATE_STOPPED:
      decideNextAction(latestDistanceCm);
      break;

    case STATE_FORWARD:
      updateForwardState(latestDistanceCm);
      break;

    case STATE_TURNING_RIGHT:
      updateTurningRightState();
      break;
  }

  // ループが速く回りすぎないように、ごく短い待ちを入れる
  delay(10);
}

// --- STATE_STOPPED: 停止中に次の動作を決める ---
void decideNextAction(float distanceCm) {
  bool wallDetected = (distanceCm <= WALL_THRESHOLD_CM);

  if (!wallDetected) {
    // 前方に壁が無い → 前進を開始
    moveForward();
    actionStartTimeMs = millis();
    robotState = STATE_FORWARD;
    currentActionLabel = "MAZE: FORWARD";
  } else {
    // 前方に壁がある → 右手法にしたがい右に回転を開始
    rotateRight();
    actionStartTimeMs = millis();
    robotState = STATE_TURNING_RIGHT;
    currentActionLabel = "MAZE: TURN RIGHT";
  }
}

// --- STATE_FORWARD: 前進中の処理 ---
void updateForwardState(float distanceCm) {
  // 距離 > 0 のチェックは、センサーの測定エラー(-1など)を
  // 「壁を検知した」と誤判定しないようにするためのもの
  bool obstacleAhead = (distanceCm <= WALL_THRESHOLD_CM && distanceCm > 0);
  bool forwardTimeElapsed = (millis() - actionStartTimeMs >= FORWARD_DURATION_MS);

  if (obstacleAhead) {
    // 前進中に壁を検知 → ただちに停止し、次の判断へ
    stopMotors();
    robotState = STATE_STOPPED;
  } else if (forwardTimeElapsed) {
    // 既定の前進時間が経過 → 停止し、次の判断へ
    stopMotors();
    robotState = STATE_STOPPED;
  }
}

// --- STATE_TURNING_RIGHT: 右回転中の処理 ---
void updateTurningRightState() {
  bool rotationTimeElapsed = (millis() - actionStartTimeMs >= ROTATE_90_DEGREES_MS);

  if (rotationTimeElapsed) {
    // 既定の回転時間が経過 → 停止し、次の判断へ
    stopMotors();
    robotState = STATE_STOPPED;
  }
}

// =====================================================
// ディスプレイ表示
// =====================================================

// Ticker により DISPLAY_UPDATE_INTERVAL_SEC 秒ごとに自動で呼ばれる
void updateDisplay() {
  display.clearDisplay();

  drawDistanceText();
  drawActionLabel();

  display.display();
}

// 画面左上に距離を表示する
void drawDistanceText() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Dist: ");

  bool distanceIsOutOfRange = (latestDistanceCm > 200 || latestDistanceCm <= 0);
  if (distanceIsOutOfRange) {
    display.println("FAR/ERR");
  } else {
    display.print(latestDistanceCm);
    display.println(" cm");
  }
}

// 画面中央付近に現在の動作名を大きく表示する
void drawActionLabel() {
  display.setTextSize(2);

  int16_t x1, y1;
  uint16_t textWidth, textHeight;
  display.getTextBounds(currentActionLabel, 0, 0, &x1, &y1, &textWidth, &textHeight);

  int centeredX = (SCREEN_WIDTH - textWidth) / 2;
  int fixedY = 30; // 画面中央よりやや下の位置

  display.setCursor(centeredX, fixedY);
  display.println(currentActionLabel);
}

// =====================================================
// モーター制御
// =====================================================
// 各関数は左右のモーターにPWM値を書き込み、ロボットの動きを決める。

void moveForward() {
  analogWrite(LEFT_MOTOR_PLUS,   MOTOR_SPEED);
  analogWrite(LEFT_MOTOR_MINUS,  0);
  analogWrite(RIGHT_MOTOR_PLUS,  MOTOR_SPEED);
  analogWrite(RIGHT_MOTOR_MINUS, 0);
}

void moveBackward() { // 現状未使用だが今後のために残す
  analogWrite(LEFT_MOTOR_PLUS,   0);
  analogWrite(LEFT_MOTOR_MINUS,  MOTOR_SPEED);
  analogWrite(RIGHT_MOTOR_PLUS,  0);
  analogWrite(RIGHT_MOTOR_MINUS, MOTOR_SPEED);
}

void turnLeftGently() { // ゆるやかに左へ曲がる（現状未使用だが今後のために残す）
  analogWrite(LEFT_MOTOR_PLUS,   MOTOR_SPEED_SLOW);
  analogWrite(LEFT_MOTOR_MINUS,  0);
  analogWrite(RIGHT_MOTOR_PLUS,  MOTOR_SPEED);
  analogWrite(RIGHT_MOTOR_MINUS, 0);
}

void turnRightGently() { // ゆるやかに右へ曲がる（現状未使用だが今後のために残す）
  analogWrite(LEFT_MOTOR_PLUS,   MOTOR_SPEED);
  analogWrite(LEFT_MOTOR_MINUS,  0);
  analogWrite(RIGHT_MOTOR_PLUS,  MOTOR_SPEED_SLOW);
  analogWrite(RIGHT_MOTOR_MINUS, 0);
}

void rotateRight() { // その場で右に回転する（迷路走行で実際に使用）
  analogWrite(LEFT_MOTOR_PLUS,   MOTOR_SPEED);
  analogWrite(LEFT_MOTOR_MINUS,  0);
  analogWrite(RIGHT_MOTOR_PLUS,  0);
  analogWrite(RIGHT_MOTOR_MINUS, MOTOR_SPEED);
}

void rotateLeft() { // その場で左に回転する（現状未使用だが今後のために残す）
  analogWrite(LEFT_MOTOR_PLUS,   0);
  analogWrite(LEFT_MOTOR_MINUS,  MOTOR_SPEED);
  analogWrite(RIGHT_MOTOR_PLUS,  MOTOR_SPEED);
  analogWrite(RIGHT_MOTOR_MINUS, 0);
}

void stopMotors() {
  analogWrite(LEFT_MOTOR_PLUS,   0);
  analogWrite(LEFT_MOTOR_MINUS,  0);
  analogWrite(RIGHT_MOTOR_PLUS,  0);
  analogWrite(RIGHT_MOTOR_MINUS, 0);
}

// =====================================================
// 距離センサー（超音波センサー）
// =====================================================

// TRIG/ECHOピンを使って前方までの距離を測定する。
// 測定に失敗（タイムアウト）した場合は -1.0 を返す。
float measureDistanceCm() {
  // 1. トリガーピンを10マイクロ秒だけHIGHにして超音波を発射させる
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 2. エコーピンがHIGHになる（反射波を受信し始める）のを待つ
  unsigned long waitStartTime = micros();
  while (digitalRead(ECHO_PIN) == LOW &&
         (micros() - waitStartTime) < MAX_DISTANCE_TIMEOUT_US) {
    // 受信開始を待機中
  }

  if (digitalRead(ECHO_PIN) == LOW) {
    // タイムアウト＝反射波が返ってこなかった（範囲外）
    return -1.0;
  }

  // 3. エコーピンがHIGHの間（パルス幅）の時間を測る
  unsigned long pulseStartTime = micros();
  unsigned long pulseWaitStartTime = micros();
  while (digitalRead(ECHO_PIN) == HIGH &&
         (micros() - pulseWaitStartTime) < MAX_DISTANCE_TIMEOUT_US) {
    // パルスが終わるのを待機中
  }
  unsigned long pulseEndTime = micros();

  if (digitalRead(ECHO_PIN) == HIGH) {
    // タイムアウト＝パルスが終わらなかった（範囲外）
    return -1.0;
  }

  // 4. パルス幅から距離(cm)を計算する
  //    音速をもとにした経験的な変換係数として 58.0 を使用
  unsigned long pulseWidthUs = pulseEndTime - pulseStartTime;
  float distanceCm = pulseWidthUs / 58.0;

  return distanceCm;
}
