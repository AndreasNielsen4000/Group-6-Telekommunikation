#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP_EEPROM.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <WiFiClientSecure.h>

#include "Hash.h"
#include "api_caller.h"

// TODO:
#define SS_PIN D8   //find out what it does
#define RST_PIN D0  //find out what it does
#define BUZZER_PIN D1 // Buzzer pin

#define NUM_USERS 5 // Number of users that can be saved in memory at the same time
#define KEYCARD_LENGTH 11 // Length of the keycard UID in bytes

// TODO: check if this is correct
#define MAX_MESSAGE_LENGTH 32 // Max length of the message from serial in bytes

#define RFID_INTERVAL 5000  // Interval in milliseconds between RFID read
#define WIFI_INTERVAL 1000  // Interval in milliseconds between WiFi read


MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.
Servo door_servo;

// Wifi credentials
const char *ssid = "";   /* Your SSID */
const char *pass = "";   /* Your Password */

const char *apiUrl = ""; /* Your API URL. Example: "https://api.example.com" */

// Global variables
unsigned long previous_RFID_Millis = 0;  // Will store last time the RFID was read
unsigned long previous_WIFI_Millis = 0;  // Will store last time the WIFI was updated
unsigned long currentMillis;
unsigned long masterPassword = hashPassword("12345678"); // Master password for the system

// Struct for stroing a user in memory (the pin and rfid are hashed)
struct User {
  unsigned long pin;
  unsigned long rfid;
};

// The array where the users are stored in memory (the pin and rfid are hashed)
// TODO: this should be stored in EEPROM
User users[NUM_USERS] = {
  { hashPassword("1234"), hashPassword("50 48 B5 1E")},
  { 0, hashPassword("0A 59 91 17")},
  { 0, hashPassword("84 93 E4 52")},
  { 0, 0 },
  { 0, 0 }
};

void setup() {
  Serial.begin(115200);  // Initiate a serial communication

  EEPROM.begin(NUM_USERS * KEYCARD_LENGTH);  // Initiate EEPROM memory size
  SPI.begin();                               // Initiate SPI bus
  mfrc522.PCD_Init();                        // Initiate MFRC522

  door_servo.attach(D4, 500, 2400);  // Attaches the servo on pin D4 to the servo object
  pinMode(BUZZER_PIN, OUTPUT);

  connected_wifi(3); // Try to connect to wifi 3 times
}

void loop() {
  // The loop is split up into two parts, one for serial input and one for RFID input

  // Read serial input
  char serialInput[MAX_MESSAGE_LENGTH] = {'\0'};
  unsigned long receivedPasswordSerial = 0;
  int receivedUserIndexSerial = -1;

  processSerialCommunication(serialInput, receivedPasswordSerial, receivedUserIndexSerial);

    // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  //Serial.println(hashPassword(read_RFID().c_str()));
  
  // Only read the UID if it is over the interval length since it has last been read
  // TODO: this should hash input and compare to hashed values in memory
  // TODO: This should also have pin 
  currentMillis = millis();
  if (currentMillis - previous_RFID_Millis > RFID_INTERVAL) {
    bool access_granted = false;

    for (int i = 0; i < NUM_USERS; i++) {
      if (users[i].rfid == hashPassword(read_RFID().c_str())) {
        Serial.println("1," + String(i));
        access_granted = true;
        access_tone();
        break;
      }
      //Serial.println(UID[i]);
    }

    if (access_granted == false) {
      Serial.println("0,-1");
      no_access_tone();
    }
  }

  /* currentMillis = millis();

  if (currentMillis - previous_WIFI_Millis > WIFI_INTERVAL) {
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
  } */
}

// FIXME: use char array instead of String
String read_RFID() {
  previous_RFID_Millis = currentMillis;

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
  char memory[12];
  for (int i = 0; i < KEYCARD_LENGTH; i++) {
    memory[i] = char(EEPROM.read(i + KEYCARD_LENGTH * x));
  }
  return memory;
}

void manual_put_memory(String memory, int x) {
  for (int i = 0; i < memory.length(); ++i) {
    //Serial.print(memory[i]);
    EEPROM.write(i + KEYCARD_LENGTH * x, memory[i]);
  }
  EEPROM.commit();
}

void put_memory(String memory) {
  // Checks if the new UID to be saved is allready saved
  for (int i = 0; i < NUM_USERS; i++) {
    if (read_memory(i) == memory) {
      return;
    }
  }
  for (int i = 0; i < NUM_USERS; i++) {
    if (read_memory(i) == "           ") {
      for (int x = 0; x < memory.length(); ++x) {
        //Serial.print(memory[x]);
        EEPROM.write(x + i * 11, memory[x]);
      }
      EEPROM.commit();
      return;
    }
  }
}

void remove_memory(String memory) {
  for (int i = 0; i < NUM_USERS; i++) {
    if (read_memory(i) == memory) {
      for (int x = 0; x < memory.length(); ++x) {
        EEPROM.write(x + i * KEYCARD_LENGTH, 32);
      }
      EEPROM.commit();
    }
  }
}

void reset_memory() {
  for (int i = 0; i < NUM_USERS; i++) {
    for (int x = 0; x < KEYCARD_LENGTH; ++x) {
      EEPROM.write(x + i * KEYCARD_LENGTH, 32);
    }
  }
  EEPROM.commit();
}

void access_tone() {
  tone(BUZZER_PIN, 1046.5);
  delay(200);
  tone(BUZZER_PIN, 1174.66);
  delay(200);
  tone(BUZZER_PIN, 1318.51);
  delay(200);
  noTone(BUZZER_PIN);
  delay(800);
  tone(BUZZER_PIN, 1396.91);
  open_door();
  delay(300);
  noTone(BUZZER_PIN);
  delay(9700);
  close_door();
}

void no_access_tone() {
  tone(BUZZER_PIN, 932.33);
  delay(500);
  tone(BUZZER_PIN, 880);
  delay(500);
  tone(BUZZER_PIN, 830.61);
  delay(800);
  noTone(BUZZER_PIN);
}

void open_door() {
  door_servo.write(0);
}

void close_door() {
  door_servo.write(90);
}

/*
  Function to split a serial message into a password and user index.
  The serial message should be in the format "password,userIndex,masterPassword(optional)".
  The master password is optional and is only used to change the password of a user.
  If the master password is not provided, the user index will be -1 meaning that the password is meant to be checked.
  Parameters:
    - serialMessage: The serial message to split.
    - password: Reference to the variable to store the password.
    - userIndex: Reference to the variable to store the user index.
*/
void splitSerialMessage(char *serialMessage, unsigned long *password, int *userIndex) {
  char *token = strtok(serialMessage, ","); // Split the serial message using comma as the delimiter
  *password = strtoul(token, NULL, 10); // Convert the password string to an unsigned long integer
  
  token = strtok(NULL, ","); // Get the next token (user index)
  if (token == NULL) {
    *userIndex = -1; // If there is no user index provided, set it to -1
    return;
  }
  
  *userIndex = atoi(token); // Convert the user index string to an integer
  token = strtok(NULL, ","); // Get the next token (master password)
  if (token == NULL || strtoul(token, NULL, 10) != masterPassword) {
    *userIndex = -1; // If there is no master password provided or the master password is incorrect, set the user index to -1
  }
}

/*
  processSerialCommunication - reads writes and processes serial input/output.
  It checks for available serial data, reads the input until a newline character is encountered,
  splits the received message into password and user index, and performs access control based on the received data.
  If a user index is received, it updates the password for that user.
  Parameters:
    - serialInput: The character array to store the received serial input.
    - receivedPasswordSerial: Reference to the variable to store the received password.
    - receivedUserIndexSerial: Reference to the variable to store the received user index.
*/
void processSerialCommunication(char *serialInput, unsigned long &receivedPasswordSerial, int &receivedUserIndexSerial) {
  if (Serial.available()) { // Check if there is serial data available
        Serial.readBytesUntil('\n', serialInput, MAX_MESSAGE_LENGTH);
        //Serial.println(serialInput);
        splitSerialMessage(serialInput, &receivedPasswordSerial, &receivedUserIndexSerial); // Split the serial message into password and user index
        bool accessGranted = false;
        if (receivedUserIndexSerial < 0) { // If we have not received a user index, then splitSerialMessage has received a valid password, therefore we must check the password.
            // Right now local but check api first
              if (connected_wifi()) {
                // Call api
                accessGranted = call_api(receivedPasswordSerial).authenticated;
              } else {
                for (int i = 0; i < NUM_USERS; i++) {
                  if (users[i].pin == receivedPasswordSerial) {
                      accessGranted = true;
                      Serial.println("1," + String(i));
                      access_tone();
                      break;
                  } 
                }
              }
            if (accessGranted == false){ // If the password is incorrect, then accessGranted will be false, and we must deny access.
              Serial.println("0,-1");
              no_access_tone();
            }
        // If we have received a user index, then splitSerialMessage has received a valid master password, therefore we must update the password for that user.
        } else if (receivedUserIndexSerial < NUM_USERS && receivedUserIndexSerial >= 0) { // This updates password for user, needs to work with server
            users[receivedUserIndexSerial].pin = receivedPasswordSerial; //update password
        } else {
            //Serial.println("Invalid user index");
        }
        //Serial.println(serialInput);
    }
}

/*
* Connect to wifi with a number of retries
*
* @param retry_times: number of times to retry connecting to wifi. Default is 0
* @return true if connected to wifi, false otherwise
*/
bool connected_wifi(char retry_times = 0){
  WiFi.begin(ssid, pass);
  // Waiting for WIFI connection
  char i = 0;
  while (WiFi.status() != WL_CONNECTED && i < retry_times) {
    delay(500);
    i++;
  }

  return WiFi.status() == WL_CONNECTED;
}

struct ApiStruct {
  bool authenticated;
  String message;
};

ApiStruct call_api(unsigned long password) {
// TODO: right now we ignore the index of the pin, but we shoulden't
  // Create a WiFiClient object to talk to the server.
  WiFiClient client;

  // Create an ApiCaller object with the WiFiClient and API URL.
  ApiCaller apiCaller(client, apiUrl);

  // Call the API with a pin
  String passwordString = String(password);
  std::unique_ptr<DynamicJsonDocument> doc = apiCaller.GET("/pin", passwordString);

  // Check if the API call was successful.
  if (doc == nullptr) {
    return ApiStruct{false, "Error calling API"};
  } else if (doc->containsKey("authenticated")) {
    bool auth = (*doc)["authenticated"].as<bool>();
    return ApiStruct{auth, (*doc)["message"].as<String>()};
  }
}
