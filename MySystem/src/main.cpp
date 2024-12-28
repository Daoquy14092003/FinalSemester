#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Gau In T3";
const char* password = "unin2611";
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;

#define SS_PIN 18
#define RST_PIN 34
#define BUZZER_PIN 5
#define SERVO_PIN 13
#define PIR_PIN 19 


String UID = "15 75 FE 28"; // UID thẻ RFIDRFID
byte lock = 0; // Trạng thái khóa
Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);
WiFiClient espClient;
PubSubClient client(espClient);

void setupWiFi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Received: " + message);
  
  if (String(topic) == "esp32/servo/control") {
    if (message == "LOCK") {
      lock = 1;
      servo.write(50);
      client.publish("esp32/servo/status", "Locked");
    } else if (message == "UNLOCK") {
      lock = 0;
      servo.write(110);
      client.publish("esp32/servo/status", "Unlocked");
    }
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected!");
      client.subscribe("esp32/servo/control");
    } else {
      Serial.print("Failed. Error: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  servo.attach(SERVO_PIN);
  servo.write(70);
  lcd.init();
  lcd.backlight();
  SPI.begin();
  rfid.PCD_Init();
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT); 
  setupWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  if (digitalRead(PIR_PIN) == HIGH) {
    // bật buzzer nếu có xâm nhập
    digitalWrite(BUZZER_PIN, HIGH);
    client.publish("esp32/motion/warning", "Motion detected");
    delay(500); 
  } else {
    // tắt buzzer nếu không có xâm nhập
    digitalWrite(BUZZER_PIN, LOW);
  }

  lcd.setCursor(4, 0);
  lcd.print("Welcome!");
  lcd.setCursor(1, 1);
  lcd.print("Put your card");

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning");
  String ID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    lcd.print(".");
    ID.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
    ID.concat(String(rfid.uid.uidByte[i], HEX));
    delay(300);
  }
  ID.toUpperCase();
  Serial.println("Card ID: " + ID);

  if (ID.substring(1) == UID) {
    // quét thẻ hợp lệ, tắt buzzer, mở khóa
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(BUZZER_PIN, LOW);
    if (lock == 0) {
      lock = 1;
      servo.write(50);
      client.publish("esp32/rfid/log", "Locked");
    } else {
      lock = 0;
      servo.write(110);
      client.publish("esp32/rfid/log", "Unlocked");
    }
    client.publish("esp32/rfid/uid", ID.c_str());
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wrong card!");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
    client.publish("esp32/rfid/log", "Invalid Card");
  }
}
