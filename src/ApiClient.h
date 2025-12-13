#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "PinConfig.h"

// REPLACE with your computer's actual IP address
const char* serverBaseUrl = "http://192.168.100.49:8000/api/v1"; 
String bearerToken = "";

// Get the token from the login endpoint
bool loginToApi() {
    HTTPClient http;
    String loginUrl = String(serverBaseUrl) + "/login";
    Serial.print("Attempting API login to: "); Serial.println(loginUrl);
    Serial.print("WiFi status: "); Serial.println(WiFi.status());
    Serial.print("Local IP: "); Serial.println(WiFi.localIP().toString());
    http.begin(loginUrl);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> doc;
    doc["email"] = "kerttu.k@esimerkki.fi"; // Using student credentials from your API docs
    doc["password"] = "salasana";
    doc["device_name"] = "SafeGotchi_ESP32";
    doc["api_key"] = "secret123";

    String requestBody;
    serializeJson(doc, requestBody);
    Serial.print("Login request body: ");
    Serial.println(requestBody);

    int httpCode = http.POST(requestBody);
    if (httpCode == 200) {
        // Get raw response body
        String respBody = http.getString();
        Serial.print("Login response body (raw): ");
        Serial.println(respBody);

        // 1) Try parsing JSON response { "token": "..." }
        StaticJsonDocument<1024> respDoc;
        DeserializationError err = deserializeJson(respDoc, respBody);
        if (!err && respDoc.containsKey("token")) {
            bearerToken = respDoc["token"].as<String>();
        } else {
            // 2) Fallback: take last non-empty line from the response (handles warnings + plain token)
            int lastNewline = respBody.lastIndexOf('\n');
            String tokenCandidate = respBody;
            if (lastNewline >= 0) tokenCandidate = respBody.substring(lastNewline + 1);
            tokenCandidate.trim();
            bearerToken = tokenCandidate;
        }

        bearerToken.trim();
        Serial.print("API Login Success! Bearer Token: ");
        Serial.println(bearerToken);
        Serial.println("API Login Success!");
        http.end();
        return true;
    }
    if (httpCode > 0) {
        // Server returned an HTTP error status; print body for diagnostics
        String resp = http.getString();
        Serial.printf("Login Failed: %d\n", httpCode);
        if (resp.length()) {
            Serial.print("Response body: "); Serial.println(resp);
        }
    } else {
        // Negative codes are network/connection errors from HTTPClient
        Serial.printf("Login Failed: %d (network/connection error)\n", httpCode);
    }
    http.end();
    return false;
}

// Send the text message
int sendApiMessage(String message, bool isPanic) {
  if (bearerToken == "") {
    return 401;
  }   

  HTTPClient http;
  http.begin(String(serverBaseUrl) + "/reports");
  http.addHeader("Authorization", "Bearer " + bearerToken);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<300> doc;
  doc["description"] = message;
  doc["is_anonymous"] = false;
  
  // Adding the "Dummy" data requested by your team member
  doc["latitude"] = 65.0123;   // Dummy Oulu coordinates
  doc["longitude"] = 25.4681;
  doc["status"] = isPanic ? "emergency" : "normal"; 

  String requestBody;
  serializeJson(doc, requestBody);

  int httpCode = http.POST(requestBody);
  Serial.printf("API Response: %d\n", httpCode);
  http.end();

  return httpCode;
}

// Upload the recorded .wav file
void uploadAudioFile() {
    if (bearerToken == "") return;

    File file = SPIFFS.open(FILE_NAME, "r");
    if (!file) {
        Serial.println("No audio file found to upload.");
        return;
    }

    HTTPClient http;
    // Note: You will need a specific endpoint in your Laravel/App for file uploads
    http.begin(String(serverBaseUrl) + "/reports/audio"); 
    http.addHeader("Authorization", "Bearer " + bearerToken);
    http.addHeader("Content-Type", "audio/wav");

    // This streams the file directly from SPIFFS to the API
    int httpCode = http.sendRequest("POST", &file, file.size());

    if (httpCode > 0) {
        Serial.printf("Audio Uploaded. Status: %d\n", httpCode);
    }
    file.close();
    http.end();
}

#endif