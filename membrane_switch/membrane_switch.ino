#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include "Hash.h"
#include <ThingSpeak.h>

const int PIN_RED   = 11;
const int PIN_GREEN = 12;
const int PIN_BLUE  = 13;
const int PIN_LIGHT = 10;
const int LDRPin = A0;

const int ROW_NUM = 4; //four rows
const int COLUMN_NUM = 4; //four columns
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2

#define MAX_MESSAGE_LENGTH 9

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};

byte pin_rows[ROW_NUM] = {9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {5, 4, 3, 2}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );
unsigned long password = hashPassword("1234");


void setup() {
  Serial.begin(9600);
  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);
  controlLED(0);
  pinMode(PIN_LIGHT, OUTPUT);
  pinMode(LDRPin, INPUT);
  lcd.init(); // initialize the lcd
  lcd.backlight();
  enterPasswordLCD();
}

void loop() {
  char message[MAX_MESSAGE_LENGTH];
  readKeyPad(message);
  unsigned long messageHash = hashPassword(message);
  controlLED(0);
  if (messageHash == password){
    updateStatus(true);
  } else {
    updateStatus(false);
  }
  Serial.println(message);
}

void readKeyPad(char *message) {
    //int to keep track of the position in the message array
    static unsigned int message_pos = 0;
    bool keyReceived = false;
    
    // Read the keypad until a # character is received.
    while (!keyReceived) {
      updateFromTS();
      char key = keypad.getKey();
      readLightSensor();
      if (key) {
        controlLED(9);
        delay(20);
        controlLED(0);
        // If a key is pressed and is not #, add it to the message. 
        if (key != '#') {
          message[message_pos] = key;
          message_pos++;
          updatePasswordLCD();
          // if message is too long, reset the message, start over, warning!.
          if (message_pos >= MAX_MESSAGE_LENGTH) {
            message[message_pos] = '\0';
            message_pos = 0;
            updateStatus(false);
          }
        } else {
          // If # is pressed, end the message and reset the message position.
          message[message_pos] = '\0';
          int messageSize = sizeof(message) - 1;
          message_pos = 0;
          keyReceived = true;
          enterPasswordLCD();
        }
      }
    }
}


void controlLED(int status){
  switch(status){
    case 0:
      analogWrite(PIN_RED,   0);
      analogWrite(PIN_GREEN, 0);
      analogWrite(PIN_BLUE,  255);
      break;
    case 1:
      analogWrite(PIN_RED,   0);
      analogWrite(PIN_GREEN, 255);
      analogWrite(PIN_BLUE,  0);
      break;
    case 2:
      for (int i = 0; i < 5; i++){
        analogWrite(PIN_RED,   255);
        analogWrite(PIN_GREEN, 0);
        analogWrite(PIN_BLUE,  0);
        delay(300);
        analogWrite(PIN_RED,   0);
        analogWrite(PIN_GREEN, 0);
        analogWrite(PIN_BLUE,  0);
        delay(300);
      }
      break;
    case 9:
      analogWrite(PIN_RED,   0);
      analogWrite(PIN_GREEN, 0);
      analogWrite(PIN_BLUE,  0);
      break;
    default:
      analogWrite(PIN_RED,   0);
      analogWrite(PIN_GREEN, 0);
      analogWrite(PIN_BLUE,  255);
      break;
    
  }
}



void readLightSensor(){
  int lightValue = analogRead(LDRPin);
  if (lightValue < 850){
    digitalWrite(PIN_LIGHT, HIGH);
  } else {
    digitalWrite(PIN_LIGHT, LOW);
  }
}

void enterPasswordLCD(){
  lcd.clear();                 // clear display
  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  lcd.print("Enter password:"); // print message at (0, 0)
  lcd.setCursor(0, 1);         // move cursor to   (0, 1)
  lcd.blink();              // start blinking cursor
}

void updatePasswordLCD(){
  lcd.print("*");
}

void wrongPasswordLCD(){
  lcd.clear();                 // clear display
  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  lcd.print("Access denied"); // print message at (0, 0)
  lcd.setCursor(0, 1);         // move cursor to   (0, 1)
}

void accessGrantedLCD(){
  lcd.clear();                 // clear display
  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  lcd.print("Access granted"); // print message at (0, 0)
  lcd.setCursor(0, 1);         // move cursor to   (0, 1)
}

void passTooLongLCD(){
  lcd.clear();                 // clear display
  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  lcd.print("Password"); // print message at (0, 0)
  lcd.setCursor(0, 1);         // move cursor to   (0, 1)
  lcd.print("too long!");
}

void updateStatus(bool accessGranted){
  if (accessGranted){
    controlLED(1);
    Serial.println("Password correct");
    accessGrantedLCD();
    delay(1000);
    controlLED(0);
    enterPasswordLCD();
  } else {
    wrongPasswordLCD();
    controlLED(2);
    Serial.println("Password incorrect");
    enterPasswordLCD();
    controlLED(0);
  }
}

void updateToTS(char *message){
  
}

void updateFromTS(){
  
}