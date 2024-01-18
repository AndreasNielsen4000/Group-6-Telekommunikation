#include "api_caller.h"
#include <iostream>
#include <string>
#include <ArduinoJson.h>

/*
 * Constructor for ApiCaller class.
 * Initializes the class with a reference to a WiFiClient and the API URL.
 * @param client Reference to a WiFiClient object.
 * @param apiUrl URL of the API to be called. Example: "http://api.example.com"
 */
ApiCaller::ApiCaller(WiFiClient &client, String apiUrl) : client(client), apiUrl(apiUrl) // Initializer member variables
{
}

/*
 * Makes a call to a specified endpoint of the API with a given value.
 * Constructs the full URL, sends the request, and processes the response.
 *
 * @param endpoint The specific endpoint of the API to be called.
 * @param value The value to be passed to the endpoint.
 * @return A unique_ptr to a DynamicJsonDocument containing the response, or nullptr if an error occurs.
 */
std::unique_ptr<DynamicJsonDocument> ApiCaller::GET(String endpoint, String value)
{
// TODO: check if there is a "/" in between all values
#include <string>

    String url = this->format_url(this->apiUrl + "/" + endpoint + "/" + value);

    Serial.print("Calling API: ");
    Serial.println(url);

    HTTPClient https;

    https.begin(client, url.c_str());
    https.addHeader("Content-Type", "application/json");

    int httpCode = https.GET();
    if (httpCode <= 0)
    {
        Serial.println("Error on HTTP request");
        return nullptr;
    }

    String payload = https.getString();
    // TODO: figure out how big DynamicJsonDocument should be
    std::unique_ptr<DynamicJsonDocument> doc = std::make_unique<DynamicJsonDocument>(payload.length() * 4);

    // cleanup the https connection
    https.end();

    // deserialize the JSON payload
    DeserializationError jsonError = deserializeJson(*doc, payload);

    switch (jsonError.code())
    {
    case DeserializationError::Code::Ok:
        Serial.println("Deserialization of JSON was successful");
        return doc;
    case DeserializationError::Code::EmptyInput:
        Serial.println("Error: Empty input to JSON deserialization");
        return nullptr;
    case DeserializationError::Code::IncompleteInput:
        Serial.println("Error: Incomplete JSON input");
        return nullptr;
    case DeserializationError::Code::InvalidInput:
        Serial.println("Error: Invalid JSON input");
        return nullptr;
    case DeserializationError::Code::NoMemory:
        Serial.println("Error: No memory");
        return nullptr;
    case DeserializationError::Code::TooDeep:
        Serial.println("Error: JSON object too deep");
        return nullptr;
    default:
        Serial.println("Error: Unknown error");
        return nullptr;
    }
}

/*
 * Formats a URL by removing any double slashes.
 * @param url The URL to be formatted.
 * @return The formatted URL.
 */
String ApiCaller::format_url(String url)
{
    // TODO: maybe use std::string instead of String
    // Convert the String to a std::string
    std::string urlStr(url.begin(), url.end());

    // Find the position of "http://" or "https://"
    size_t httpsPos = urlStr.find("https://");
    size_t httpPos = urlStr.find("http://");
    size_t startPos;

    // Determine the starting position for replacements
    if (httpsPos != std::string::npos)
    {
        startPos = httpsPos + 8; // Length of "https://"
    }
    else if (httpPos != std::string::npos)
    {
        startPos = httpPos + 7; // Length of "http://"
    }
    else
    {
        startPos = 0; // Start from beginning if neither is found
    }

    // Loop through the string and replace "//" with "/"
    size_t pos = startPos;
    while ((pos = urlStr.find("//", pos)) != std::string::npos)
    {
        urlStr.replace(pos, 2, "/");
        pos++; // Move past the replaced "/"
    }

    return urlStr.c_str();
}