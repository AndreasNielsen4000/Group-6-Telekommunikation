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

// TODO:
#define SS_PIN D8   //find out what it does
#define RST_PIN D0  //find out what it does
#define BUZZER_PIN D1 // Buzzer pin

#define NUM_USERS 5 // Number of users that can be saved in memory at the same time
#define KEYCARD_LENGTH 11 // Length of the keycard UID in bytes

// TODO: check if this is correct
#define MAX_MESSAGE_LENGTH 50 // Max length of the message from serial in bytes

#define RFID_INTERVAL 5000  // Interval in milliseconds between RFID read
#define WIFI_INTERVAL 1000  // Interval in milliseconds between WiFi read
#define NEW_RFID_TIMEOUT 10000 // Time to read a new RFID before a timeout in milliseconds

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.
Servo door_servo;

// Wifi credentials
const char *ssid = "Pixel_6553";   /* Your SSID */
const char *pass = "yzdrzatxcsywnih";   /* Your Password */

const char *apiUrl = "http://192.168.143.130:8080"; /* Your API URL. Example: "https://api.example.com" */

// Global variables
unsigned long previous_RFID_Millis = 0;  // Will store last time the RFID was read
unsigned long previous_WIFI_Millis = 0;  // Will store last time the WIFI was updated
unsigned long currentMillis;
unsigned long masterPassword = hashPassword("12345678"); // Master password for the system

WiFiClient client;
ApiCaller apiCaller = ApiCaller(client, apiUrl);

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

enum SerialCommand {
  LOGIN, // hashedPassword, userIndex = -1
  NEW_PASSWORD, // hashedPassword, userIndex, masterPassword
  NEW_RFID, // userIndex, masterPassword
  CANCEL_RFID,
};

enum SerialSend {
  ACCESS_GRANTED, // userIndex
  ACCESS_DENIED, // userIndex
  RFID_READ, // rfid
};

struct CommandStruct {
  SerialCommand command;
  std::optional<unsigned long> password;
  std::optional<int> userIndex;
  std::optional<unsigned long> masterPassword; // masterPassword is not set on each command
};

struct ApiStruct {
  bool authenticated;
  int id;
};


// TODO: do something with this
bool connected_to_wifi(char retry_times);
std::optional<CommandStruct> processSerialCommunication();
void sendSerialResponse(SerialSend serialSend, int userIndex = -1);


// TODO: rename me to something better
//ApiStruct call_api(unsigned long password) {
//// TODO: right now we ignore the index of the pin, but we shoulden't
//  // Create a WiFiClient object to talk to the server.
//  WiFiClient client;
//
//  // Create an ApiCaller object with the WiFiClient and API URL.
//  ApiCaller apiCaller(client, apiUrl);
//
//  // Call the API with a pin
//  String passwordString = String(password);
//  std::unique_ptr<DynamicJsonDocument> doc = apiCaller.GET("/rfid", passwordString);
//
//  // Check if the API call was successful.
//  if (doc == nullptr) {
//    return ApiStruct{false, 0};
//  } 
//  
//  // Format the response
//  if (doc->containsKey("authenticated") && doc->containsKey("id")) {
//    bool auth = (*doc)["authenticated"].as<bool>();
//    int id = (*doc)["id"].as<int>();
//
//    return ApiStruct{auth, id};
//  }
//
//  return ApiStruct{false, "API call was successful, but the response was not valid."};
//}


void setup() {
  Serial.begin(115200);  // Initiate a serial communication

  EEPROM.begin(sizeof(User) * NUM_USERS);  // Initiate EEPROM memory size
  SPI.begin();                               // Initiate SPI bus
  mfrc522.PCD_Init();                        // Initiate MFRC522

  door_servo.attach(D4, 500, 2400);  // Attaches the servo on pin D4 to the servo object
  pinMode(BUZZER_PIN, OUTPUT);

  connected_to_wifi(3); // Try to connect to wifi 3 times
}


void loop() {
  // The loop is split up into two parts, one for serial input and one for RFID input

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
        if (!cmd.password.has_value()) {
          // TODO: error should have the password...
        }
        
        // FIXME: plz
        ////ApiStruct api_call = call_api(cmd.password.value());
        
        //if (api_call.authenticated) {
        //  // TODO: should get index of user (the 1)
        //  //Serial.println("1,-1");
        //  sendSerialResponse(SerialSend::ACCESS_GRANTED, api_call.id);
        //  access_tone();
        //} else {
        //  //Serial.println("0,-1");
        //  sendSerialResponse(SerialSend::ACCESS_DENIED);
        //  no_access_tone();
        //}

        
      } else {
        bool access_granted = false;
        // Look in EEPROM
        for (int i = 0; i < NUM_USERS; i++) {
          if (users[i].pin == cmd.password.value()) {
            //Serial.println("1,-1");
            access_granted = true;
            sendSerialResponse(SerialSend::ACCESS_GRANTED, i);
            access_tone();
            return;
          }
        }

        if (access_granted == false) {
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          //Serial.println("0,-1");
          no_access_tone();
        }
      }


      break; // LOGIN
    }
    case SerialCommand::NEW_RFID: {
      // TODO: test for cancel command

      // You need to be connected to wifi to change the RFID
      if (!connected_to_wifi(0)) {
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          return;
      }

      Serial.println("Made it here");

      if (!cmd.masterPassword.has_value()) {
        Serial.println("We are fucked");
      }

      // Check if the master password is correct
      if (cmd.masterPassword.value() != masterPassword) {
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        break;
      }

      Serial.println("Made it here 2");

      // Get the new rfid code
      bool has_read_rfid = false;
      long new_rfid_millis = millis();

      unsigned long new_hashed_rfid;

      Serial.println("Made it here 3");

      do {
        if (!mfrc522.PICC_IsNewCardPresent() && !mfrc522.PICC_ReadCardSerial()) {
          currentMillis = millis();
          continue;
        }

        new_hashed_rfid = hashPassword(read_RFID().c_str());
        has_read_rfid = true;
      } while (!has_read_rfid || currentMillis - new_rfid_millis < NEW_RFID_TIMEOUT);

      if (!has_read_rfid) {
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        break;
      }

      std::unique_ptr<DynamicJsonDocument> doc = apiCaller.PUT("/rfid", String(new_hashed_rfid), String(cmd.userIndex.value()));
      if (doc == nullptr) {
        // TODO: error handling
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        break;
      } 

      if (doc->containsKey("success")) {
        bool succeeded = (*doc)["success"].as<bool>();
        if (succeeded) {

          // Update the rfid in memory
          compare_and_update_rfid(cmd.userIndex.value(), new_hashed_rfid);

          sendSerialResponse(SerialSend::RFID_READ, cmd.userIndex.value());
          access_tone();
        } else {
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          no_access_tone();
        }
      }

    break; // NEW_RFID
    }
    default:
      // Should never reach here
      // TODO: error handling
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

  //Serial.println(hashPassword(read_RFID().c_str()));
  
  // Only read the UID if it is over the interval length since it has last been read
  currentMillis = millis();
  if (currentMillis - previous_RFID_Millis > RFID_INTERVAL) {
    bool access_granted = false;

    // Call api
    if (connected_to_wifi(0)) {
      Serial.println("Calling api to check data");

      std::unique_ptr<DynamicJsonDocument> doc = apiCaller.GET("/rfid", String(hashPassword(read_RFID().c_str())));
      if (doc == nullptr) {
        // TODO: error handling
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        return;
      } 

      if (doc->containsKey("authenticated") ) {
        bool authenticated = (*doc)["authenticated"].as<bool>();
        if (authenticated) {
          int id = (*doc)["id"].as<int>();

          sendSerialResponse(SerialSend::ACCESS_GRANTED, id);
          access_tone();
          return;
        } else {
          sendSerialResponse(SerialSend::ACCESS_DENIED);
          no_access_tone();
          return;
        }
      } else {
        sendSerialResponse(SerialSend::ACCESS_DENIED);
        no_access_tone();
        return;
      }

      //ApiStruct api_call = call_api(hashPassword(read_RFID().c_str()));
      //if (api_call.authenticated) {
      //  sendSerialResponse(SerialSend::ACCESS_GRANTED, 1);//TO DO: get index of user from api
      //  access_granted = true;
      //  access_tone();
      //} else {
      //  sendSerialResponse(SerialSend::ACCESS_DENIED);
      //  //Serial.println("0,-1");
      //  no_access_tone();
      //}
      //return;
    }

    // Look in EEPROM
    for (int i = 0; i < NUM_USERS; i++) {
      if (users[i].rfid == hashPassword(read_RFID().c_str())) {
        sendSerialResponse(SerialSend::ACCESS_GRANTED, i);
        //Serial.println("1,-1");
        access_granted = true;
        access_tone();
        break;
      }
    }

    if (access_granted == false) {
      sendSerialResponse(SerialSend::ACCESS_DENIED);
      //Serial.println("0,-1");
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

unsigned long read_memory(int user_index, bool rfid) {
  int rfid_index = 0;
  if (rfid) {
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

void initialize_user_array_from_eeprom() {
  // Read the users array from EEPROM
  for (int i = 0; i < NUM_USERS; i++) {
    // Calculate the EEPROM address for the current user
    int address = i * sizeof(User);

    // Read the user data from EEPROM
    EEPROM.get(address, users[i]);
  }
}

void write_user_array_to_eeprom() {
  for (int i = 0; i < NUM_USERS; i++) {
    // Calculate the EEPROM address for the current user
    int address = i * sizeof(User);

    // Write the hashed PIN and RFID values to EEPROM
    EEPROM.put(address, users[i]);
  }
}

void reset_memory() {
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();

  initialize_user_array_from_eeprom();
}

/*
* Compares the new hashed pin with the old hashed pin and updates the pin if they are different
* It updates the pin in memory (users array) and in EEPROM
*
* @param user_index: the index of the user in the users array
* @param new_hashed_pin: the new hashed pin
*/
void compare_and_update_pin(int user_index, unsigned long new_hashed_pin) {
  if (users[user_index].pin != new_hashed_pin) {
    // Update the pin in memory
    users[user_index].pin = new_hashed_pin;

    // Update the pin in EEPROM
    int address = (user_index * 2) * sizeof(long);
    EEPROM.put(address, new_hashed_pin);
  }
}  

/* 
* Compares the new hashed rfid with the old hashed rfid and updates the rfid if they are different
* It updates the rfid in memory (users array) and in EEPROM
*
* @param user_index: the index of the user in the users array
* @param new_hashed_rfid: the new hashed rfid
*/
void compare_and_update_rfid(int user_index, unsigned long new_hashed_rfid) {
  if (users[user_index].rfid != new_hashed_rfid) {
    // Update the rfid in memory
    users[user_index].rfid = new_hashed_rfid;

    // Update the rfid in EEPROM
    int address = (user_index * 2 + 1) * sizeof(long);
    EEPROM.put(address, new_hashed_rfid);
  }
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
  delay(1700);
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
* Connect to wifi with a number of retries
*
* @param retry_times: number of times to retry connecting to wifi. Default is 0
* @return true if connected to wifi, false otherwise
*/
bool connected_to_wifi(char retry_times = 0){
  WiFi.begin(ssid, pass);
  // Waiting for WIFI connection
  char i = 0;
  while (WiFi.status() != WL_CONNECTED && i < retry_times) {
    delay(500);
    i++;
  }

  return WiFi.status() == WL_CONNECTED;
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

/**
 * Reads the serial input and processes it to extract command information.
 * 
 * @return An optional CommandStruct object containing the extracted command information, or std::nullopt if no commands were sent.
 */
std::optional<CommandStruct> processSerialCommunication() {
  // Read serial input
  char serialInput[MAX_MESSAGE_LENGTH] = {'\0'};
  CommandStruct commandStruct;
  commandStruct.masterPassword = std::nullopt;
  commandStruct.password = std::nullopt;
  commandStruct.userIndex = std::nullopt;

  if (Serial.available()) { // Check if there is serial data available
    Serial.readBytesUntil('\n', serialInput, MAX_MESSAGE_LENGTH);
    //Serial.println(serialInput);
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
    if (token != NULL && strlen(token) > 0) {
      commandStruct.password = strtoul(token, NULL, 10); // Convert the password string to an unsigned long integer
      Serial.print("Password: ");
      Serial.println(commandStruct.password.value());
    }
    token = strtok(NULL, ","); // Get the next token (user index)
    if (token != NULL && strlen(token) > 0) {
      commandStruct.userIndex = atoi(token); // Convert the user index string to an integer
      Serial.print("User index: ");
      Serial.println(commandStruct.userIndex.value());
    }
    token = strtok(NULL, ","); // Get the next token (master password)
    if (token != NULL) {
      commandStruct.masterPassword = strtoul(token, NULL, 10); // Convert the master password string to an unsigned long integer
      Serial.print("Master password: ");
      Serial.println(commandStruct.masterPassword.value());
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
///*
//  processSerialCommunication - reads, writes and processes serial input/output.
//  It checks for available serial data, reads the input until a newline character is encountered,
//  splits the received message into password and user index, and performs access control based on the received data.
//  If a user index is received, it updates the password for that user.
//  Parameters:
//    - serialInput: The character array to store the received serial input.
//    - receivedPasswordSerial: Reference to the variable to store the received password.
//    - receivedUserIndexSerial: Reference to the variable to store the received user index.
//*/
//void processSerialCommunication(char *serialInput, unsigned long &receivedPasswordSerial, int &receivedUserIndexSerial) {
//  if (Serial.available()) { // Check if there is serial data available
//        Serial.readBytesUntil('\n', serialInput, MAX_MESSAGE_LENGTH);
//        //Serial.println(serialInput);
//        splitSerialMessage(serialInput, &receivedPasswordSerial, &receivedUserIndexSerial); // Split the serial message into password and user index
//        bool accessGranted = false;
//        if (receivedUserIndexSerial < 0) { // If we have not received a user index, then splitSerialMessage has received a valid password, therefore we must check the password.
//            // Right now local but check api first
//              if (connected_to_wifi()) {
//                // Call api
//                //accessGranted = call_api(receivedPasswordSerial).authenticated;
//              } else {
//                for (int i = 0; i < NUM_USERS; i++) {
//                  if (users[i].pin == receivedPasswordSerial) {
//                      accessGranted = true;
//                      Serial.println("1," + String(i));
//                      access_tone();
//                      break;
//                  } 
//                }
//              }
//            if (accessGranted == false){ // If the password is incorrect, then accessGranted will be false, and we must deny access.
//              Serial.println("0,-1");
//              no_access_tone();
//            }
//        // If we have received a user index, then splitSerialMessage has received a valid master password, therefore we must update the password for that user.
//        } else if (receivedUserIndexSerial < NUM_USERS && receivedUserIndexSerial >= 0) { // This updates password for user, needs to work with server
//            users[receivedUserIndexSerial].pin = receivedPasswordSerial; //update password
//        } else {
//            //Serial.println("Invalid user index");
//        }
//        //Serial.println(serialInput);
//    }
//}


/* Read from EEPROM
  // Read the users array from EEPROM
  for (int i = 0; i < NUM_USERS; i++) {
    // Calculate the EEPROM address for the current user
    int address = i * sizeof(User);

    // Read the user data from EEPROM
    EEPROM.get(address, users[i]);
  }
*/


