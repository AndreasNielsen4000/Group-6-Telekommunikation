#include "led_control.h"



led_control::led_control(int redPin, int greenPin, int bluePin, int lightPin, int ldrPin, int buzzerPin) : 
              PIN_RED(redPin), PIN_GREEN(greenPin), PIN_BLUE(bluePin), PIN_LIGHT(lightPin), LDRPin(ldrPin), buzzerPin(buzzerPin) {
}

void led_control::init() {
  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);
  controlLED(0);
  pinMode(PIN_LIGHT, OUTPUT);
  pinMode(LDRPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  //tone(buzzerPin, 698, 200);
  //delay(100);
  //tone(buzzerPin, 698, 200);
}

void led_control::controlLED(int status) {
  switch(status){
    case 0: //Idle LED status
      analogWrite(PIN_RED,   0);
      analogWrite(PIN_GREEN, 0);
      analogWrite(PIN_BLUE,  255);
      break;
    case 1: //Access Granted LED status
      analogWrite(PIN_RED,   0);
      analogWrite(PIN_GREEN, 255);
      analogWrite(PIN_BLUE,  0);
      //tone(buzzerPin, 523, 500);
      //tone(buzzerPin, 523, 500);
      break;
    case 2: //Wrong Password LED status
      for (int i = 0; i < 5; i++){
        analogWrite(PIN_RED,   255);
        analogWrite(PIN_GREEN, 0);
        analogWrite(PIN_BLUE,  0);
        //tone(buzzerPin, 262, 300);
        delay(300);
        analogWrite(PIN_RED,   0);
        analogWrite(PIN_GREEN, 0);
        analogWrite(PIN_BLUE,  0);
        //tone(buzzerPin, 262, 300);
        delay(300);
      }
      break;
    case 9: //LED off status
      analogWrite(PIN_RED,   0);
      analogWrite(PIN_GREEN, 0);
      analogWrite(PIN_BLUE,  0);
      break;
    default: //Idle LED status
      analogWrite(PIN_RED,   0);
      analogWrite(PIN_GREEN, 0);
      analogWrite(PIN_BLUE,  255);
      break;
    
  }
}


void led_control::readLightSensor() {
  int lightValue = analogRead(LDRPin);
  if (lightValue < 2500){
    digitalWrite(PIN_LIGHT, HIGH);
  } else if (lightValue > 3000) {
    digitalWrite(PIN_LIGHT, LOW);
  }
}


void led_control::keyPressLED() {
    tone(buzzerPin, 698, 20);
    controlLED(9);
    delay(20);
    controlLED(0);
}
