# api caller

This is a simple api caller that can be used to call any api. It is a simple wrapper around the requests libraries.

## Installation

1. Choose the board configuration `NodeMCU 0.9 (ESP-12 Module)` for the ESP8266

2. Download the library `arduinoJson`

3. Ensure that the server is running on the same network as the ESP8266 or that the server is accessible from the internet. Look in the `server` directory for more information.

## Usage

Include the header file:
```cpp
#include <api_caller.h>
```

Create an instance of the api caller:
```cpp
WiFiClient client;
String apiUrl = "http://example.com";
ApiCaller apiCaller(client, apiUrl);
```


Then you can call the api:
```cpp
apiCaller.call("pin", "1");
```

This will call the api with the following url `http://example.com/pin/1` and if the api returns a json object it will be parsed and returned as a unique pointer to an `DynamicJsonDocument`.
