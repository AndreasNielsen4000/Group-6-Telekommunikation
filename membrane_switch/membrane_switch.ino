#include <Keypad.h>
#include "Hash.h"
#include "lcd_display.h"
#include "led_control.h"
//*************************** TEST ***************************
#include <BleSerial.h>
//************************************************************


// Constants
#define MAX_MESSAGE_LENGTH 9
#define MAX_SERIAL_MESSAGE_LENGTH 50
#define NUM_USERS 5
#define RXD2 13 //define RX pin for serial2 communication (between ESPs)
#define TXD2 15 //define TX pin for serial2 communication (between ESPs)

// Pins
const int PIN_RED   = 4;
const int PIN_GREEN = 14;
const int PIN_BLUE  = 27;
const int PIN_LIGHT = 23;
const int LDRPin = 36;
const int buzzerPin = 5;

// Variables for user and menu index
int menuIndex = 0;
int userIndex = 0;

//***************************** TEST ***************************
BleSerial SerialBT;
//************************************************************

// Struct for storing access credentials
/*
typedef struct accessCredentials
{
    unsigned long password;
    char username[15];
};
*/
// Function declarations
/*
void updatePasswordList(accessCredentials *credentialsList);
void cleanAccessCredentials(accessCredentials *credentialsList);
*/
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

// Credentials list manual 
/*
accessCredentials credentialsList[NUM_USERS] = {
        {hashPassword("1234"), "user1"},
        {hashPassword("\0"), "user2"},
        {hashPassword("\0"), "user3"},
        {hashPassword("\0"), "user4"},
        {hashPassword("\0"), "user5"},
};
*/


// Object declarations for LCD display and LED control
lcd_display lcdDisplay;
led_control ledControl(PIN_RED, PIN_GREEN, PIN_BLUE, PIN_LIGHT, LDRPin, buzzerPin);

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

  if (Serial2.available()) {
      Serial2.readBytesUntil('\n', serialMessage, MAX_SERIAL_MESSAGE_LENGTH);
      Serial.println("Serial message received in loop()");
      return;
  }
  mainMenuKeyPad(message, serialMessage); //Start main menu keypad function, returns message array from keypad input
  
  unsigned long messageHash = hashPassword(message);   
  ledControl.controlLED(0); //Set LED to idle status
  //******************* TEST ********************
  if (SerialBT.available()) {
    messageHash = hashPassword(SerialBT.readStringUntil('\n').c_str());
    SerialBT.println("Received");
  }
  //*********************************************
  if (messageHash != hashPassword("\0")) {
    Serial2.println(messageHash); //send password to ESP to open door
  }

  
  if (serialMessage[0] != '\0' && serialMessage[0] != '\r') {
    splitSerialMessage(serialMessage, &passFound, &userIndex);
    Serial.println("MESSAGE: ");
    Serial.println(serialMessage);
    Serial.println("PASSFOUND: ");
    Serial.println(passFound);
    Serial.println("USERINDEX: ");
    Serial.println(userIndex);
    updateStatus(passFound, userIndex);
  }
}

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
    
    // Read the keypad until a # character is received.
    while (!keyReceived) {
      if (Serial2.available()) {
        Serial2.readBytesUntil('\n', serialMessage, MAX_SERIAL_MESSAGE_LENGTH);
        Serial.println("Serial message received in mainMenuKeyPad()");
        return;
      }
      
      //******************* TEST ********************
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
      //*********************************************

      char key = keypad.getKey();
      ledControl.readLightSensor();
      if (key) {
        ledControl.keyPressLED();
        // If a key is pressed and is not #, add it to the message. 
        if (key != '#') {
          if (menuIndex == 1 && key == '*') {
            return;
          }
          if ((key == 'A' || key == 'B' || key == 'C' || key == '*')){ //Change here <- ignore letters 
            continue;
          } else if ( key == 'D' ) {
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
    //LCD init
    lcdDisplay.enterPasswordLCD("Installer");
    //message init
    char message[MAX_MESSAGE_LENGTH];
    static unsigned int message_pos = 0;

    //read keypad for admin access
    mainMenuKeyPad(message, serialMessage);
    
    unsigned long messageHash = hashPassword(message);

    //check if admin access is granted
    int userIndex = 0;
    if (messageHash == adminPassword) {
        menuIndex = 2;
        lcdDisplay.printUserNameLCD(userIndex); 
    }
    else if (menuIndex != 2) {
        lcdDisplay.enterPasswordLCD("default");
        return;
    }
    while (menuIndex == 2) {
        if (Serial2.available()) {
          Serial2.readBytesUntil('\n', serialMessage, MAX_SERIAL_MESSAGE_LENGTH);
          Serial.println("Serial message received in adminMenuKeyPad()");
          return;
        }
        char key = keypad.getKey();
        ledControl.readLightSensor();
        if (key) {
            ledControl.keyPressLED();
            // If a key is pressed and is not #, add it to the message. 
            if (key == '*') {
                menuIndex = 0;
                message_pos = 0;
                message[message_pos] = '\0';
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
                mainMenuKeyPad(message, serialMessage);
                //credentialsList[userIndex].password = hashPassword(message); // SKAL VEL IKKE BRUGES LÆNGERE
                //Send password, user index and hashed master password to ESP in one string separated by commas
                Serial2.println(String(hashPassword(message)) + "," + String(userIndex) + "," + String(adminPassword));
                //updatePasswordList(credentialsList); // HELLERE IKKE BRUGES LÆNGERE!!
                menuIndex = 0;
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
                }
            }
        }
    }
}
/*
lav struct tempCredentialsList (som global) og gem EEPROM i credentialsList. 
Programmet skriver alt aktivt i tempCredentialsList og gemmer i EEPROM hvis credentialsList ikke er lig tempCredentialsList.
*/

// VI SKAL DA IKKE BRUGE DEN HER LÆNGERE!!??
/*
void updatePasswordList(struct accessCredentials *credentialsList) {

    accessCredentials tempCredentialsList[NUM_USERS];

    memcpy(tempCredentialsList, credentialsList, sizeof(accessCredentials) * NUM_USERS);

    cleanAccessCredentials(credentialsList);

    for (int i = 0; i < NUM_USERS; i++) {
      if (tempCredentialsList[i].password != '\0')
          credentialsList[i] = tempCredentialsList[i];
    }
}

void cleanAccessCredentials(struct accessCredentials *credentialsList) {
    accessCredentials credentialsReset = {'\0', ""};

    for (int i = 0; i < NUM_USERS; i++) {
      credentialsList[i] = credentialsReset;
    }
}
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
