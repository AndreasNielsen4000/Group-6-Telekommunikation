/*
* LCD display class and methods for the keypad
* with custom characters for the arrows and lock/unlock symbols
* Written by Alexander Nordentoft (s176361) and Andreas Nielsen (s203833) - January 2024
* Estimated workshare: Andreas Nielsen (s203833) 60% and Alexander Nordentoft (s176361) 40%.
*/
#include "lcd_display.h"

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 row

// Custom characters for LCD
byte Lock[] = {
  B01110,
  B10001,
  B10001,
  B11111,
  B11011,
  B11011,
  B11111,
  B00000
};

byte unLock[] = {
  B01110,
  B00001,
  B00001,
  B11111,
  B11011,
  B11011,
  B11111,
  B00000
};

byte arrowUp[] = {
  B00000,
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00000,
  B00000
};

byte arrowDown[] = {
  B00000,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
  B00000,
  B00000
};

byte arrowLeft[] = {
  B00000,
  B00100,
  B01000,
  B11111,
  B01000,
  B00100,
  B00000,
  B00000
};

byte arrowRight[] = {
  B00000,
  B00100,
  B00010,
  B11111,
  B00010,
  B00100,
  B00000,
  B00000
};

byte rfidSymbol[] = {
  B01110,
  B11111,
  B11111,
  B11111,
  B01100,
  B01111,
  B01100,
  B01111
};

/*
* Constructor for the LCD display
*
*/
lcd_display::lcd_display() {  
}

// initialize the LCD
void lcd_display::init() {
  lcd.init();                   
  lcd.backlight();
  lcd.createChar(0, arrowUp);
  lcd.createChar(1, arrowDown);
  lcd.createChar(2, arrowLeft);
  lcd.createChar(3, arrowRight);
  lcd.createChar(4, rfidSymbol);
  lcd.createChar(6, unLock);
  lcd.createChar(7, Lock);
}

//Prints the main menu on the LCD
void lcd_display::enterPasswordLCD(char user[]) {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (user == "User") { //Different password prompts
    lcd.print("User password:");
  } else if (user == "Installer") {
    lcd.print("Inst. password:");
  } else {
    lcd.print("Enter password:");
  }
  lcd.setCursor(0, 1);// start blinking cursor on next line
  lcd.blink();              
}

//Prints * symbols on the LCD when entering a password
void lcd_display::updatePasswordLCD() {
  lcd.print("*");
}

//Access Denied message on the LCD
void lcd_display::wrongPasswordLCD() {
  lcd.clear();                 
  lcd.setCursor(0, 0);         
  lcd.print("Access denied"); 
  lcd.setCursor(0, 1);         
  for (int i = 0; i < 16; i++) {
    lcd.write(7);
  }
  lcd.blink_off();
}

//Access Granted message on the LCD
void lcd_display::accessGrantedLCD(int userIndex) {
  lcd.clear();                
  lcd.setCursor(0, 0);         
  lcd.print("Welcome User "); 
  lcd.print(userIndex + 1);
  lcd.setCursor(0, 1);         
  for (int i = 0; i < 16; i++) {
    lcd.write(6);
  lcd.blink_off();
  }
}

//admin menu LCD print
void lcd_display::adminMenuLCD(int userIndex) {
  lcd.clear();                 
  lcd.setCursor(0, 0);       
  lcd.print("User "); 
  lcd.print(userIndex + 1);
  lcd.setCursor(0, 1);        
  lcd.print(" ");
  lcd.print(" ");
  lcd.write(0);
  lcd.print("B");
  lcd.print(" ");
  lcd.write(1);
  lcd.print("C");
  lcd.print(" ");
  lcd.write(2);
  lcd.print("*");
  lcd.print(" ");
  lcd.write(3);
  lcd.print("#");
  lcd.print(" ");
  lcd.write(4);
  lcd.print("D");
  lcd.blink_off();
}

//RFID prompt
void lcd_display::presentRFIDLCD() {
  lcd.clear();  
  lcd.setCursor(0, 0); 
  lcd.print("Present RFID");
  lcd.setCursor(0, 1); 
  lcd.print("card");
  lcd.blink_off();
}

