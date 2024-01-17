/*
  Simple hash function for passwords. 
  Uses the djb2 algorithm. https://theartincode.stanis.me/008-djb2/
  Written by Andreas Nielsen (s203833)
  Estimated workshare: Andreas Nielsen (s203833) 100%
*/
#ifndef Hash_h
#define Hash_h

#include "Arduino.h"


unsigned long hashPassword(const char* password) {
    unsigned long hash = 5381;
    int c;

    while (c = *password++)
        hash = ((hash << 5) + hash) + c; // hash * 33 + c 

    return hash;
}

#endif