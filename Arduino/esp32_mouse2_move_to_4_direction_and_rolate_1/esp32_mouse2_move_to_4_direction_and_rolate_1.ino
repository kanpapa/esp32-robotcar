//-----------------------------------------------------
// esp32_mouse2_move_to_4_direction_and_rolate_1.ino
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

// ★★★ 修正箇所 1: 動作ステータスを保持するグローバル変数の追加 ★★★
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

void setup()
{
  Serial.begin(9600);

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
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  display.setTextSize(1);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);      // Draw white text
  display.setCursor(0,0);              // Start at top-left corner
  display.println(F("Hello, world!"));

  display.display();
  delay(2000); // Pause for 2 seconds
  
  analogWrite(LP,0);
  analogWrite(LM,0);
  analogWrite(RP,0);
  analogWrite(RM,0);
  
  current_movement = "STOPPED"; // 初期状態をSTOPPEDに設定

  // flip the pin every 0.3s
  flipper.attach(0.3, flip);
}

void loop() //RUN ZIGZAG
{
  TTL();
  delay(1000);
  TTR();
  delay(1000);
  FWD();
  delay(1000);
  BAK();
  delay(1000);
  RTL();
  delay(1000);
  RTR();
  delay(1000);
  STOP();
  delay(1000);
}

void flip() {
  cm = dest();
  // ディスプレイをクリア
  display.clearDisplay();
  
  // 1. Distance Label
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Distance:");

  // 2. Distance Value and Bar
  display.setTextSize(2); // 少し大きなフォント
  display.setCursor(0, 15);

  if (cm == -1.0) {
    display.println("Out of Range");
  } else {
    display.print(cm);
    display.println(" cm");
    
    // 距離に基づいて四角形（バー）を描画する例
    int bar_width = map(cm, 0, 200, 0, SCREEN_WIDTH);
    if (bar_width > SCREEN_WIDTH) bar_width = SCREEN_WIDTH; 
    if (bar_width < 0) bar_width = 0; 

    display.fillRect(0, 40, bar_width, 5, SSD1306_WHITE); // プログレスバー
  }

  // ★★★ 修正箇所 3: 動作ステータスの表示 ★★★
  display.setTextSize(1);
  display.setCursor(0, 52); 
  display.print("STATUS: ");
  display.println(current_movement); 
  
  display.display();
}

// ★★★ 修正箇所 2: 各動作関数で current_movement を更新 ★★★

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

////HC-SR04_UltrasocicSensorExample.inoからの引用. 元々はvoid loop().
float dest() {

  unsigned long t1;
  unsigned long t2;
  unsigned long pulse_width;
  float current_cm;
  // float inches;

  // Hold the trigger pin high for at least 10 us
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Wait for pulse on echo pin
  while ( digitalRead(ECHO_PIN) == 0 );

  // Measure how long the echo pin was held high (pulse width)
  // Note: the micros() counter will overflow after ~70 min
  t1 = micros();
  while ( digitalRead(ECHO_PIN) == 1);
  t2 = micros();
  pulse_width = t2 - t1;

  // Calculate distance in centimeters and inches. The constants
  // are found in the datasheet, and calculated from the assumed speed
  //of sound in air at sea level (~340 m/s).
  current_cm = pulse_width / 58.0;
  return(current_cm);
}