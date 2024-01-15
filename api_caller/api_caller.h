#ifndef API_CALLER_H
#define API_CALLER_H

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

class ApiCaller
{
public:
    ApiCaller(WiFiClient &client, String apiUrl);
    std::unique_ptr<DynamicJsonDocument> GET(String endpoint, String value);
    std::unique_ptr<DynamicJsonDocument> PUT(String endpoint, String id, String value);
    String format_url(String url);

private:
    WiFiClient &client;
    String apiUrl;
};

#endif