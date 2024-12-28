#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 21
#define SS_PIN 18

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  
  Serial.println("Place your card near the reader...");
  Serial.println();
}

void loop() {
  // Kiểm tra xem có thẻ mới hay không
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Đọc dữ liệu từ thẻ
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Tạo chuỗi UID từ các byte
  String UID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    UID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""); // Thêm "0" nếu giá trị nhỏ hơn 0x10
    UID += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) {
      UID += " "; // Thêm khoảng trắng giữa các byte
    }
  }

  // In UID ra Serial Monitor
  UID.toUpperCase(); // Chuyển thành chữ hoa
  Serial.print("Card UID: ");
  Serial.println(UID);

  // Dừng xử lý cho thẻ hiện tại
  mfrc522.PICC_HaltA();

  delay(1000); // Thêm thời gian để người dùng dễ đọc
}
