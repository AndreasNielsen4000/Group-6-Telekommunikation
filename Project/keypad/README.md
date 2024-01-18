# Code for Keypad on ESP

## serial_test
The serial test code is to simulate the serial communication between the RFID and Keypad ESPs


## Keypad_ESP
This is the code for the keypad


## Serial Communication between ESPs

### From Keypad
Syntax: `<COMMAND>,<hashedPassword>(optional),<userIndex>(optional),<hashedMasterPassword>(optional)`

#### Examples:
##### User Login:
Keypad Sends:
`LOGIN,<hashedPassword>, , `

##### New User Password:
Keypad Sends:
`NEW_PASSWORD,<hashedPassword>,<userIndex>,<hashedMasterPassword>`

##### New User RFID:
Keypad Sends:
`NEW_RFID, ,<userIndex>,<hashedMasterPassword>`

Cancel RFID:
`CANCEL_RFID, , , `


### To Keypad
Syntax: `<COMMAND>,<userIndex>(optional)`

#### Examples:
##### User Has Logged In:
RFID Sends:
`ACCESS,<userIndex>`

##### Wrong Password:
RFID Sends:
`ACCESS_DENIED,`

##### New User RFID created:
Keypad Sends:
`RFID_READ,`
