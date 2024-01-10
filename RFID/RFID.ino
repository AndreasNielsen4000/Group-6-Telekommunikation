#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
//#include <EEPROM.h>
#include <ESP_EEPROM.h>

#define SS_PIN D8
#define RST_PIN D0
#define buzzer D1
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

unsigned long previousMillis = 0;  // will store last time the RIFD was read
unsigned long currentMillis;
const long interval = 5000;  // interval at which to blink (milliseconds)
String UID[4];

Servo myservo;

void setup() {
  Serial.begin(115200);  // Initiate a serial communication
  EEPROM.begin(5 * 7);   // Initiate EEPROM memory size
  SPI.begin();           // Initiate  SPI bus
  mfrc522.PCD_Init();    // Initiate MFRC522
  Serial.println("Approximate your card to the reader...");
  Serial.println();

  myservo.attach(D4);  // attaches the servo on pin 9 to the servo object
  move_servo(90);

  pinMode(buzzer, OUTPUT);
  //access_tone();

  //UID[1] = "50 48 B5 1E";
  //UID[2] = "0A 59 91 17";
  //UID[3] = "84 93 E4 52";

  //put_memory(UID);
  //EEPROM.put(0,UID[1]);
  EEPROM.get(0,UID[1]);
  Serial.println(UID[1]);
  //read_memory();
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
    bool access_granted = false;

    for (int i = 0; i < 4; i++) {
      if (UID[i] == read_RFID()) {
        access_tone();
        access_granted = true;
      }

      Serial.println(UID[i]);
    }

    if (access_granted == false) {
      no_access_tone();
    }

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

String *read_memory() {
  static String memory[4];
  for (int i = 0; i < 4; i++) {
    EEPROM.get(i * 7, memory[i]);
  }
  return memory;
}

void put_memory(String memory[]) {
  for (int i = 0; i < 4; i++) {
    EEPROM.put(i * 7, memory[i]);
    Serial.println(memory[i]);
  }
  bool ok1 = EEPROM.commit();
}

void move_servo(int degree) {
  myservo.write(degree);
}

void access_tone() {
  tone(buzzer, 1046.5);
  delay(200);
  tone(buzzer, 1174.66);
  delay(200);
  tone(buzzer, 1318.51);
  delay(200);
  noTone(buzzer);
  delay(800);
  tone(buzzer, 1396.91);
  delay(300);
  noTone(buzzer);
}

void no_access_tone() {
  tone(buzzer, 932.33);
  delay(500);
  tone(buzzer, 880);
  delay(500);
  tone(buzzer, 830.61);
  delay(800);
  noTone(buzzer);
}