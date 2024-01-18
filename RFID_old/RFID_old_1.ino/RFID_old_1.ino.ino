#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <ESP_EEPROM.h>

#include "Hash.h"

#define SS_PIN D8
#define RST_PIN D0
#define buzzer D1

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.
Servo door_servo;

unsigned long previousMillis = 0;  // Will store last time the RIFD was read
unsigned long currentMillis;
const long interval = 5000;  // Interval at which to blink (milliseconds)
const int keycards = 5;
const int KEYCARD_LENGTH = 11;
String UID[keycards];  // Create placeholders for the RFID tags

void setup() {
  Serial.begin(115200);  // Initiate a serial communication

  EEPROM.begin(keycards * KEYCARD_LENGTH);  // Initiate EEPROM memory size
  SPI.begin();                              // Initiate  SPI bus
  mfrc522.PCD_Init();                       // Initiate MFRC522
  Serial.println("Approximate your card to the reader...");
  Serial.println();

  door_servo.attach(2, 500, 2400);  // Attaches the servo on pin D4 to the servo object
  pinMode(buzzer, OUTPUT);

  //UID[0] = "           ";
  //UID[1] = "50 48 B5 1E";
  //UID[2] = "0A 59 91 17";
  //UID[3] = "84 93 E4 52";
  //UID[4] = "           ";

  for (int i = 0; i < keycards; i++) {
    //manual_put_memory(UID[i], i);
  }

  // read the UID values from the memory
  for (int i = 0; i < keycards; i++) {
    UID[i] = read_memory(i);
    Serial.println(UID[i]);
  }

  //write_user_array_to_eeprom("17 48 43 4A");
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

    for (int i = 0; i < keycards; i++) {
      if (UID[i] == read_RFID()) {
        access_tone();
        access_granted = true;
      }
      Serial.println(UID[i]);
    }

    if (access_granted == false) {
      no_access_tone();
    }

    unsigned long active_UID = hashPassword(read_RFID().c_str());
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

String read_memory(int x) {
  String memory = "";
  for (int i = 0; i < KEYCARD_LENGTH; i++) {
    memory = String(memory + char(EEPROM.read(i + KEYCARD_LENGTH * x)));
  }
  return memory;
}

void manual_put_memory(String memory, int x) {
  for (int i = 0; i < memory.length(); ++i) {
    Serial.print(memory[i]);
    EEPROM.write(i + KEYCARD_LENGTH * x, memory[i]);
  }
  EEPROM.commit();
}

void write_user_array_to_eeprom(String memory) {
  // Checks if the new UID to be saved is allready saved
  for (int i = 0; i < keycards; i++) {
    if (read_memory(i) == memory) {
      return;
    }
  }
  for (int i = 0; i < keycards; i++) {
    if (read_memory(i) == "           ") {
      for (int x = 0; x < memory.length(); ++x) {
        Serial.print(memory[x]);
        EEPROM.write(x + i * 11, memory[x]);
      }
      EEPROM.commit();
      return;
    }
  }
}

void remove_memory(String memory) {
  for (int i = 0; i < keycards; i++) {
    if (read_memory(i) == memory) {
      for (int x = 0; x < memory.length(); ++x) {
        EEPROM.write(x + i * KEYCARD_LENGTH, 32);
      }
      EEPROM.commit();
    }
  }
}

void reset_memory() {
  for (int i = 0; i < keycards; i++) {
    for (int x = 0; x < KEYCARD_LENGTH; ++x) {
      EEPROM.write(x + i * KEYCARD_LENGTH, 32);
    }
  }
  EEPROM.commit();
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
  open_door();
  delay(300);
  noTone(buzzer);
  delay(9700);
  close_door();
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

void open_door() {
  door_servo.write(0);
}

void close_door() {
  door_servo.write(90);
}