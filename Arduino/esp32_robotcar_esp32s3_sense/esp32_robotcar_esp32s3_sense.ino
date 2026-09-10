//-----------------------------------------------------
// esp32_robotcar_esp32s3_sense.ino
//
// ロボットカー制御: XIAO ESP32C3/C6
// AIセンサー：XIAO ESP32S3 Sense
//
// 概要:
//   ESP32S3 Senseのカメラでジェスチャーごとに走行、停止、右回転するロボットのプログラムです。
//   OLEDディスプレイにジェスチャーの認識結果とモーターの動作状態を表示します。
//
//-----------------------------------------------------

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Seeed_Arduino_SSCMA.h>

#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
SSCMA AI;

// モータードライバのピン(DRV8835)
#define AIN1 D0
#define AIN2 D1
#define BIN1 D2
#define BIN2 D3

#define GESTURE_PAPER   0
#define GESTURE_ROCK    1
#define GESTURE_SCISSOR 2

const int BASE_SPEED = 250;
const int TURN_SPEED = 100;
const float CONF_THRESHOLD = 0.7;
const int DEBOUNCE_COUNT = 2;

int lastGesture = -1;
int stableCount = 0;
int displayedGesture = -999; // 表示済みと違う値で初期化

// ESP32S3 Senseのシリアル接続
#define AI_RX_PIN D7
#define AI_TX_PIN D6
HardwareSerial AISerial(1);

void showDebug(const char* status, int gesture, float score) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(status);

  display.print("Gesture: ");
  switch (gesture) {
    case GESTURE_PAPER:   display.println("PAPER(stop)");  break;
    case GESTURE_ROCK:    display.println("ROCK(right)");  break;
    case GESTURE_SCISSOR: display.println("SCISSOR(fwd)"); break;
    default:              display.println("none");         break;
  }

  display.print("Score: ");
  display.println(score, 2);
  display.display();
}

void setup() {
  Serial.begin(115200);

  // OLED初期化(Vision AI V2より先でも後でもOK、アドレスが違うので競合しない)
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed");
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Booting...");
  display.display();

  // for ESP32S3 Sense
  delay(3000); // ESP32S3 Senseのブート待ち（カメラ+モデルロード完了を待つ）

  Serial.println("Connecting to ESP32S3 Sense...");
  display.println("Connecting to ESP32S3 Sense...");
  display.display();

  AISerial.begin(921600, SERIAL_8N1, AI_RX_PIN, AI_TX_PIN);

  bool connected = false;
  for (int i = 0; i < 10 && !connected; i++) {
    connected = AI.begin(&AISerial);
    if (!connected) { 
      delay(500);
      display.print(i);
      display.print(".");
      display.display();
    }
  }

  if (connected) {
    Serial.println("ESP32S3 Sense connected!");
    showDebug("ESP32S3 Sense: OK", -1, 0);
  } else {
    Serial.println("ESP32S3 Sense connection failed!");
    showDebug("ESP32S3 Sense: FAILED", -1, 0);
    while (true) { // 無限ループで停止
      delay(1000);
    }
  }

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
}

void stopCar() {
  analogWrite(AIN1, 0); analogWrite(AIN2, 0);
  analogWrite(BIN1, 0); analogWrite(BIN2, 0);
}

void goForward() {
  analogWrite(AIN1, BASE_SPEED); analogWrite(AIN2, 0);
  analogWrite(BIN1, BASE_SPEED); analogWrite(BIN2, 0);
}

void turnRight() {
  analogWrite(AIN1, BASE_SPEED); analogWrite(AIN2, 0);
  analogWrite(BIN1, TURN_SPEED); analogWrite(BIN2, 0);
}

int detectGesture(float &outScore) {
  int bestTarget = -1;
  float bestScore = 0;
  for (int i = 0; i < AI.boxes().size(); i++) {
    if (AI.boxes()[i].score > bestScore) {
      bestScore = AI.boxes()[i].score;
      bestTarget = AI.boxes()[i].target;
    }
  }
  outScore = bestScore;
  if (bestScore < CONF_THRESHOLD) return -1;
  return bestTarget;
}

void loop() {
  int ret = AI.invoke(1, false, false); // 1回推論リクエスト→結果待ち
  Serial.print("invoke ret=");
  Serial.println(ret);
  
  if (ret) {
    Serial.print("boxes=");
    Serial.print(AI.boxes().size());
    Serial.print(" classes=");
    Serial.print(AI.classes().size());
    Serial.print(" perf(infer)=");
    Serial.println(AI.perf().inference);

    for (int i = 0; i < AI.classes().size(); i++) {
      Serial.print("class[");
      Serial.print(i);
      Serial.print("] target=");
      Serial.print(AI.classes()[i].target);
      Serial.print(" score=");
      Serial.println(AI.classes()[i].score);
    }
    for (int i = 0; i < AI.boxes().size(); i++) {
      Serial.print("box[");
      Serial.print(i);
      Serial.print("] target=");
      Serial.print(AI.boxes()[i].target);
      Serial.print(" score=");
      Serial.println(AI.boxes()[i].score);
    }

    float score;
    int gesture = detectGesture(score);

    if (gesture == lastGesture) {
      stableCount++;
    } else {
      lastGesture = gesture;
      stableCount = 1;
    }

    if (stableCount >= DEBOUNCE_COUNT) {
      switch (gesture) {
        case GESTURE_PAPER:   stopCar();   break;
        case GESTURE_SCISSOR: goForward(); break;
        case GESTURE_ROCK:    turnRight(); break;
        default:              stopCar();   break;
      }

      // ★確定した時だけ表示更新(毎フレーム更新しない)
      if (gesture != displayedGesture) {
        showDebug("Running", gesture, score);
        displayedGesture = gesture;
      }
    }
  }
}