#include "Hash.h"

#define RXD2 15
#define TXD2 13

#define MAX_MESSAGE_LENGTH 32
#define NUM_USERS 5

typedef struct accessCredentials
{
    unsigned long password;
    char username[15];
};

accessCredentials credentialsList[NUM_USERS] = {
        {hashPassword("1234"), "user1"},
        {hashPassword("\0"), "user2"},
        {hashPassword("\0"), "user3"},
        {hashPassword("\0"), "user4"},
        {hashPassword("\0"), "user5"},
};

unsigned long masterPassword = hashPassword("12345678");

void setup(){
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

//create currentMillis and previousMillis variables
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
const long interval = 10000;

void loop(){
    char input[MAX_MESSAGE_LENGTH] = {'\0'};
    unsigned long receivedPassword = 0;
    int receivedUserIndex = -1;
    
    if (Serial.available()) {
        Serial.readBytesUntil('\n', input, MAX_MESSAGE_LENGTH);
        Serial2.println(input);
    }
    
    if (Serial2.available()) {
        Serial2.readBytesUntil('\n', input, MAX_MESSAGE_LENGTH);
        splitSerialMessage(input, &receivedPassword, &receivedUserIndex);
        if (receivedUserIndex < 0) {
            for (int i = 0; i < NUM_USERS; i++) {
                if (credentialsList[i].password == receivedPassword) {
                    Serial2.println("1," + String(i));
                    break;
                }
            }
            Serial2.println("0,-1");
        } else if (receivedUserIndex < NUM_USERS && receivedUserIndex >= 0) {
            credentialsList[receivedUserIndex].password = receivedPassword; //update password
        } else {
            Serial.println("Invalid user index");
        }
        Serial.println(input);
    }
    currentMillis = millis();

    //print out the users and their passwords every 10 seconds for debugging
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        for (int i = 0; i < NUM_USERS; i++) {
            Serial.println("User " + String(i) + ": " + String(credentialsList[i].password));
        }

    }
}


void splitSerialMessage(char *serialMessage, unsigned long *password, int *userIndex) {
  char *token = strtok(serialMessage, ",");
  char *endPointer;
  *password = strtoul(token, &endPointer, 10);
  //if we only receive password and no communication from ESP, set userIndex to -1
    if (token == NULL) {
        *userIndex = -1;
        return;
    } else { //if we receive a comma, should receive a user index and a hashed master password, we should check if the master password is correct. If it is correct, return password and user index.
        token = strtok(NULL, ",");
        if (token == NULL) {
            *userIndex = -1;
            return;
        } else {
            *userIndex = atoi(token);
            token = strtok(NULL, ",");
            if (token == NULL) {
                *userIndex = -1;
                return;
            } else {
                if (strtoul(token, &endPointer, 10) == masterPassword) {
                    return;
                } else {
                    *userIndex = -1;
                    return;
                }
            }
        }
    }
}
