#ifndef led_control_h
#define led_control_h

#include "Arduino.h"

class led_control
{
private:
    /* data */
    // Pins
    const int PIN_RED;
    const int PIN_GREEN;
    const int PIN_BLUE;
    const int PIN_LIGHT;
    const int LDRPin;
    const int buzzerPin;
public:
    led_control(int redPin, int greenPin, int bluePin, int lightPin, int ldrPin, int buzzerPin);
    void init();
    void controlLED(int status);
    void readLightSensor();
    void keyPressLED();
};


#endif
