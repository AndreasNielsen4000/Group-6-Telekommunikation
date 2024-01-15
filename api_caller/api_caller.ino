#include "api_caller.h"
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

/* Example ino file - calling server*/

const char *ssid = "";   /* Your SSID */
const char *pass = "";   /* Your Password */
const char *apiUrl = ""; /* Your API URL. Example: "https://api.example.com" */

void setup(void)
{

    Serial.begin(115200);

    // Wait for serial to initialize.
    // FIXME: There should be a better way to do this.
    delay(1000);

    Serial.println("Starting API Caller example...");

    WiFi.begin(ssid, pass);

    // Waiting for WIFI connection
    Serial.println("WIFI connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.print("\n\nConnected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Not connected to WiFi...");
        delay(1000);
        return;
    }

    // Create a WiFiClient object to talk to the server.
    WiFiClient client;

    // Create an ApiCaller object with the WiFiClient and API URL.
    ApiCaller apiCaller(client, apiUrl);

    // Call the API with a pin
    String pin = "1234";
    std::unique_ptr<DynamicJsonDocument> doc = apiCaller.GET("/pin", pin);

    // Check if the API call was successful.
    if (doc == nullptr)
    {
        Serial.println("Error calling API");
        delay(1000);
        return;
    }

    if (doc->containsKey("authenticated"))
    {
        if ((*doc)["authenticated"].as<bool>())
        {
            Serial.println("API call was successful and the pin was correct.");

            // Get the value of the "message" key.
            // TODO: check if the key exists before getting the value.
            String message = (*doc)["message"].as<String>();
            Serial.println(message);
        }
        else
        {
            Serial.println("API call was successful, but the pin was incorrect.");
        }
    }
    else
    {
        Serial.println("API call was successful, but the response was not valid.");
        delay(1000);
        return;
    }

    delay(60000);
}