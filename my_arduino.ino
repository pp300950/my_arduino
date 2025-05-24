#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Servo.h>

// LCD and RTC setup
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

const int MAX_DOSES = 6;
bool doseTaken[MAX_DOSES];
DateTime pillTimes[MAX_DOSES];
bool isPillTimeSet = false; // เพิ่มตัวแปรตรวจสอบว่าได้ตั้งเวลาเตือนแล้วหรือยัง
bool alertedDose[MAX_DOSES] = {false};

// servo
Servo myServo;

// Pins
const int buttonUpPin = 2;
const int buttonDownPin = 3;
const int trigPin = 4;
const int echoPin = 5;
const int buzzerPin = 6;
const int relayPin = 8; // relay มอเตอร์สั่น
const int LEDPin = 10; 

// Menu settings
int menuIndex = 0;
int dosePerDay = 1;
int reminderMode = 0; // 0: Once, 1: Mon-Fri, 2: Sat-Sun

// Reminder timing
DateTime firstPillTime;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
bool boxOpened = false;
// สำหรับตรวจสอบการทำงานของเซอร์โว
unsigned long boxCloseTime = 0;

void setup()
{
  digitalWrite(relayPin, HIGH); // ปิดรีเลย์
  myServo.attach(9);
  myServo.write(180); // 0-100:เปิด | 180:ล็อก
  delay(1000);
  myServo.detach(); // ปลดการเชื่อมต่อ

  Serial.begin(9600);
  Wire.begin();
  lcd.init();
  // lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.backlight();

  pinMode(buttonUpPin, INPUT_PULLUP);
  pinMode(buttonDownPin, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  if (rtc.lostPower())
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  pinMode(relayPin, OUTPUT);

  lcd.setCursor(0, 0);
  lcd.print("Smart Pill Box");

  tone(buzzerPin, 1000, 100);
  delay(150);
  tone(buzzerPin, 1000, 100);
  delay(150);
  tone(buzzerPin, 2000, 50);
  delay(100);
  tone(buzzerPin, 1000, 150);
  delay(150);

  noTone(buzzerPin);

  delay(2000);
  lcd.clear();
}

void loop()
{
  handleMenu();
  if (isPillTimeSet)
  {
    checkReminder();
  }
}

// ========== MENU SETTING ==========
void manualSetPillTimes()
{
  DateTime now = rtc.now();
  for (int i = 0; i < dosePerDay; i++)
  {
    int hour = 0;
    int minute = 0;
    bool confirm = false;

    while (!confirm)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set Dose ");
      lcd.print(i + 1);
      lcd.print(":");

      lcd.setCursor(0, 1);
      lcd.print((hour < 10 ? "0" : ""));
      lcd.print(hour);
      lcd.print(":");
      lcd.print((minute < 10 ? "0" : ""));
      lcd.print(minute);

      lcd.print(" ");
      lcd.print(hour < 12 ? "AM" : "PM");

      bool buttonUp = digitalRead(buttonUpPin) == LOW;
      bool buttonDown = digitalRead(buttonDownPin) == LOW;

      static unsigned long lastChange = 0;
      if (millis() - lastChange > debounceDelay)
      {
        if (buttonUp)
        {
          tone(buzzerPin, 500, 100);
          hour = (hour + 1) % 24;
          lastChange = millis();
        }
        else if (buttonDown)
        {
          tone(buzzerPin, 500, 100);
          minute = (minute + 5) % 60;
          lastChange = millis();
        }
      }

      // กดยืนยันโดยกดค้างปุ่ม UP (นานกว่า 1 วินาที)
      if (digitalRead(buttonUpPin) == LOW)
      {
        unsigned long pressStart = millis();
        while (digitalRead(buttonUpPin) == LOW)
        {
          if (millis() - pressStart > 1000)
          {
            // ลบ 1 ชั่วโมงก่อนยืนยัน
            hour = (hour == 0) ? 23 : (hour - 1);
            confirm = true;
            tone(buzzerPin, 1000, 200);
            break;
          }
        }
      }

      isPillTimeSet = true;
      menuIndex = 3; // เด้งไปหน้า Waiting

      delay(200);
    }

    pillTimes[i] = DateTime(now.year(), now.month(), now.day(), hour, minute, 0);
    doseTaken[i] = false;

    Serial.print("Dose ");
    Serial.print(i + 1);
    Serial.print(" set to ");
    Serial.print(hour);
    Serial.print(":");
    Serial.println(minute);
  }

  // ยังไม่เปิดใช้งาน reminder จนกว่าจะยืนยันในเมนู
  isPillTimeSet = false;
}
void handleMenu()
{
  static bool lastButtonUp = HIGH;
  static bool lastButtonDown = HIGH;

  bool buttonUp = digitalRead(buttonUpPin);
  bool buttonDown = digitalRead(buttonDownPin);

  // ปุ่ม UP: เปลี่ยนเมนู
  if (buttonUp == LOW && lastButtonUp == HIGH && (millis() - lastDebounceTime) > debounceDelay)
  {
    menuIndex = (menuIndex + 1) % 4;
    lastDebounceTime = millis();
    tone(buzzerPin, 500, 100);
  }

  // ปุ่ม DOWN: ทำสิ่งที่เมนูนั้นสั่ง
  if (buttonDown == LOW && lastButtonDown == HIGH && (millis() - lastDebounceTime) > debounceDelay)
  {
    switch (menuIndex)
    {
    case 0:
      dosePerDay = (dosePerDay % 6) + 1;
      break;
    case 1:
      reminderMode = (reminderMode + 1) % 3;
      break;
    case 2:
      manualSetPillTimes();
      boxOpened = false;
      isPillTimeSet = true;
      break;
    case 3:
      // ไม่ทำอะไรที่นี่ แค่ให้ switch ด้านล่างแสดงผล
      break;
    }

    lastDebounceTime = millis();
    tone(buzzerPin, 500, 100);
  }

  lastButtonUp = buttonUp;
  lastButtonDown = buttonDown;

  // ==== ส่วนแสดงผลเมนู ====
  lcd.clear();
  lcd.setCursor(0, 0);
  switch (menuIndex)
  {
  case 0:
    lcd.print("Set Doses/Day");
    lcd.setCursor(0, 1);
    lcd.print("Doses: ");
    lcd.print(dosePerDay);
    break;
  case 1:
    lcd.print("Reminder Mode");
    lcd.setCursor(0, 1);
    lcd.print(reminderMode == 0 ? "Once" : (reminderMode == 1 ? "Mon-Fri" : "Sat-Sun"));
    break;
  case 2:
    lcd.print("Start Reminder");
    lcd.setCursor(0, 1);
    lcd.print("Starts in 1 min");
    break;
  case 3:
    DateTime now = rtc.now();
    lcd.setCursor(0, 0);

    // ค้นหาเวลายาถัดไป
    bool foundNext = false;
    char buffer[9] = "Tomorrow"; // ค่า default

    for (int i = 0; i < dosePerDay; i++)
    {
      if (pillTimes[i] > now)
      {
        sprintf(buffer, "%02d:%02d%s",
                pillTimes[i].hour() > 12 ? pillTimes[i].hour() - 12 : pillTimes[i].hour(),
                pillTimes[i].minute(),
                pillTimes[i].hour() >= 12 ? "PM" : "AM");
        foundNext = true;
        break;
      }
    }

    // แสดงบนบรรทัดแรก: Wait | เวลา
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wait | ");
    lcd.print(buffer); // แสดงเวลาถัดไปหรือตัวอักษร "Tomorrow"

    lcd.setCursor(0, 1);
    char nowBuffer[12]; // สำหรับเวลาในรูปแบบ HH:MM:SS
    sprintf(nowBuffer, "%02d:%02d:%02d%s",
            now.hour() > 12 ? now.hour() - 12 : now.hour(),
            now.minute(),
            now.second(),
            now.hour() >= 12 ? "PM" : "AM");
    lcd.print(nowBuffer);
    break;
  }

  // ตรวจจับการกดปุ่มล่างค้างเกิน 2 วินาทีเพื่อรีเซต
  if (digitalRead(buttonDownPin) == LOW)
  {
    unsigned long pressStart = millis();

    while (digitalRead(buttonDownPin) == LOW)
    {
      if (millis() - pressStart > 2000)
      {
        // แสดงข้อความก่อนรีสตาร์ท
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("System Restarting");
        delay(1000);

        // รีสตาร์ทระบบ
        asm volatile("  jmp 0");
      }
    }
  }

  delay(200); // ป้องกันการกระพริบจอเร็วเกินไป
}

// ========== REMINDER LOGIC ==========
void checkReminder()
{
  if (!isPillTimeSet)
    return;

  DateTime now = rtc.now();
  float distance = measureDistance();

  for (int i = 0; i < dosePerDay; i++)
  {
    if (!doseTaken[i] && now >= pillTimes[i] && shouldAlertToday(now))
    {
      alertUser(i);
      alertedDose[i] = true;
    }

    if (distance > 13.0 && alertedDose[i] && !doseTaken[i])
    {
      doseTaken[i] = true;
      boxOpened = true;
      Serial.print("Dose ");
      Serial.print(i + 1);
      Serial.println(" taken");
      noTone(buzzerPin);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Pill Taken!");
      delay(2000);
      lcd.clear();
    }
  }
}

// ========== ALERT ==========
void alertUser(int doseIndex)
{

  static unsigned long lastBeepTime = 0;
  static int beepCount[MAX_DOSES] = {0};       // นับจำนวนครั้งที่แจ้งเตือน
  static bool isAlerting[MAX_DOSES] = {false}; // กำลังอยู่ในช่วงแจ้งเตือนไหม
  myServo.attach(9);
  myServo.write(100); // ปลดล็อค
  delay(100);

  if (!isAlerting[doseIndex])
  {
    isAlerting[doseIndex] = true;
    beepCount[doseIndex] = 0;
    alertedDose[doseIndex] = true;
    lastBeepTime = millis();
  }

  // ถ้ายังไม่เปิดกล่องและยังแจ้งเตือนไม่ครบ 5 ครั้ง
  if (!doseTaken[doseIndex] && beepCount[doseIndex] < 20)
  {
    if (millis() - lastBeepTime >= 1000) // ทุก 1 วินาที
    {
      tone(buzzerPin, 1000, 500);
      beepCount[doseIndex]++;
      lastBeepTime = millis();
      digitalWrite(relayPin, LOW); // เปิดรีเลย์
      digitalWrite(LEDPin, HIGH); // เปิดไฟ
      delay(250);
      digitalWrite(LEDPin, LOW); // ปิดไฟ
      digitalWrite(relayPin, HIGH); // ปิดรีเลย์
    }
  }

  // ตรวจจับการเปิดกล่อง
  if (measureDistance() > 15.0 && !doseTaken[doseIndex])
  {
    doseTaken[doseIndex] = true;
    // Serial.print("Dose ");
    // Serial.print(doseIndex + 1);
    // Serial.println(" taken by box opening");
    delay(3000);
    myServo.attach(9);
    myServo.write(180); // ล้อคกล่อง
    delay(1000);
    myServo.detach();

    // เสียงเตือนเมื่อเปิดกล่อง
    tone(buzzerPin, 2000, 200);
    delay(250);
    tone(buzzerPin, 1500, 200);
    delay(250);
    noTone(buzzerPin);
    digitalWrite(LEDPin, HIGH); // เปิดไฟ

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pill Taken!");
    delay(2000);
    digitalWrite(LEDPin, LOW); // เปิดไฟ

    lcd.clear();
  }

  // หยุดแจ้งเตือนถ้าครบ 5 ครั้งแล้ว
  if (beepCount[doseIndex] >= 5)
  {
    noTone(buzzerPin);
  }
}

// ========== CHECK REMINDER TYPE ==========
bool shouldAlertToday(DateTime now)
{
  int dow = now.dayOfTheWeek(); // 0: Sunday, 1: Monday, ...
  if (reminderMode == 0)
    return true;
  else if (reminderMode == 1)
    return dow >= 1 && dow <= 5;
  else
    return dow == 0 || dow == 6;
}

// ========== MEASURE DISTANCE ==========
float measureDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = (duration * 0.0343) / 2;
  return distance;
}
