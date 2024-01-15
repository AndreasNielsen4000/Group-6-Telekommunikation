#include <Keypad.h>
#include "Hash.h"
#include "lcd_display.h"
#include "led_control.h"
#include <BleSerial.h>

// Constants
#define MAX_MESSAGE_LENGTH 9
#define MAX_SERIAL_MESSAGE_LENGTH 50
#define NUM_USERS 5
#define RXD2 13 //define RX pin for serial2 communication (between ESPs)
#define TXD2 15 //define TX pin for serial2 communication (between ESPs)

// Pins for LED, light sensor and buzzer
const int PIN_RED   = 4;
const int PIN_GREEN = 14;
const int PIN_BLUE  = 27;
const int PIN_LIGHT = 23;
const int LDRPin = 36;
const int buzzerPin = 5;

// Variables for user and menu index
int menuIndex = 0;
int userIndex = 0;

// Function declarations
void adminMenuKeyPad(char *serialMessage);

// Rows and columns for keypad and keypad initialization
const int ROW_NUM = 4; 
const int COLUMN_NUM = 4;

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};

byte pin_rows[ROW_NUM] = {16, 17, 18, 19}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {25, 26, 32, 33}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

// Master password for admin access
unsigned long adminPassword = hashPassword("12345678");

// Object declarations for LCD display, LED control and Bluetooth serial communication
lcd_display lcdDisplay;
led_control ledControl(PIN_RED, PIN_GREEN, PIN_BLUE, PIN_LIGHT, LDRPin, buzzerPin);
BleSerial SerialBT;

//Setup function for Arduino
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  //************** TEST *******************
  SerialBT.begin("KeypadBT"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
  //****************************************
  lcdDisplay.init();
  lcdDisplay.enterPasswordLCD("default"); //initial menu on lcd
  ledControl.init();
}

//Loop function for Arduino
void loop() {
  char message[MAX_MESSAGE_LENGTH] = {'\0'}; //message array for keypad input
  char serialMessage[MAX_SERIAL_MESSAGE_LENGTH] = {'\0'}; //message array for serial input
  bool passFound = false; //bool for checking if password is found

  //Serial communication check
  if (Serial2.available()) {
      Serial2.readBytesUntil('\n', serialMessage, MAX_SERIAL_MESSAGE_LENGTH);
      Serial.println("Serial message received in loop()");
      return;
  }

  //Start main menu keypad function, changes the message array with keypad inputs
  mainMenuKeyPad(message, serialMessage);
  
  unsigned long messageHash = hashPassword(message);   
  ledControl.controlLED(0); //Set LED to idle status
  
  //If messageHash is not equal to the hash of an empty string, send the password to the other ESP through serial
  if (messageHash != hashPassword("\0")) {
    Serial2.println(messageHash); //send password to ESP to open door
  }

  // If serialMessage is not empty, split the serial message from other ESP into message, accessGranted and userIndex
  if (serialMessage[0] != '\0' && serialMessage[0] != '\r') {
    splitSerialMessage(serialMessage, &passFound, &userIndex);
    Serial.println("MESSAGE: ");
    Serial.println(serialMessage);
    Serial.println("PASSFOUND: ");
    Serial.println(passFound);
    Serial.println("USERINDEX: ");
    Serial.println(userIndex);
    updateStatus(passFound, userIndex); //Gives feedback to user if password is correct or not through LED and LCD
  }
}

//Function for splitting serial message into accessGranted and userIndex
void splitSerialMessage(char *serialMessage, bool *accessGranted, int *userIndex) {
  char *token = strtok(serialMessage, ",");
  *accessGranted = atoi(token);
  token = strtok(NULL, ",");
  *userIndex = atoi(token);
}

void mainMenuKeyPad(char *message, char *serialMessage) {
    //int to keep track of the position in the message array
    static unsigned int message_pos = 0;
    bool keyReceived = false;
    
    //Checking for keypad input, serial input and bluetooth input
    while (!keyReceived) {

      //Serial communication check
      if (Serial2.available()) {
        Serial2.readBytesUntil('\n', serialMessage, MAX_SERIAL_MESSAGE_LENGTH);
        Serial.println("Serial message received in mainMenuKeyPad()");
        return;
      }
      
      //Bluetooth communication check
      if (SerialBT.available()) {
        String temp = SerialBT.readStringUntil('\n');
        temp.toCharArray(message, temp.length() + 1);
        if (message_pos > 0) {
            message[message_pos] = '\0';
            message_pos = 0;
            keyReceived = true;
        }
        Serial.println("Serial BT message received in mainMenuKeyPad()");
        SerialBT.println("Received");
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
            message_pos = 0;
            message[message_pos] = '\0';
          } else {  
            message[message_pos] = key;
            message_pos++;
            lcdDisplay.updatePasswordLCD();
            // if message is too long, reset the message, start over, warning!.
            if (message_pos >= MAX_MESSAGE_LENGTH) {
              message_pos = 0;
              message[message_pos] = '\0';
              updateStatus(false, 0);
              return;
            }
          }
        } else {
          // If # is pressed, end the message and reset the message position.
          if (message_pos > 0) {
            message[message_pos] = '\0';
            message_pos = 0;
            keyReceived = true;
            lcdDisplay.enterPasswordLCD("default");
          }
        }
      }
    }
}


void adminMenuKeyPad(char *serialMessage) {
    lcdDisplay.enterPasswordLCD("Installer");  //LCD init for admin menu
    char message[MAX_MESSAGE_LENGTH]; //message init
    int userIndex = 0;

    //read keypad for admin access
    mainMenuKeyPad(message, serialMessage);
    unsigned long messageHash = hashPassword(message);

    //check if admin access is granted, if not, return to main menu
    if (messageHash == adminPassword) {
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
        //Serial communication check
        if (Serial2.available()) {
          Serial2.readBytesUntil('\n', serialMessage, MAX_SERIAL_MESSAGE_LENGTH);
          Serial.println("Serial message received in adminMenuKeyPad()");
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
                if (userIndex >= 5) {
                userIndex = 0;
                }
                lcdDisplay.printUserNameLCD(userIndex);
            } else if (key == 'C') {
                userIndex--;
                if (userIndex < 0) {
                userIndex = 4;
                }
                lcdDisplay.printUserNameLCD(userIndex);
            } else if (key == '#') {
                lcdDisplay.enterPasswordLCD("User");
                mainMenuKeyPad(message, serialMessage); // Enter new password for user
                if (message[0] != '\0') {
                  Serial2.println(String(hashPassword(message)) + "," + String(userIndex) + "," + String(adminPassword)); // send password and user index to other ESP
                }
                menuIndex = 0;
            }
        }
    }
}


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
