#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Hello!, Phongsakorn Phabjansing!!");
  delay(1000); // รอให้เห็นข้อความเริ่มต้นก่อนเลื่อน
}

void loop() {
  lcd.scrollDisplayLeft();  // เลื่อนข้อความไปทางซ้าย
  delay(300);               // เวลาหน่วงระหว่างการเลื่อน (ปรับได้)
}
