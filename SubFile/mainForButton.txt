#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

// กำหนด LCD และ RTC
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;  //ยังไม่มี

// พินต่างๆ
const int buttonUpPin = 2;
const int buttonDownPin = 3;
const int trigPin = 4;
const int echoPin = 5;
const int buzzerPin = 6;

// การตั้งค่าเมนู
int menuIndex = 0;
int dosePerDay = 1;
int reminderMode = 0; // 0: ครั้งเดียว, 1: จ-ศ, 2: ส-อา

// เวลาแจ้งเตือน
DateTime firstPillTime;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
bool boxOpened = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  lcd.begin();
  lcd.backlight();

  pinMode(buttonUpPin, INPUT_PULLUP);
  pinMode(buttonDownPin, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.setCursor(0, 0);
  lcd.print("Smart Pill Box");
  delay(2000);
  lcd.clear();
}

void loop() {
  handleMenu();
  checkReminder();
}

// ========== MENU SETTING ==========
void handleMenu() {
  static bool lastButtonUp = HIGH;
  static bool lastButtonDown = HIGH;

  bool buttonUp = digitalRead(buttonUpPin);
  bool buttonDown = digitalRead(buttonDownPin);

  if (buttonUp == LOW && lastButtonUp == HIGH && (millis() - lastDebounceTime) > debounceDelay) {
    menuIndex++;
    if (menuIndex > 3) menuIndex = 0;
    lastDebounceTime = millis();
  }

  if (buttonDown == LOW && lastButtonDown == HIGH && (millis() - lastDebounceTime) > debounceDelay) {
    switch (menuIndex) {
      case 0: dosePerDay++; if (dosePerDay > 6) dosePerDay = 1; break;
      case 1: reminderMode = (reminderMode + 1) % 3; break;
      case 2: firstPillTime = rtc.now() + TimeSpan(0, 0, 1, 0); break;
    }
    lastDebounceTime = millis();
  }

  lastButtonUp = buttonUp;
  lastButtonDown = buttonDown;

  lcd.setCursor(0, 0);
  lcd.print("เมนู: ");
  lcd.print(menuIndex);
  lcd.setCursor(0, 1);

  Serial.print("Menu Index: ");
  Serial.println(menuIndex);

  switch (menuIndex) {
    case 0:
      lcd.print("ยาต่อวัน: ");
      lcd.print(dosePerDay);
      Serial.print("Dose/Day: ");
      Serial.println(dosePerDay);
      break;
    case 1:
      lcd.print("โหมดเตือน: ");
      if (reminderMode == 0) lcd.print("ครั้งเดียว");
      else if (reminderMode == 1) lcd.print("จ-ศ");
      else lcd.print("ส-อา");
      Serial.print("Reminder Mode: ");
      Serial.println(reminderMode);
      break;
    case 2:
      lcd.print("เริ่มเตือนใน 1นาที");
      Serial.println("First pill time set to 1 min later");
      break;
    case 3:
      lcd.print("รอเวลาเตือน...");
      break;
  }

  delay(200);
}

// ========== REMINDER LOGIC ==========
void checkReminder() {
  DateTime now = rtc.now();

  // เงื่อนไขเตือน
  if (now >= firstPillTime && shouldAlertToday(now) && !boxOpened) {
    alertUser();
  }

  // ตรวจสอบว่าผู้ใช้เปิดกล่องหรือยัง
  if (measureDistance() > 12.0) {
    boxOpened = true;
    noTone(buzzerPin);
    Serial.println("กล่องถูกเปิดแล้ว");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("กินยาแล้ว!");
    delay(2000);
    lcd.clear();
  }
}

// ========== ALERT ==========
void alertUser() {
  tone(buzzerPin, 1000);
  delay(500);
  noTone(buzzerPin);
  delay(300);
  Serial.println("เตือนให้กินยา!");
}

// ========== CHECK REMINDER TYPE ==========
bool shouldAlertToday(DateTime now) {
  int dow = now.dayOfTheWeek(); // 0: Sunday, 1: Monday, ...
  if (reminderMode == 0) return true;
  else if (reminderMode == 1) return dow >= 1 && dow <= 5;
  else return dow == 0 || dow == 6;
}

// ========== MEASURE DISTANCE ==========
float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = (duration * 0.0343) / 2;
  return distance;
}
