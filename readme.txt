เวลารัน
คอมไพล์โค้ดก่อน >> เเล้วค่อยอัปโหลด


คำสั่งคอมไพล์ สำหรับ UNO
C:\Users\Administrator\Desktop\arduino-cli.exe compile -b arduino:avr:uno C:\Users\Administrator\Desktop\my_arduino/my_arduino.ino

คำสั่งอัปโหลด สำหรับ UNO
C:\Users\Administrator\Desktop\arduino-cli.exe upload -b arduino:avr:uno -p COM6 C:\Users\Administrator\Desktop\my_arduino/my_arduino.ino

คำสั่งรวม UNO
C:\Users\Administrator\Desktop\arduino-cli.exe compile --upload -b arduino:avr:uno -p COM6 C:\Users\Administrator\Desktop\my_arduino/my_arduino.ino


=================================================================================


คำสั่งคอมไพล์ สำหรับ D1
C:\Users\Administrator\Desktop\arduino-cli.exe compile -b esp8266:esp8266:d1_mini C:\Users\Administrator\Desktop\my_arduino/my_arduino.ino

คำสั่งอัปโหลด สำหรับ D1
C:\Users\Administrator\Desktop\arduino-cli.exe upload -b esp8266:esp8266:d1_mini -p COM6 C:\Users\Administrator\Desktop\my_arduino

คำสั่งรวม D1
C:\Users\Administrator\Desktop\arduino-cli.exe compile --upload -b esp8266:esp8266:d1_mini -p COM6 C:\Users\Administrator\Desktop\my_arduino --verbose
