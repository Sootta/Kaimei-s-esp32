#include <Arduino.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <ESP32Servo.h>

const char ssid[] = "KAIMEI'S-ESP32AP";
const char pass[] = "12345678";       // パスワードは8文字以上
const IPAddress ip(192,168,123,45);
const IPAddress subnet(255,255,255,0);
AsyncWebServer server(80);            // ポート設定

// Jsonオブジェクトの初期化
StaticJsonDocument<512> doc;

//ハードの設定----------------------------------------------------
const int FIRST_DIR_PIN = 23;
const int FIRST_PWM_PIN = 0;
const int SECOND_DIR_PIN = 24;
const int SECOND_PWM_PIN = 2;
const int THIRD_DIR_PIN = 25;
const int THIRD_PWM_PIN = 4;

const int FIRST_CHANNEL = 4;
const int SECOND_CHANNEL = 5;
const int THIRD_CHANNEL = 6;

int joystick_x;
int joystick_y;
double sarvo_x;
double sarvo_y;

Servo upservo;
Servo downservo;

const int UP_SERVO_PIN  = 8;
const int DOWN_SERVO_PIN = 9;

int up_servo_deg = 90;
int down_servo_deg = 90;
//----------------------------------------------------

void setup()
{
  Serial.begin(115200);


  // SPIFFSのセットアップ
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  WiFi.softAP(ssid, pass);           // SSIDとパスの設定
  delay(100);                        // このdelayを入れないと失敗する場合がある
  WiFi.softAPConfig(ip, ip, subnet); // IPアドレス、ゲートウェイ、サブネットマスクの設定
  
  IPAddress myIP = WiFi.softAPIP();  // WiFi.softAPIP()でWiFi起動

  // 各種情報を表示
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // GETリクエストに対するハンドラーを登録
  // rootにアクセスされた時のレスポンス
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });
  // style.cssにアクセスされた時のレスポンス
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // LED の制御変数の変更リクエスト
  server.on(
    "/post_test",
    HTTP_POST,
    [](AsyncWebServerRequest * request){},
    NULL,
    [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      String resjson = "";
 
      for (size_t i = 0; i < len; i++) {
        //Serial.write(data[i]);
        resjson.concat(char(data[i]));
      }
 
      //Serial.println(resjson);
      DeserializationError error = deserializeJson(doc, resjson);

      if(error){
        Serial.println("deserializeJson() faild");
        request->send(400);
      }
      else{
        joystick_x = doc["STICK_X"];       //zz Tilt Motor Default SPEED
        joystick_y = doc["STICK_Y"];
        sarvo_x = doc["SARVO_X"];
        sarvo_y = doc["SARVO_Y"];
        request->send(200);
      }
  });

  joystick_x = 0;
  joystick_y = 0;
  sarvo_x = 0;
  sarvo_y = 0;

  ledcSetup(FIRST_CHANNEL, 10000, 8);
  ledcSetup(SECOND_CHANNEL, 10000, 8);
  ledcSetup(THIRD_CHANNEL, 10000, 8);
  ledcAttachPin(FIRST_PWM_PIN, FIRST_CHANNEL);
  ledcAttachPin(SECOND_PWM_PIN, SECOND_CHANNEL);
  ledcAttachPin(THIRD_PWM_PIN, THIRD_CHANNEL);

  pinMode(FIRST_DIR_PIN, OUTPUT);
  pinMode(SECOND_DIR_PIN, OUTPUT);
  pinMode(THIRD_DIR_PIN, OUTPUT);

  upservo.attach(UP_SERVO_PIN);
  downservo.attach(DOWN_SERVO_PIN);

  upservo.write(up_servo_deg);
  downservo.write(down_servo_deg);

  // サーバースタート
  server.begin();

  Serial.println("Server start!");
}

void loop() {

  int vector[] = {0,0,0};
  vector[0] = joystick_x * 1;
  vector[1] = joystick_x * -1.0/2 + joystick_y * sqrt(3)/2;
  vector[2] = joystick_x * -1.0/2 - joystick_y * sqrt(3)/2;

  up_servo_deg = map(sarvo_x, 0, 1, 90, 180);
  down_servo_deg = 90 - map(sarvo_y, 0, 1, 0, 90);

  upservo.write(up_servo_deg);
  downservo.write(down_servo_deg);

  if((vector[0] + vector[1] + vector[2]) / 3 <= 40) {
    digitalWrite(FIRST_DIR_PIN, vector[0] > 0 ? LOW:HIGH);
    ledcWrite(FIRST_CHANNEL, abs(vector[0]));
    digitalWrite(SECOND_DIR_PIN, vector[1] > 0 ? LOW:HIGH);
    ledcWrite(SECOND_CHANNEL, abs(vector[1]));
    digitalWrite(THIRD_DIR_PIN, vector[2] > 0 ? LOW:HIGH);
    ledcWrite(THIRD_CHANNEL, abs(vector[2]));
  }
  /*
  Serial.print(sarvo_x * 100);
  Serial.print(" ");
  Serial.print(sarvo_y * 100);
  Serial.print(" ");
  Serial.print(joystick_x);
  Serial.print(" ");
  Serial.println(joystick_y);
  delay(200);
  */
}