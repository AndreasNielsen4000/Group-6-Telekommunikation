#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
//#include <ESP_EEPROM.h>

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

unsigned long previousMillis = 0;  // will store last time the RIFD was read
unsigned long currentMillis;
const long interval = 5000;        // interval at which to blink (milliseconds)

void setup() {
  Serial.begin(9600);  // Initiate a serial communication
  SPI.begin();         // Initiate  SPI bus
  mfrc522.PCD_Init();  // Initiate MFRC522
  Serial.println("Approximate your card to the reader...");
  Serial.println();

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
  if (currentMillis - previousMillis >= interval) {
    String active_UID = read_RFID();
    Serial.println(active_UID);
  }
}

String read_RFID() {
  previousMillis = currentMillis;

  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  return content.substring(1);  // return the UID
}