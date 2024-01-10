/*
  Simple hash function for passwords. At the moment, it just adds up the values of the characters in the password.
  This is not a secure hash function, but it is good enough for this project.
*/
#ifndef Hash_h
#define Hash_h

#include "Arduino.h"


unsigned long hashPassword(const char* password) {
    unsigned long hash = 0;
    for (int i = 0; password[i] != '\0'; i++) {
        hash += password[i];
    }
    return hash;
}

#endif