#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "PinConfig.h"
#include <WiFiClientSecure.h>

const char* serverBaseUrl = "ADD YOUR SERVER URL HERE";
String bearerToken = "";

bool loginToApi() {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String loginUrl = String(serverBaseUrl) + "/login"; 
    Serial.println("Logging in...");

    http.begin(client, loginUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");

    // Use DynamicJsonDocument to be safe with stack memory
    DynamicJsonDocument doc(512); 
    doc["email"] = "testitamakotsi@example.com";
    doc["password"] = "testi1234";
    doc["device_name"] = "SafeGotchi_ESP32";
    doc["api_key"] = "ADD YOUR API KEY HERE";

    String requestBody;
    serializeJson(doc, requestBody);

    int httpCode = http.POST(requestBody);

    if (httpCode == 200) {
        String respBody = http.getString();
        
        DynamicJsonDocument respDoc(1024);
        deserializeJson(respDoc, respBody);

        if (respDoc.containsKey("access_token")) {
            bearerToken = respDoc["access_token"].as<String>();
            Serial.print("access token: "); 
            Serial.println(bearerToken);
            http.end();
            return true;
        } 
    }
    
    Serial.printf("Login Failed: %d\n", httpCode);
    http.end();
    return false;
}

int sendApiReport() {
    if (bearerToken == "") {
        return -1;
    }
    
    HTTPClient http;
    http.begin(String(serverBaseUrl) + "/reports");
    http.addHeader("Authorization", "Bearer " + bearerToken);
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<300> doc;
    doc["description"] = "Test Report from Device";
    doc["is_anonymous"] = 0;
    
    String requestBody;
    serializeJson(doc, requestBody);
    
    int httpCode = http.POST(requestBody);
    Serial.printf("API Response Code: %d\n", httpCode);
    
    int reportId = -1; // Default to error
    
    if (httpCode == 200 || httpCode == 201) {
            String payload = http.getString(); 
            Serial.print("Raw Server Response: ");
            Serial.println(payload);
    
            StaticJsonDocument<1024> responseDoc;
            DeserializationError error = deserializeJson(responseDoc, payload); 
    
            if (!error && responseDoc.containsKey("id")) {
                reportId = responseDoc["id"].as<int>();
                Serial.printf("Parsed Report ID: %d\n", reportId);
            } else if (error) {
                Serial.printf("JSON Error: %s\n", error.c_str());
            }
        }
    
        http.end();
        return reportId;
}

int sendApiMessage(String message, int reportId) {
  if (bearerToken == "") {
    return -1;
  }

  HTTPClient http;
  String url = String(serverBaseUrl) + "/reports/" + String(reportId) + "/messages"; 
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + bearerToken);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<300> doc;
  doc["content"] = message;
  doc["is_anonymous"] = 0;
  doc["type"] = "text";
  doc["lat"] = 65.0123;
  doc["lon"] = 25.4681;

  String requestBody;
  serializeJson(doc, requestBody);

  Serial.printf("Sending API Message to: %s\n", url.c_str());
  int httpCode = http.POST(requestBody);
  Serial.printf("API Response Code: %d\n", httpCode);

  int messageId = -1;

  if (httpCode == 200 || httpCode == 201) {
        String payload = http.getString(); 
        StaticJsonDocument<1024> responseDoc;
        DeserializationError error = deserializeJson(responseDoc, payload); 

        if (!error && responseDoc.containsKey("id")) {
            messageId = responseDoc["id"].as<int>();
            Serial.printf("Message ID: %d\n", messageId);
        } else if (error) {
            Serial.printf("JSON Error: %s\n", error.c_str());
        }
    } else {
        String response = http.getString();
        Serial.print("SERVER ERROR: ");
        Serial.println(response);
    }

    http.end();
    return messageId;
}

// Upload the recorded .wav file
void sendApiVoiceRecording(int reportId) {
    if (bearerToken == "" || reportId <= 0) {
        Serial.println("Error: Missing Token or Report ID");
        return;
    }

    File file = LittleFS.open(FILE_NAME, "r");
    if (!file) {
        Serial.println("No audio file found in memory.");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();
    
    const char* host = "ADD YOUR SERVER DOMAIN HERE"; 
    const int port = 443;

    Serial.println("Connecting to upload audio...");
    if (!client.connect(host, port)) {
        Serial.println("Connection failed!");
        file.close();
        return;
    }

    String boundary = "------------------------Esp32Boundary7MA4YWxkTrZu0gW";
    
    // The head contains all your text fields (lat, lon, type)
    String headPayload = "";
    headPayload += "--" + boundary + "\r\n";
    headPayload += "Content-Disposition: form-data; name=\"type\"\r\n\r\naudio\r\n";
    
    headPayload += "--" + boundary + "\r\n";
    headPayload += "Content-Disposition: form-data; name=\"is_anonymous\"\r\n\r\n0\r\n"; // Matches curl '0'
    
    headPayload += "--" + boundary + "\r\n";
    headPayload += "Content-Disposition: form-data; name=\"lat\"\r\n\r\n60.1699\r\n"; // Replace with variable if needed
    
    headPayload += "--" + boundary + "\r\n";
    headPayload += "Content-Disposition: form-data; name=\"lon\"\r\n\r\n24.9384\r\n";

    // The File Header
    headPayload += "--" + boundary + "\r\n";
    headPayload += "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n";
    headPayload += "Content-Type: audio/wav\r\n\r\n";

    String tailPayload = "\r\n--" + boundary + "--\r\n";

    size_t totalLen = headPayload.length() + file.size() + tailPayload.length();
    String urlPath = "/api/v1/reports/" + String(reportId) + "/messages";

    client.println("POST " + urlPath + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("Authorization: Bearer " + bearerToken);
    client.println("Accept: application/json");
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println();

    client.print(headPayload);
    
    // Stream file 1KB at a time to save RAM
    uint8_t buf[1024];
    while (file.available()) {
        int bytesRead = file.read(buf, sizeof(buf));
        client.write(buf, bytesRead);
    }
    
    client.print(tailPayload);

    Serial.println("Upload sent. Waiting for response...");
    while (client.connected() && !client.available()) delay(10);
    
    String responseLine = client.readStringUntil('\n'); // Read the response status line
    Serial.println("Server Status: " + responseLine);
    
    while (client.available()) {
        client.read(); // Flush buffer
    }

    file.close();
    client.stop();
}

#endif