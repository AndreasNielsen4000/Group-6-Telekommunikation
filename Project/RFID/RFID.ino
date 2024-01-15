#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <ESP_EEPROM.h>

#include <apicaller.h>
//#include "../Libraries/api_caller/api_caller.h" // TODO: check this works
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
//#include "../Libraries/Hash.h"
#include <Hash.h>

#define SS_PIN D8
#define RST_PIN D0
#define buzzer D1
#define NUM_USERS 5

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.
Servo myservo;

const char *ssid = "";   /* Your SSID */
const char *pass = "";   /* Your Password */
const char *apiUrl = ""; /* Your API URL. Example: "https://api.example.com" */

unsigned long previous_RFID_Millis = 0;  // Will store last time the RFID was read
unsigned long previous_WIFI_Millis = 0;  // Will store last time the WIFI was updated
unsigned long currentMillis;
const long RFID_interval = 5000;  // Interval of RFID read
const long WIFI_interval = 1000;  // Interval of WIFI read
const int keycards = NUM_USERS; // TODO: maybe change this
const int keycard_length = 11;
//String UID[keycards];  // Create placeholders for the RFID tags

char UID[NUM_USERS][11] = {'\0'};

// Struct for stroing a user in memory (the pin and rfid is hashed)
struct User {
  unsigned long pin;
  unsigned long rfid;
};

// The user store
User users[NUM_USERS] = {
  { 0, 0 },
  { 0, 0 },
  { 0, 0 },
  { 0, 0 },
  { 0, 0 }
};


void setup() {
  Serial.begin(115200);  // Initiate a serial communication

  WiFi.begin(ssid, pass);
  // Waiting for WIFI connection
  Serial.println("WIFI connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\n\nConnected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  EEPROM.begin(keycards * keycard_length);  // Initiate EEPROM memory size
  SPI.begin();                              // Initiate  SPI bus
  mfrc522.PCD_Init();                       // Initiate MFRC522
  Serial.println("Approximate your card to the reader...");
  Serial.println();

  myservo.attach(2, 500, 2400);  // Attaches the servo on pin D4 to the servo object
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

  //put_memory("17 48 43 4A");
}

void loop() {
  while (WiFi.status() != WL_CONNECTED) {
    //Serial.println("Not connected to WiFi...");
    //delay(1000);  // remove delay

    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent()) {
      return;
    }
    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    // Only read the UID if it is over the interval length since it has last been read
    // TODO: this should hash input and compare to hashed values in memory
    // TODO: This should also have pin 
    currentMillis = millis();
    if (currentMillis - previous_RFID_Millis > RFID_interval) {
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
    }
  }

  currentMillis = millis();

  if (currentMillis - previous_WIFI_Millis > WIFI_interval) {
    previous_WIFI_Millis = currentMillis;

    // Create a WiFiClient object to talk to the server.
    WiFiClient client;

    // Create an ApiCaller object with the WiFiClient and API URL.
    ApiCaller apiCaller(client, apiUrl);

    // Call the API with a pin
    String UID = read_RFID();
    std::unique_ptr<DynamicJsonDocument> doc = apiCaller.get("/rfid", UID);

    // Check if the API call was successful.
    if (doc == nullptr) {
      Serial.println("Error calling API");
      return;
    } else if (doc->containsKey("authenticated")) {
      if (doc->get<bool>("authenticated")) {
        Serial.println("API call was successful and the pin was correct.");

        access_tone();

        // Get the value of the "message" key.
        // TODO: check if the key exists before getting the value.
        String message = doc->get<String>("message");
        Serial.println(message);
      } else {
        Serial.println("API call was successful, but the pin was incorrect.");
        no_access_tone();
      }
    } else {
      Serial.println("API call was successful, but the response was not valid.");
      return;
    }
  }
}

// FIXME: use char array instead of String
char* read_RFID() {
  previous_RFID_Millis = currentMillis;

  String content = "";

  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  return content.substring(1).c_str();  // return the UID
}

String read_memory(int x) {
  String memory = "";
  for (int i = 0; i < keycard_length; i++) {
    memory = String(memory + char(EEPROM.read(i + keycard_length * x)));
  }
  return memory;
}

void manual_put_memory(String memory, int x) {
  for (int i = 0; i < memory.length(); ++i) {
    Serial.print(memory[i]);
    EEPROM.write(i + keycard_length * x, memory[i]);
  }
  EEPROM.commit();
}

void put_memory(String memory) {
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
        EEPROM.write(x + i * keycard_length, 32);
      }
      EEPROM.commit();
    }
  }
}

void reset_memory() {
  for (int i = 0; i < keycards; i++) {
    for (int x = 0; x < keycard_length; ++x) {
      EEPROM.write(x + i * keycard_length, 32);
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
  myservo.write(0);
}

void close_door() {
  myservo.write(90);
}

void splitSerialMessage(char *serialMessage, unsigned long *password, int *userIndex) {
  char *token = strtok(serialMessage, ",");
  *password = strtoul(token, NULL, 10);
  
  token = strtok(NULL, ",");
  if (token == NULL) {
    *userIndex = -1;
    return;
  }
  
  *userIndex = atoi(token);
  token = strtok(NULL, ",");
  if (token == NULL || strtoul(token, NULL, 10) != masterPassword) {
    *userIndex = -1;
  }
}