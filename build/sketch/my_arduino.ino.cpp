#include <Arduino.h>
#line 1 "C:\\Users\\Administrator\\Desktop\\my_arduino\\my_arduino.ino"
#include <Wire.h>

#line 3 "C:\\Users\\Administrator\\Desktop\\my_arduino\\my_arduino.ino"
void setup();
#line 39 "C:\\Users\\Administrator\\Desktop\\my_arduino\\my_arduino.ino"
void loop();
#line 3 "C:\\Users\\Administrator\\Desktop\\my_arduino\\my_arduino.ino"
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\nScanning I2C devices on GPIO4 (SDA) and GPIO5 (SCL)...");

  Wire.begin(4, 5); // SDA, SCL

  byte error, address;
  int nDevices = 0;

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("✅ I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println();
      nDevices++;
    } else if (error == 4) {
      Serial.print("❌ Unknown error at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }

    delay(5);  // เพิ่ม delay ให้สแกนช้าๆ
  }

  if (nDevices == 0)
    Serial.println("❌ No I2C devices found.");
  else
    Serial.println("✅ Scan complete.");
}

void loop() {}

