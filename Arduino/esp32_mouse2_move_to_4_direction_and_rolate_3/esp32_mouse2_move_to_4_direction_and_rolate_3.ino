//-----------------------------------------------------
// esp32_mouse2_move_to_4_direction_and_rolate.ino
// 
// for XIAO ESP32C6
//
// 2025/09/28
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

#define OLED_RESET      -1 // Reset pin # (or -1 if sharing Arduino reset pin)
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

// ★★★ 回転時間を定義 (環境に応じて調整が必要な値) ★★★
const int ROTATE_90_DELAY = 1000; // 90度右回転に必要な遅延時間（ミリ秒）。1000msに仮設定。

void setup()
{
  Serial.begin(99600);

  // The Trigger pin will tell the sensor to range find
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);

  //Set Echo pin as input to measure the duration of 
  //pulses coming back from the distance sensor
  pinMode(ECHO_PIN, INPUT);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  display.display();
  delay(2000); 

  // Clear the buffer
  display.clearDisplay();

  display.setTextSize(1);              
  display.setTextColor(SSD1306_WHITE);      
  display.setCursor(0,0);              
  display.println(F("Starting 90-degree turns..."));

  display.display();
  delay(2000); 
  
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
  // ★★★ 修正箇所 1: 以前の ZIGZAG パターンをコメントアウト ★★★
  /*
  TTL();
  delay(5000);
  TTR();
  delay(5000);
  FWD();
  delay(5000);
  BAK();
  delay(5000);
  RTL();
  delay(5000);
  RTR();
  delay(5000);
  STOP();
  delay(5000);
  */
  
  // ★★★ 修正箇所 2: 右方向に90度回転を4回実行 ★★★
  
  for (int i = 0; i < 4; i++) {
    // 1. 右回転開始
    RTR(); 
    // 90度回転に必要な時間だけ待機 (RTR関数内で current_movement は "ROTATE RIGHT" に設定される)
    delay(ROTATE_90_DELAY); 
    
    // 2. 停止
    STOP();
    // 停止時間 (次の回転までの間隔)
    delay(1000); 
  }
  
  // 4回の回転が完了したら、ループを抜けるか、長時間停止する。
  // ここでは長時間停止し、プログラムを事実上終了させる。
  current_movement = "TASK COMPLETE";
  STOP();
  delay(60000); // 1分間停止
}


void flip() {
  // 距離測定コードはコメントアウトのまま

  display.clearDisplay();
  
  /* 距離表示コード (コメントアウト) */

  // 動作ステータスを最大の文字サイズで画面中央に表示
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3); 
  
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(current_movement, 0, 0, &x1, &y1, &w, &h);
  
  int x = (SCREEN_WIDTH - w) / 2;
  int y = (SCREEN_HEIGHT - h) / 2;
  
  display.setCursor(x, y); 
  display.println(current_movement); 
  
  display.display();
}

// モーター制御関数（current_movementの更新はそのまま）

void FWD(void) // forward
{
  analogWrite(LP,250);
  analogWrite(LM,0);
  analogWrite(RP,250);
  analogWrite(RM,0);
  current_movement = "FORWARD";
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
  current_movement = "ROTATE RIGHT";
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
  current_movement = "STOPPED";
}

// 距離測定関数 (未使用だが残す)
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