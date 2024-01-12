#include "lcd_display.h"
/*
Function for the LCD display. 
*/


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

/*
* Constructor for the LCD display
*
*/
lcd_display::lcd_display() {  
}


void lcd_display::init() {
  // initialize the LCD
  lcd.init();                   
  lcd.backlight();
  lcd.createChar(0, arrowUp);
  lcd.createChar(1, arrowDown);
  lcd.createChar(2, arrowLeft);
  lcd.createChar(3, arrowRight);
  lcd.createChar(6, unLock);
  lcd.createChar(7, Lock);
}


void lcd_display::enterPasswordLCD(char user[]) {
  lcd.clear();                 // clear display
  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  if (user == "User") {
    lcd.print("User password:"); // print message at (0, 0)
  } else if (user == "Installer") {
    lcd.print("Inst. password:"); // print message at (0, 0)
  } else {
    lcd.print("Enter password:"); // print message at (0, 0)
  }
  lcd.setCursor(0, 1);         // move cursor to   (0, 1)
  lcd.blink();              // start blinking cursor
}


void lcd_display::updatePasswordLCD() {
  lcd.print("*");
}


void lcd_display::wrongPasswordLCD() {
  lcd.clear();                 // clear display
  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  lcd.print("Access denied"); // print message at (0, 0)
  lcd.setCursor(0, 1);         // move cursor to   (0, 1)
  for (int i = 0; i < 16; i++) {
    lcd.write(7);
  }
  lcd.blink_off();
}


void lcd_display::accessGrantedLCD(int userIndex) {
  lcd.clear();                 // clear display
  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  lcd.print("Welcome User "); // print message at (0, 0)
  lcd.print(userIndex);
  lcd.setCursor(0, 1);         // move cursor to   (0, 1)¨
  for (int i = 0; i < 16; i++) {
    lcd.write(6);
  lcd.blink_off();
  }
}


void lcd_display::passTooLongLCD() {
  lcd.clear();                 // clear display
  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  lcd.print("Password"); // print message at (0, 0)
  lcd.setCursor(0, 1);         // move cursor to   (0, 1)
  lcd.print("too long!");
  lcd.blink_off();
}


void lcd_display::printUserNameLCD(struct accessCredentials *credentialsList, int userIndex) {
  lcd.clear();                 // clear display
  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  lcd.print("User "); // print message at (0, 0)
  lcd.print(userIndex+1);
  lcd.setCursor(0, 1);         // move cursor to   (0, 1)
  lcd.print(" ");
  lcd.write(0);
  lcd.print("B");
  lcd.print(" ");
  lcd.print(" ");
  lcd.write(1);
  lcd.print("C");
  lcd.print(" ");
  lcd.print(" ");
  lcd.write(2);
  lcd.print("*");
  lcd.print(" ");
  lcd.print(" ");
  lcd.write(3);
  lcd.print("#");
  lcd.blink_off();
}

