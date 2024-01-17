/*
  Keypad_ESP.ino - This file contains the code for an ESP-based keypad system.
  It includes functionality for keypad input, LCD display, LED control, and serial communication between ESPs.
  It is meant to be used with and ESP32 and communicates with another ESP through serial communication.
  Created by Alexander Nordentoft (s176361) and Andreas Nielsen (s203833), January 2024.
*/

#include <Keypad.h>
#include "Hash.h"
#include "lcd_display.h"
#include "led_control.h"
#include <BleSerial.h>

// Constants
#define MAX_MESSAGE_LENGTH 9
#define MAX_SERIAL_MESSAGE_LENGTH 150
#define NUM_USERS 5
#define RXD2 13 //define RX pin for serial2 communication (between ESPs)
#define TXD2 15 //define TX pin for serial2 communication (between ESPs)
// Pins for LED, light sensor and buzzer
#define RED_LED 4
#define GREEN_LED 14
#define BLUE_LED 27
#define WHITE_LED 23
#define LDR_PIN 36
#define BUZZER_PIN 5
// Rows and columns for keypad and keypad initialization
#define ROW_NUM 4 
#define COLUMN_NUM 4
//Keypad setup created with help from: https://arduinogetstarted.com/tutorials/arduino-keypad
byte rowPins[ROW_NUM] = {16, 17, 18, 19}; //connect to the row pinouts of the keypad
byte columnPins[COLUMN_NUM] = {25, 26, 32, 33}; //connect to the column pinouts of the keypad

// Variables for user and menu index
int menuIndex = 0;
int userIndex = 0;

// Function declarations
void adminMenuKeyPad(char *serialMessage);

char keypadKeys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};


Keypad keypad = Keypad( makeKeymap(keypadKeys), rowPins, columnPins, ROW_NUM, COLUMN_NUM );

// Password for admin access
unsigned long masterPassword = hashPassword("12345678");

// Object declarations for LCD display, LED control and Bluetooth serial communication
lcd_display lcdDisplay;
led_control ledControl(RED_LED, GREEN_LED, BLUE_LED, WHITE_LED, LDR_PIN, BUZZER_PIN);
BleSerial SerialBT;

//Setup function for Arduino
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  SerialBT.begin("KeypadBT"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
  lcdDisplay.init();
  lcdDisplay.enterPasswordLCD("default"); //initial menu on lcd
  ledControl.init();
}

//Loop function for Arduino
void loop() {
  char message[MAX_MESSAGE_LENGTH] = {'\0'}; //message array for keypad input
  char serialMessage[MAX_SERIAL_MESSAGE_LENGTH] = {'\0'}; //message array for serial input

  /*  //Serial communication check
  if (Serial2.available()) {
      Serial2.readBytesUntil('\n', serialMessage, MAX_SERIAL_MESSAGE_LENGTH);
      Serial.println("Serial message received in loop()");
      return;
  } */


  //Start main menu keypad function, changes the message array with keypad inputs
  mainMenuKeyPad(message, serialMessage);
  
  unsigned long messageHash = hashPassword(message);   
  ledControl.controlLED(0); //Set LED to idle status
  
  //If messageHash is not equal to the hash of an empty string, send the password to the other ESP through serial
  if (messageHash != hashPassword("\0")) {
    Serial2.println(String("LOGIN") + "," + String(messageHash) + "," + " " + ",");
    Serial.println(String("LOGIN") + "," + String(messageHash) + "," + " " + ",");
  }

  // If serialMessage is not empty, split the serial message from other ESP into message, accessGranted and userIndex, serial prints for debugging
  if (serialMessage[0] != '\0' && serialMessage[0] != '\r') {
    Serial.println("Serial message received from serial: " + String(serialMessage));
    parseSerialResponse(serialMessage);
  }
}

/**
 * Parses the serial response message and updates the status based on the received information.
 * 
 * @param serialMessage The serial response message to parse.
 */
void parseSerialResponse(char *serialMessage) {
  int userIndex = -1;
  bool accessGranted = false;
  String tempMessage = "";
  tempMessage = serialMessage;
  char *token = strtok(serialMessage, ",");
  if (strcmp(token, "ACCESS") == 0) {
    accessGranted = true;
  } else if (strcmp(token, "ACCESS_DENIED") == 0) {
    accessGranted = false;
  } else {  // Not a valid message, must be a debug message from the other ESP - print it to the serial monitor and return
    Serial.println("Received: " + tempMessage);
    return;
  }
  token = strtok(NULL, ",");
  if (token != NULL) {
    userIndex = atoi(token);
  } else {
    userIndex = -1; // Set a default value for userIndex if not provided
  }
  updateStatus(accessGranted, userIndex); //Gives feedback to user if password is correct or not through LED and LCD
}

/**
 * mainMenuKeyPad reads the keypad input in the main menu and performs actions based on the input.
 * It allows for the user to enter a password and send it to the other ESP through serial communication.
 * It also allows for the user to enter the admin menu by pressing D.
 * If the admin menu is entered, the adminMenuKeyPad function is called.
 * 
 * Parameters:
 *  - message A pointer to the message buffer.
 *  - serialMessage A pointer to the serial message buffer.
 */
void mainMenuKeyPad(char *message, char *serialMessage) {
    //int to keep track of the position in the message array
    static unsigned int messagePos = 0;
    bool keyReceived = false;
    
    //Checking for keypad input, serial input and bluetooth input
    while (!keyReceived) {
      //If there is data available to read from the serial port, read it into the serial message buffer and exit the function
      if (checkSerialCommunication(serialMessage)) {
        if (messagePos > 0) {
          messagePos = 0;
          message[messagePos] = '\0';
        }
        return;
      }

      // Check if there is data available to read from the Bluetooth serial
      if (SerialBT.available()) {
        // Read the incoming data until a newline character is encountered
        String temp = SerialBT.readStringUntil('\n');
        // Convert the read String into a character array and store it in 'message'
        temp.toCharArray(message, temp.length() + 1);
        // If there is a message position, null-terminate the message at that position
        // and reset the message position
        if (messagePos > 0) {
            message[messagePos] = '\0';
            messagePos = 0;
            // Set the flag to indicate a key has been received
            keyReceived = true;
        }
        // Print a debug message to the serial monitor
        Serial.println("Serial BT message received in mainMenuKeyPad()");
        // Send a confirmation message back over Bluetooth serial
        SerialBT.println("Received");
        // Exit the function early since a key has been received
          return;
      }

      char key = keypad.getKey(); //Read keypad input
      ledControl.readLightSensor(); //Control light sensor

      if (key) {
        ledControl.keyPressLED();
        // If a key is pressed and is not #, add it to the message. 
        if (key != '#') {
          if (menuIndex == 1 && key == '*') {
            menuIndex = 0;
            return;
          }
          if ((key == 'A' || key == 'B' || key == 'C' || key == '*')){ //Change here <- ignore letters 
            continue;
          } else if (key == 'D' && menuIndex != 0) { //If D is pressed in admin menu <- ignore letter
            continue;
          } else if ( key == 'D' && menuIndex == 0) { //If D is pressed in main manu, go to admin menu
            menuIndex = 1;
            adminMenuKeyPad(serialMessage);
            messagePos = 0;
            message[messagePos] = '\0';
          } else {  
            message[messagePos] = key;
            messagePos++;
            lcdDisplay.updatePasswordLCD();
            // if message is too long, reset the message, start over, warning!.
            if (messagePos >= MAX_MESSAGE_LENGTH) {
              messagePos = 0;
              message[messagePos] = '\0';
              updateStatus(false, 0);
              return;
            }
          }
        } else {
          // If # is pressed, end the message and reset the message position.
          if (messagePos > 0) {
            message[messagePos] = '\0';
            messagePos = 0;
            keyReceived = true;
            lcdDisplay.enterPasswordLCD("default");
          }
        }
      }
    }
}

/**
 * adminMenuKeyPad reads the keypad input in the admin menu and performs actions based on the input.
 * If a valid admin password is entered, the admin menu is entered.
 * It allows for the admin to select a user to change the password for.
 * If a new password is entered, it is sent to the other ESP through serial communication.
 * 
 * Parameters:
 *  - serialMessage A pointer to the serial message buffer to handle received serial messages from the other ESP while in the admin menu
 */
void adminMenuKeyPad(char *serialMessage) {
    lcdDisplay.enterPasswordLCD("Installer");  //LCD init for admin menu
    char message[MAX_MESSAGE_LENGTH]; //message init
    int userIndex = 0;

    //read keypad for admin access
    mainMenuKeyPad(message, serialMessage);
    unsigned long messageHash = hashPassword(message);

    //check if admin access is granted, if not, return to main menu
    if (messageHash == masterPassword) {
        menuIndex = 2;
        lcdDisplay.printUserNameLCD(userIndex); 
    }
    else if (menuIndex != 2) {
        menuIndex = 0;
        lcdDisplay.enterPasswordLCD("default");
        return;
    }
    //If access is granted, read keypad for admin menu
    while (menuIndex == 2) {
        //If there is data available to read from the serial port, read it into the serial message buffer and exit the function
        if (checkSerialCommunication(serialMessage)) {
          menuIndex = 0;
          return;
        }

        //admin menu keypad control
        char key = keypad.getKey();
        ledControl.readLightSensor();
        // Select user to change password for, * to quit, # to confirm, B and C to change user
        if (key) {
            ledControl.keyPressLED();
            // If a key is pressed and is not #, add it to the message. 
            if (key == '*') {
                menuIndex = 0;
                lcdDisplay.enterPasswordLCD("default");
            } else if (key == 'B') {
                userIndex++;
                if (userIndex >= NUM_USERS) {
                userIndex = 0;
                }
                lcdDisplay.printUserNameLCD(userIndex);
            } else if (key == 'C') {
                userIndex--;
                if (userIndex < 0) {
                userIndex = NUM_USERS-1;
                }
                lcdDisplay.printUserNameLCD(userIndex);
            } else if (key == 'D') {
                lcdDisplay.presentRFIDLCD();
                Serial2.println(String("NEW_RFID") + ","  + " " + "," + String(userIndex) + "," + String(masterPassword));
                Serial.println(String("NEW_RFID") + ","  + " " + "," + String(userIndex) + "," + String(masterPassword));
                //Wait for confirmation of RFID being scanned from other ESP
                while (!checkSerialCommunication(serialMessage) || (strstr(serialMessage, "READ_RFID") != NULL || strstr(serialMessage, "ACCESS_DENIED") != NULL)) {
                    char key = keypad.getKey();
                    if (key == '*') {
                      menuIndex = 0;
                      Serial2.println(String("CANCEL_RFID") + ","  + " " + "," + String(userIndex) + "," + String(masterPassword));
                      Serial.println(String("CANCEL_RFID") + ","  + " " + "," + String(userIndex) + "," + String(masterPassword));
                      lcdDisplay.enterPasswordLCD("default");
                      return;
                    }

                    if (strcmp(serialMessage, "LOGIN") == 0 || strcmp(serialMessage, "NEW_PASSWORD") == 0 || strcmp(serialMessage, "NEW_RFID") == 0 || strcmp(serialMessage, "CANCEL_RFID") == 0) {
                      // Ignore known commands that are not relevant
                      continue;
                    }
                    // Print any other received serial data
                    Serial.println(serialMessage);
                }
                continue;
            } else if (key == '#') {
                lcdDisplay.enterPasswordLCD("User");
                mainMenuKeyPad(message, serialMessage); // Enter new password for user
                if (message[0] != '\0') {
                  Serial2.println(String("NEW_PASSWORD") + "," + String(hashPassword(message)) + "," + String(userIndex) + "," + String(masterPassword)); // send password and user index to other ESP
                  Serial.println(String("NEW_PASSWORD") + "," + String(hashPassword(message)) + "," + String(userIndex) + "," + String(masterPassword)); // send password and user index to other ESP
                }
                menuIndex = 0;
            }
        }
    }
}

/**
 * updateStatus function updates the LED and LCD display based on whether access was granted.
 * It also prints a message to the serial monitor indicating whether the password was correct.
 * 
 * Parameters:
 *  - accessGranted A boolean indicating whether access was granted.
 *  - userIndex The index of the user for whom access was granted or denied.
 */
void updateStatus(bool accessGranted, int userIndex) {
  if (accessGranted){
    ledControl.controlLED(1);
    Serial.println("Password correct");
    lcdDisplay.accessGrantedLCD(userIndex);
    delay(1000);
    ledControl.controlLED(0);
    lcdDisplay.enterPasswordLCD("default");
  } else {
    lcdDisplay.wrongPasswordLCD();
    ledControl.controlLED(2);
    Serial.println("Password incorrect");
    lcdDisplay.enterPasswordLCD("default");
    ledControl.controlLED(0);
  }
}


/**
 * checkSerialCommunication checks if there is data available to read from the serial port.
 * If data is available, it reads the incoming message until a newline character is encountered.
 * It then prints a debug message to the serial monitor.
 * 
 * Parameters:
 * - serialMessage A pointer to the serial message buffer.
 * Returns: 
 * - true if a message was received, false otherwise.
 */
bool checkSerialCommunication(char* serialMessage) {
    if (Serial2.available()) {
        Serial2.readBytesUntil('\n', serialMessage, MAX_SERIAL_MESSAGE_LENGTH);
        Serial.println("Serial2 message received.");
        return true;
    }
    return false;
}