#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
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
    return -1; // Indicate error
  }

  HTTPClient http;
  http.begin(String(serverBaseUrl) + "/reports");
  http.addHeader("Authorization", "Bearer " + bearerToken);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<300> doc;
  doc["description"] = message;
  doc["is_anonymous"] = false;
  doc["latitude"] = 65.0123;
  doc["longitude"] = 25.4681;
  doc["status"] = isPanic ? "emergency" : "normal"; 

  String requestBody;
  serializeJson(doc, requestBody);

  int httpCode = http.POST(requestBody);
  Serial.printf("API Response Code: %d\n", httpCode);

  int reportId = -1; // Default to error

  if (httpCode == 200 || httpCode == 201) {
        // CAPTURE THE STRING ONCE
        String payload = http.getString(); 
        
        // Use the variable for your Serial logs
        Serial.print("Raw Server Response: ");
        Serial.println(payload);

        StaticJsonDocument<1024> responseDoc;
        // PARSE THE VARIABLE, NOT THE STREAM
        DeserializationError error = deserializeJson(responseDoc, payload); 

        if (!error && responseDoc.containsKey("id")) {
            reportId = responseDoc["id"].as<int>();
            Serial.printf("Parsed Report ID: %d\n", reportId);
        } else if (error) {
            Serial.printf("JSON Error: %s\n", error.c_str());
        }
    }

    http.end();
    return reportId; // Now returning the actual database ID
}

// Upload the recorded .wav file
void uploadAudioFile(int reportId) {
    if (bearerToken == "" || reportId <= 0) return;

    File file = LittleFS.open(FILE_NAME, "r"); // Opens the recorded .wav
    if (!file) {
        Serial.println("No audio file found to upload.");
        return;
    }

    HTTPClient http;
    String uploadUrl = String(serverBaseUrl) + "/reports/" + String(reportId) + "/audio"; 
    
    http.begin(uploadUrl);
    http.addHeader("Authorization", "Bearer " + bearerToken); // Auth from loginToApi()
    http.addHeader("Content-Type", "audio/wav");

    // Stream the file from SPIFFS directly to the PHP server
    int httpCode = http.sendRequest("POST", &file, file.size());

    if (httpCode == 200) {
        Serial.println("Audio Upload Success!");
    } else {
        Serial.printf("Audio Upload Failed, code: %d\n", httpCode);
    }

    file.close();
    http.end();
}

#endif