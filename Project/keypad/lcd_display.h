#ifndef lcd_display_h
#define lcd_display_h

#include <LiquidCrystal_I2C.h>
#include "Arduino.h"


extern LiquidCrystal_I2C lcd;

class lcd_display
{
private:

public:
    lcd_display();
    void init();
    // maybe change user to adminAccess boolean
    void enterPasswordLCD(char user[]);
    void updatePasswordLCD();
    void wrongPasswordLCD();
    void accessGrantedLCD(int userIndex);
    void passTooLongLCD();
    void printUserNameLCD(int userIndex);
};


#endif