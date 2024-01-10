#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
//#include <EEPROM.h>
//#include <ESP_EEPROM.h>

#define SS_PIN D8
#define RST_PIN D0
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

unsigned long previousMillis = 0;  // will store last time the RIFD was read
unsigned long currentMillis;
const long interval = 5000;  // interval at which to blink (milliseconds)

Servo myservo;

void setup() {
  Serial.begin(115200);  // Initiate a serial communication
  SPI.begin();         // Initiate  SPI bus
  mfrc522.PCD_Init();  // Initiate MFRC522
  Serial.println("Approximate your card to the reader...");
  Serial.println();

  //myservo.attach(8);  // attaches the servo on pin 9 to the servo object
  //move_servo(90);

  //pinMode(7, OUTPUT);
  //access_tone();
}

void loop() {





  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Only read the UID if it is over the interval length since it has last been read
  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    String active_UID = read_RFID();
    Serial.println(active_UID);
  }
}

String read_RFID() {
  previousMillis = currentMillis;

  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  return content.substring(1);  // return the UID
}

void move_servo(int degree) {
  myservo.write(degree);
}

void access_tone() {
  tone(7, 1046.5);
  delay(200);
  tone(7, 1174.66);
  delay(200);
  tone(7, 1318.51);
  delay(200);
  noTone(7);
  delay(800);
  tone(7, 1396.91);
  delay(300);
  noTone(7);
}

void no_access_tone() {
  tone(7, 932.33);
  delay(500);
  tone(7, 880);
  delay(500);
  tone(7, 830.61);
  delay(800);
  noTone(7);
}