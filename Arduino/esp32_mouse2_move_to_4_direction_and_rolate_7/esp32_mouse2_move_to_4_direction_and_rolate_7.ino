//-----------------------------------------------------
// esp32_mouse2_move_to_4_direction_and_rolate.ino
// 
// for XIAO ESP32C6
//
// 2025/09/28 - Maze Solver with Non-Blocking Collision Detection
//-----------------------------------------------------

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Ticker.h>
Ticker flipper;
float cm = 0;

// ★★★ 状態管理用の列挙型と変数 ★★★
enum RobotState {
  STATE_STOPPED,
  STATE_FORWARD,
  STATE_TURNING_RIGHT
};
RobotState robot_state = STATE_STOPPED;

// ★★★ 動作時間の管理用変数 ★★★
unsigned long action_start_time = 0; 
String current_movement = "INITIALIZING";

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET      -1 
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pins
const int TRIG_PIN = 18;
const int ECHO_PIN = 20;

const unsigned int MAX_DIST = 23200;

int LP = 0; // AIN1   LEFT PLUS
int LM = 1; // AIN2   LEFT MINUS
int RP = 2; // BIN1   RIGHT MINUS
int RM = 21; // BIN2   RIGHT PLUS

// ★★★ 迷路脱出のための定数設定 ★★★
const int ROTATE_90_DELAY = 600; // 90度右回転時間 (ms)
const int FORWARD_DELAY = 2000;  // 直進時間 (ms)
const float WALL_THRESHOLD_CM = 15.0; // 前方の壁判断の閾値 (cm)

void setup()
{
  Serial.begin(99600);

  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); 
  }

  display.display();
  delay(1000); 

  display.clearDisplay();

  display.setTextSize(1);              
  display.setTextColor(SSD1306_WHITE);      
  display.setCursor(0,0);              
  display.println(F("Maze Solver Ready."));

  display.display();
  delay(1000); 
  
  analogWrite(LP,0);
  analogWrite(LM,0);
  analogWrite(RP,0);
  analogWrite(RM,0);
  
  // flip the pin every 0.3s
  flipper.attach(0.3, flip);
}

void loop()
{
  // ★★★ 常に距離を測定 ★★★
  float distance = dest(); 
  cm = distance; // flip()関数に値を渡す
  
  // ★★★ ステートマシン（非ブロッキングロジック） ★★★
  switch (robot_state) {
    case STATE_STOPPED:
      // 停止中: 次の動作を判断する
      if (distance > WALL_THRESHOLD_CM) {
        // 【壁なし】前進を開始
        FWD(); 
        action_start_time = millis();
        robot_state = STATE_FORWARD;
        current_movement = "MAZE: FORWARD";
      } else {
        // 【壁あり】右回転を開始 (右手法)
        RTR();
        action_start_time = millis();
        robot_state = STATE_TURNING_RIGHT;
        current_movement = "MAZE: TURN RIGHT";
      }
      break;

    case STATE_FORWARD:
      // 直進中: 常に衝突をチェック
      if (distance <= WALL_THRESHOLD_CM) {
        // ★★★ 衝突検知！即座に停止して再判断へ ★★★
        STOP(); 
        robot_state = STATE_STOPPED;
        // current_movement は次のループで更新される
      } else if (millis() - action_start_time >= FORWARD_DELAY) {
        // 規定の直進時間（2000ms）を完了。停止して再判断へ
        STOP(); 
        robot_state = STATE_STOPPED;
      }
      break;
      
    case STATE_TURNING_RIGHT:
      // 右回転中: 回転時間をチェック
      if (millis() - action_start_time >= ROTATE_90_DELAY) {
        // 回転時間（600ms）を完了。停止して再判断へ
        STOP(); 
        robot_state = STATE_STOPPED;
      }
      break;
  }
  
  // 処理速度が速すぎる場合の暴走を防ぐため、ごく短い遅延を入れる
  delay(10); 
}


void flip() {
  // Tickerで0.3秒ごとに実行される表示更新ロジック

  display.clearDisplay();
  
  // 1. 距離表示
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Dist: ");
  if (cm > 200 || cm == 0) {
      display.println("FAR/ERR");
  } else {
      display.print(cm);
      display.println(" cm");
  }

  // 2. 動作ステータスを大きく中央に表示
  display.setTextSize(2); 
  
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(current_movement, 0, 0, &x1, &y1, &w, &h);
  
  int x = (SCREEN_WIDTH - w) / 2;
  int y = 30; // 画面の中央やや下に設定
  
  display.setCursor(x, y); 
  display.println(current_movement); 
  
  display.display();
}

// モーター制御関数 (PWM設定のみを行い、制御を loop() に戻すため delay や状態遷移のロジックは含まない)

void FWD(void) // forward
{
  analogWrite(LP,250);
  analogWrite(LM,0);
  analogWrite(RP,250);
  analogWrite(RM,0);
}

void BAK(void) // back (未使用だが残す)
{
  analogWrite(LP,0);
  analogWrite(LM,250);
  analogWrite(RP,0);
  analogWrite(RM,250);
}

void TTL(void) // trun to left (未使用だが残す)
{
  analogWrite(LP,100);
  analogWrite(LM,0);
  analogWrite(RP,250);
  analogWrite(RM,0);
}

void TTR(void) // trun to right (未使用だが残す)
{
  analogWrite(LP,250);
  analogWrite(LM,0);
  analogWrite(RP,100);
  analogWrite(RM,0);
}

void RTR(void) // rotate to right
{
  analogWrite(LP,250);
  analogWrite(LM,0);
  analogWrite(RP,0);
  analogWrite(RM,250);
}

void RTL(void) // rotate to left (未使用だが残す)
{
  analogWrite(LP,0);
  analogWrite(LM,250);
  analogWrite(RP,250);
  analogWrite(RM,0);
}

void STOP(void) // stop
{
  analogWrite(LP,0);
  analogWrite(LM,0);
  analogWrite(RP,0);
  analogWrite(RM,0);
}

// 距離測定関数
float dest() {
  unsigned long t1;
  unsigned long t2;
  unsigned long pulse_width;
  float current_cm;
  
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long start_time = micros();
  // Wait for pulse on echo pin, with a timeout (MAX_DIST + safety margin)
  while ( digitalRead(ECHO_PIN) == 0 && (micros() - start_time) < (MAX_DIST * 1.5) );
  
  if(digitalRead(ECHO_PIN) == 0) {
      // タイムアウトした場合は、範囲外の値 (-1.0) を返す
      return -1.0; 
  }

  t1 = micros();
  start_time = micros();
  // Measure pulse width, with a timeout
  while ( digitalRead(ECHO_PIN) == 1 && (micros() - start_time) < (MAX_DIST * 1.5) );
  t2 = micros();
  
  if (digitalRead(ECHO_PIN) == 1) {
      // タイムアウトした場合は、範囲外の値 (-1.0) を返す
      return -1.0; 
  }
  
  pulse_width = t2 - t1;

  current_cm = pulse_width / 58.0;
  return(current_cm);
}