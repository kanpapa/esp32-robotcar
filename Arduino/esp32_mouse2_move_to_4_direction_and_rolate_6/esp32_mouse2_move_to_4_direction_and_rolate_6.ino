//-----------------------------------------------------
// esp32_mouse2_move_to_4_direction_and_rolate.ino
// 
// for XIAO ESP32C6
//
// 2025/09/28 - Maze Solving with Right-Hand Rule
//-----------------------------------------------------

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Ticker.h>
Ticker flipper;
float cm = 0;

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
  
  current_movement = "STOPPED"; 

  // flip the pin every 0.3s
  flipper.attach(0.3, flip);
}

void loop()
{
  // ★★★ 迷路脱出アルゴリズムの実装 ★★★

  // 1. 前方の距離を測定
  float distance = dest(); 
  
  // 動作開始前の状態表示
  STOP(); 
  delay(100); 

  if (distance > WALL_THRESHOLD_CM) {
    // 【直進可能】前進する
    
    // 画面表示を更新
    current_movement = "MAZE: FORWARD"; 
    
    // 実際に直進
    FWD(); 
    delay(FORWARD_DELAY); // 2000ms 直進
  } else {
    // 【前方に壁あり】 右手法に従い右回転を試みる
    
    // 画面表示を更新
    current_movement = "MAZE: TURN RIGHT"; 
    
    // 実際に右回転
    RTR(); 
    delay(ROTATE_90_DELAY); // 600ms 右回転
  }
  
  // 動作を終えたら一時停止
  STOP();
  delay(100); // クールダウン
}


void flip() {
  // ★★★ 距離測定を復活 ★★★
  cm = dest();
  
  display.clearDisplay();
  
  // 1. 距離表示 (小さく)
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Dist: ");
  if (cm > 200) {
      display.println("MAX");
  } else {
      display.print(cm);
      display.println(" cm");
  }

  // 2. 動作ステータスを大きく中央に表示
  display.setTextSize(2); // サイズを2に変更して、長い文字列でも表示しやすくする
  
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(current_movement, 0, 0, &x1, &y1, &w, &h);
  
  // 画面中央下部に表示
  int x = (SCREEN_WIDTH - w) / 2;
  int y = 30; // 画面の中央やや下に設定
  
  display.setCursor(x, y); 
  display.println(current_movement); 
  
  display.display();
}

// モーター制御関数 (変更なし)

void FWD(void) // forward
{
  analogWrite(LP,250);
  analogWrite(LM,0);
  analogWrite(RP,250);
  analogWrite(RM,0);
  // current_movement = "FORWARD"; // loop()内で設定するためコメントアウト
}

void BAK(void) // back
{
  analogWrite(LP,0);
  analogWrite(LM,250);
  analogWrite(RP,0);
  analogWrite(RM,250);
  current_movement = "BACKWARD";
}

void TTL(void) // trun to left
{
  analogWrite(LP,100);
  analogWrite(LM,0);
  analogWrite(RP,250);
  analogWrite(RM,0);
  current_movement = "TURN LEFT";
}

void TTR(void) // trun to right
{
  analogWrite(LP,250);
  analogWrite(LM,0);
  analogWrite(RP,100);
  analogWrite(RM,0);
  current_movement = "TURN RIGHT";
}

void RTR(void) // rotate to right
{
  analogWrite(LP,250);
  analogWrite(LM,0);
  analogWrite(RP,0);
  analogWrite(RM,250);
  // current_movement = "ROTATE RIGHT"; // loop()内で設定するためコメントアウト
}

void RTL(void) // rotate to left
{
  analogWrite(LP,0);
  analogWrite(LM,250);
  analogWrite(RP,250);
  analogWrite(RM,0);
  current_movement = "ROTATE LEFT";
}

void STOP(void) // stop
{
  analogWrite(LP,0);
  analogWrite(LM,0);
  analogWrite(RP,0);
  analogWrite(RM,0);
  // current_movement = "STOPPED"; // loop()内で設定するためコメントアウト
}

// 距離測定関数 (復活)
float dest() {
  unsigned long t1;
  unsigned long t2;
  unsigned long pulse_width;
  float current_cm;
  
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  while ( digitalRead(ECHO_PIN) == 0 );

  t1 = micros();
  while ( digitalRead(ECHO_PIN) == 1);
  t2 = micros();
  pulse_width = t2 - t1;

  current_cm = pulse_width / 58.0;
  return(current_cm);
}