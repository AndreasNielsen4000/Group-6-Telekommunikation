/*
  RFID.ino - This file contains the code for an ESP-based RFID system.
  It includes functionality for RFID input, EEPROM, serial communication between ESPs, WiFi setup and use of api_caller.
  It is meant to be used with an ESP8266 and communicates with another ESP through serial communication.
*/

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP_EEPROM.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <WiFiClientSecure.h>
#include <optional>

#include "Hash.h"
#include "api_caller.h"

#define SS_PIN D8 //MFRC522 SS pin ("slave select")
#define RST_PIN D0  //RESET pin for MFRC522
#define BUZZER_PIN D1 // Buzzer pin
#define NUM_USERS 5 // Number of users that can be saved in memory at the same time
#define KEYCARD_LENGTH 11 // Length of the keycard UID in bytes
#define MAX_MESSAGE_LENGTH 50 // Max length of the message from serial in bytes
#define OPEN_DOOR_TIME 5000  // The time the door is open in milliseconds
#define RFID_INTERVAL 5000  // Interval in milliseconds between RFID read
#define WIFI_INTERVAL 1000  // Interval in milliseconds between WiFi read
#define NEW_RFID_TIMEOUT 10000 // Time to read a new RFID before a timeout in milliseconds

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.
Servo door_servo;

// Wifi credentials
const char *ssid = "Alexanders iPhone";   /* Your SSID */
const char *pass = "tellmywifi";   /* Your Password */

const char *apiUrl = "http://172.20.10.5:8080"; /* Your API URL. Example: "https://api.example.com" */

// Global variables
unsigned long previous_RFID_Millis = 0;  // Will store last time the RFID was read
unsigned long previous_WIFI_Millis = 0;  // Will store last time the WIFI was updated
unsigned long currentMillis;
unsigned long adminPassword = hashPassword("12345678"); // adminpassword for the system

WiFiClient client;
ApiCaller apiCaller = ApiCaller(client, apiUrl);

// Struct for storing a user in memory (the pin and rfid are hashed)
struct User {
  unsigned long pin;
  unsigned long rfid;
};

// The array where the users are stored in memory (the pin and rfid are hashed)
User users[NUM_USERS] = {};

//Enum for the different received serial commands
enum SerialCommand {
  LOGIN,
  NEW_PASSWORD,
  NEW_RFID,
  CANCEL_RFID,
};

//Enum for the different serial responses
enum SerialSend {
  ACCESS_GRANTED,
  ACCESS_DENIED,
  RFID_READ,
  PIN_READ,
};

//Struct to store the command from serial communication and the associated data
struct CommandStruct {
  SerialCommand command;
  std::optional<unsigned long> password;
  std::optional<int> userIndex;
  std::optional<unsigned long> adminPassword;
};

struct ApiStruct {
  bool authenticated;
  int id;
};

bool connected_to_wifi(char retry_times);
std::optional<CommandStruct> processSerialCommunication();
void sendSerialResponse(SerialSend serialSend, int userIndex = -1);

void setup() {
  Serial.begin(115200);  // Initiate a serial communication

  EEPROM.begin(sizeof(User) * NUM_USERS);  // Initiate EEPROM memory size
  SPI.begin();                               // Initiate SPI bus
  mfrc522.PCD_Init();                        // Initiate MFRC522

  door_servo.attach(D4, 500, 2400);  // Attaches the servo on pin D4 to the servo object
  pinMode(BUZZER_PIN, OUTPUT);

  initialize_user_array_from_eeprom(); // Read the users array from EEPROM

  connected_to_wifi(3); // Try to connect to wifi 3 times
}

void loop() {
  // The loop is split up into two parts, one for serial input and one for RFID input, serial input is based on the optionalCommand

  std::optional<CommandStruct> optionalCommand = processSerialCommunication();

  // There is a command from serial to be executed
  if (optionalCommand.has_value()) {
    CommandStruct cmd = optionalCommand.value();

    switch (cmd.command)
    {
    case SerialCommand::LOGIN: {
      // Check if the password is correct
      // in either api or local if no internet
      // save in EEPROM from API if it has changed?
      if (connected_to_wifi(0)) {
        Serial.println("Calling api to check data");

        std::unique_ptr<DynamicJsonDocument> doc = apiCaller.GET("/pin", String(cmd.password.value()));
        if (doc == nullptr) {
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          return;
        } 

        if (doc->containsKey("authenticated") ) {
          bool authenticated = (*doc)["authenticated"].as<bool>();
          if (authenticated) {
            int id = (*doc)["id"].as<int>();
            sendSerialResponse(SerialSend::ACCESS_GRANTED, id);
            if (users[id].pin != cmd.password.value()) {
              Serial.println("Found password in server but not in EEPROM, adding password to EEPROM");
              compare_and_update_rfid(id, cmd.password.value());
            }
            
            access_tone();
            //Add to EEPROM at the correct index
            return;
          } else {
            sendSerialResponse(SerialSend::ACCESS_DENIED);
            for (int i = 0; i < NUM_USERS; i++) {
            if (users[i].pin == cmd.password.value()) {
              Serial.println("Found password in EEPROM but not in server, deleting from EEPROM");
              compare_and_update_pin(i, 0);
            }
          }
            no_access_tone();
            //If code is in EEPROM, remove it
            return;
          }
        } else {
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          no_access_tone();
          return;
        }
        
      } else {
        bool access_granted = false;
        // Look in EEPROM
        for (int i = 0; i < NUM_USERS; i++) {
          if (users[i].pin == cmd.password.value()) {
            access_granted = true;
            sendSerialResponse(SerialSend::ACCESS_GRANTED, i);
            access_tone();
            return;
          }
        }

        if (access_granted == false) {
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          no_access_tone();
        }
      }

      break; // LOGIN
    }
    case SerialCommand::NEW_RFID: {

      // You need to be connected to wifi to change the RFID
      if (!connected_to_wifi(0)) {
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          return;
      }

      if (!cmd.adminPassword.has_value()) {
        Serial.println("No admin password");
        return;
      }

      // Check if the admin password is correct
      if (cmd.adminPassword.value() != adminPassword) {
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        break;
      };

      // Get the new rfid code
      bool has_read_rfid = false;
      long new_rfid_millis = millis();

      unsigned long new_hashed_rfid = 0;

      do {
        if (!mfrc522.PICC_IsNewCardPresent()) {
          currentMillis = millis();
          continue;
        }
        if (!mfrc522.PICC_ReadCardSerial()) {
          continue;
        }
        Serial.println("Has read data");

        new_hashed_rfid = hashPassword(read_RFID().c_str());
        Serial.println(new_hashed_rfid);
        has_read_rfid = true;
      } while (!has_read_rfid && currentMillis - new_rfid_millis < NEW_RFID_TIMEOUT);

      if (!has_read_rfid) {
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        break;
      }

      std::unique_ptr<DynamicJsonDocument> doc = apiCaller.PUT("/rfid", String(cmd.userIndex.value()), String(new_hashed_rfid));
      if (doc == nullptr) {
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        break;
      } 

      if (doc->containsKey("success")) {
        bool succeeded = (*doc)["success"].as<bool>();
        if (succeeded) {

          Serial.println("Updated RFID");
          // Update the rfid in memory
          compare_and_update_rfid(cmd.userIndex.value(), new_hashed_rfid);

          sendSerialResponse(SerialSend::RFID_READ, cmd.userIndex.value());
          access_tone();
        } else {
          Serial.println("Failed to update RFID");
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          no_access_tone();
        }
      } else {
        Serial.println("Failed to update RFID");
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        no_access_tone();
      }

      break; // NEW_RFID
    }
    case SerialCommand::NEW_PASSWORD: {
      // You need to be connected to wifi to change the password
      if (!connected_to_wifi(0)) {
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          return;
      }

      if (!cmd.adminPassword.has_value() && !cmd.password.has_value()) {
        Serial.println("No admin password or password");
        return;
      }

      // Check if the admin password is correct
      if (cmd.adminPassword.value() != adminPassword) {
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        break;
      }

      std::unique_ptr<DynamicJsonDocument> doc = apiCaller.PUT("/pin", String(cmd.userIndex.value()), String(cmd.password.value()));
      if (doc == nullptr) {
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        break;
      } 

      if (doc->containsKey("success")) {
        bool succeeded = (*doc)["success"].as<bool>();
        if (succeeded) {

          Serial.println("Updated PIN");
          // Update the pin in memory
          compare_and_update_pin(cmd.userIndex.value(), cmd.password.value());

          sendSerialResponse(SerialSend::PIN_READ, cmd.userIndex.value());
          access_tone();
        } else {
          Serial.println("Failed to update PIN");
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          no_access_tone();
        }
      } else {
        Serial.println("Failed to update PIN");
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        no_access_tone();
      }

    }
    default:
      // Should never reach here
      break;
    }
  }

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
  if (currentMillis - previous_RFID_Millis > RFID_INTERVAL) {
    bool access_granted = false;

    // Call api
    if (connected_to_wifi(0)) {
      Serial.println("Calling api to check data");
      unsigned long new_hashed_rfid = hashPassword(read_RFID().c_str());

      std::unique_ptr<DynamicJsonDocument> doc = apiCaller.GET("/rfid", String(new_hashed_rfid));
      if (doc == nullptr) {
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        return;
      } 

      if (doc->containsKey("authenticated") ) {
        bool authenticated = (*doc)["authenticated"].as<bool>();
        if (authenticated) {
          int id = (*doc)["id"].as<int>();

          sendSerialResponse(SerialSend::ACCESS_GRANTED, id);
          
          if (users[id].rfid != new_hashed_rfid) {
            Serial.println("Found RFID in server but not in EEPROM, adding RFID to EEPROM");
            compare_and_update_rfid(id, new_hashed_rfid);
          }

          access_tone();
          return;
        } else {
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          for (int i = 0; i < NUM_USERS; i++) {
            if (users[i].rfid == new_hashed_rfid) {
              Serial.println("Found RFID in EEPROM but not in server, deleting from EEPROM");
              compare_and_update_rfid(i, 0);
            }
          }
          
          no_access_tone();
          return;
        }
      } else {
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        no_access_tone();
        return;
      }

    }

    // Look in EEPROM
    for (int i = 0; i < NUM_USERS; i++) {
      if (users[i].rfid == hashPassword(read_RFID().c_str())) {
        sendSerialResponse(SerialSend::ACCESS_GRANTED, i);
        access_granted = true;
        access_tone();
        break;
      }
    }

    if (access_granted == false) {
      sendSerialResponse(SerialSend::ACCESS_DENIED);
      no_access_tone();
    }
  }
}

/**
* Reads the UID of the RFID by collecting each byte together in on String
* Uses the MFRC522.h to read the bytes from the RFID
*
* @return the read RFID UID as a String
*/
String read_RFID() {
  previous_RFID_Millis = currentMillis;

  String content = "";

  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) { // Read 4 bytes from the RFID
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));// Add a leading zero to the UID bytes
    content.concat(String(mfrc522.uid.uidByte[i], HEX));  // Add the UID bytes to the content String
  }
  content.toUpperCase();
  return content.substring(1);  // return the UID
}

/**
* Read the pin or UID from a specific user 
*
* @param user_index: the index of the user in the users array
* @param read_rfid: true if is a RFID UID that needs to be read
* @return the hashed pin or UID as an unsigned long
*/
unsigned long read_memory(int user_index, bool read_rfid) {
  int rfid_index = 0;
  if (read_rfid) {
    rfid_index = 1;
  }

  // Calculate the EEPROM address for the current user
  int address = (user_index * 2 + rfid_index) * sizeof(long);

  // Read every byte of the data
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  // Collect each byte of the long together in one variable
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

/*
* Save all of the pins and UIDs from the EEPROM to the user array
*/
void initialize_user_array_from_eeprom() {
  // Read the users array from EEPROM
  for (int i = 0; i < NUM_USERS; i++) {
    // Calculate the EEPROM address for the current user
    int address = i * sizeof(User);

    // Read the user data from EEPROM
    EEPROM.get(address, users[i]);
  }
}

/*
* Save the user array that is filled with all of the pins and UIDs to the EEPROM
*/
void write_user_array_to_eeprom() {
  for (int i = 0; i < NUM_USERS; i++) {
    // Calculate the EEPROM address for the current user
    int address = i * sizeof(User);

    // Write the hashed PIN and RFID values to EEPROM
    EEPROM.put(address, users[i]);
  }
  EEPROM.commit();
}

/*
* Replace all of the memory of the EEPROM by a zero, thereby reseting the memory
*/
void reset_memory() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();

  initialize_user_array_from_eeprom();
}

/**
* Compares the new hashed pin with the old hashed pin and updates the pin if they are different
* It updates the pin in memory (users array) and in EEPROM
*
* @param user_index: the index of the user in the users array
* @param new_hashed_pin: the new hashed pin
*/
void compare_and_update_pin(int user_index, unsigned long new_hashed_pin) {
  if (users[user_index].pin != new_hashed_pin) {
    // Update the pin in user array
    users[user_index].pin = new_hashed_pin;

    // Update the pin in EEPROM
    int address = (user_index * 2) * sizeof(long);
    EEPROM.put(address, new_hashed_pin);
    EEPROM.commit();
  }
}  

/** 
* Compares the new hashed rfid with the old hashed rfid and updates the rfid if they are different
* It updates the rfid in memory (users array) and in EEPROM
*
* @param user_index: the index of the user in the users array
* @param new_hashed_rfid: the new hashed rfid
*/
void compare_and_update_rfid(int user_index, unsigned long new_hashed_rfid) {
  if (users[user_index].rfid != new_hashed_rfid) {
    // Update the rfid in user array
    users[user_index].rfid = new_hashed_rfid;

    // Update the rfid in EEPROM
    int address = (user_index * 2 + 1) * sizeof(long);
    EEPROM.put(address, new_hashed_rfid);
    EEPROM.commit();
  }
}

/*
* Plays a melody when the access is granted, and then the door opens and closes after a specified time
*/
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
  delay(OPEN_DOOR_TIME - 300);
  close_door();
}

/**
* Plays a melody when the access was denied
*/
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
  door_servo.write(180);
}

void close_door() {
  door_servo.write(100);
}

/**
 * Connect to wifi with a number of retries
 *
 * @param retry_times: number of times to retry connecting to wifi. Default is 0
 * @return true if connected to wifi, false otherwise
 */
bool connected_to_wifi(char retry_times = 0){
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Already connected to WiFi");
    return true;
  }
  Serial.println("WiFi not connected, connecting to WiFi SSID " + String(ssid));
  WiFi.begin(ssid, pass);
  // Waiting for WIFI connection
  char i = 0;
  while (WiFi.status() != WL_CONNECTED && i < retry_times) {
    delay(500);
    i++;
  }

  return WiFi.status() == WL_CONNECTED;
}

/**
 * Reads the serial input and processes it to extract command information.
 * 
 * @return An optional CommandStruct object containing the extracted command information, or std::nullopt if no commands were sent.
 */
std::optional<CommandStruct> processSerialCommunication() {
  // Read serial input
  char serialInput[MAX_MESSAGE_LENGTH] = {'\0'};
  CommandStruct commandStruct;
  commandStruct.adminPassword = std::nullopt;
  commandStruct.password = std::nullopt;
  commandStruct.userIndex = std::nullopt;

  if (Serial.available()) { // Check if there is serial data available
    Serial.readBytesUntil('\n', serialInput, MAX_MESSAGE_LENGTH);
    char* token = strtok(serialInput, ","); // Split the serial message using comma as the delimiter
    if (strcmp(token, "LOGIN") == 0) {
      commandStruct.command = SerialCommand::LOGIN;
    } else if (strcmp(token, "NEW_PASSWORD") == 0) {
      commandStruct.command = SerialCommand::NEW_PASSWORD;
    } else if (strcmp(token, "NEW_RFID") == 0) {
      commandStruct.command = SerialCommand::NEW_RFID;
    } else if (strcmp(token, "CANCEL_RFID") == 0) {
      commandStruct.command = SerialCommand::CANCEL_RFID;
    } else {
      // No commands sent
      return std::nullopt;
    }
    Serial.print("Command: ");
    Serial.println(commandStruct.command);  
    token = strtok(NULL, ","); // Get the next token (password)
    if (token != NULL && strlen(token) > 0 && strcmp(token, " ") != 0) {
      commandStruct.password = strtoul(token, NULL, 10); // Convert the password string to an unsigned long integer
      Serial.print("Password: ");
      Serial.println(commandStruct.password.value());
    }
    token = strtok(NULL, ","); // Get the next token (user index)
    if (token != NULL && strlen(token) > 0 && strcmp(token, " ") != 0) {
      commandStruct.userIndex = atoi(token); // Convert the user index string to an integer
      Serial.print("User index: ");
      Serial.println(commandStruct.userIndex.value());
    }
    token = strtok(NULL, ","); // Get the next token (admin password)
    if (token != NULL && strcmp(token, " ") != 0) {
      commandStruct.adminPassword = strtoul(token, NULL, 10); // Convert the admin password string to an unsigned long integer
      Serial.print("Admin password: ");
      Serial.println(commandStruct.adminPassword.value());
    }
    return commandStruct;
  }

  // No commands sent
  return std::nullopt;
}

/**
 * Sends a serial response based on the given SerialSend value and user index.
 * 
 * @param serialSend The type of serial response to send (ACCESS_GRANTED, ACCESS_DENIED, or RFID_READ).
 * @param userIndex The index of the user associated with the response, or -1 if no user is associated with the response.
 */
void sendSerialResponse(SerialSend serialSend, int userIndex) {
  switch (serialSend) {
    case SerialSend::ACCESS_GRANTED: //Access granted
      Serial.println("ACCESS," + String(userIndex));
      break;
    case SerialSend::ACCESS_DENIED: //Access denied
      Serial.println("ACCESS_DENIED," + String(userIndex));
      break;
    case SerialSend::RFID_READ: //We have read an RFID
      Serial.println("RFID_READ," + String(userIndex));
      break;
    default:
      break; // Should never reach here
  }
}