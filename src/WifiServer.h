#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "PinConfig.h"
#include "Credentials.h" // New dependency
#include <Adafruit_ST7789.h>

WebServer server(80);

// --- New Function: Connect to Wi-Fi ---
bool connectToWiFi(Adafruit_ST7789 &tft) {
  tft.fillScreen(ST77XX_BLUE);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 20); tft.println("CONNECTING...");

  WiFi.mode(WIFI_STA); // Set to Station mode
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  tft.setTextSize(1);
  tft.setCursor(10, 60); tft.println("SSID:");
  tft.setTextSize(2);
  tft.setCursor(10, 80); tft.println(WIFI_SSID);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_CONNECTION_ATTEMPTS) {
    delay(500);
    // Draw an animating dot for connection status
    tft.fillRect(100 + (attempts % 4) * 10, 120, 8, 8, ST77XX_WHITE);
    if (attempts > 0 && (attempts-1) % 4 == 3) {
      tft.fillRect(100, 120, 40, 8, ST77XX_BLUE); // Clear dots
    }
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    tft.fillScreen(ST77XX_RED);
    tft.setTextSize(2);
    tft.setCursor(10, 50); tft.println("WIFI FAILED!");
    tft.setTextSize(1);
    tft.setCursor(10, 80); tft.println("Check Credentials");
    delay(5000); // Show failure for 5 seconds
    return false;
  }

  return true;
}

void startWebServer(Adafruit_ST7789 &tft) {

  // 2. UI Update (Show connected IP)
  IPAddress IP = WiFi.localIP();
  tft.fillScreen(ST77XX_BLUE);
  tft.setTextColor(ST77XX_WHITE); 
  
  tft.setTextSize(2);
  tft.setCursor(10, 20); tft.println("CONNECTED!");
  
  tft.setTextSize(1);
  tft.setCursor(10, 60); tft.println("1. Connect to network:");
  tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(2);
  tft.println(" SafeGotchi"); // Still showing the SSID for clarity, even though it's the home/school Wi-Fi.
  
  tft.setTextColor(ST77XX_WHITE); tft.setTextSize(1);
  tft.setCursor(10, 110); tft.println("2. Open Browser:");
  tft.setTextColor(ST77XX_GREEN); tft.setTextSize(2);
  tft.print(" "); tft.println(IP); // Show the local IP on the network

  // 3. Web Server Handlers (No Change)
  server.on("/", HTTP_GET, []() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { background-color: #121212; color: #ffffff; font-family: Helvetica, sans-serif; text-align: center; padding: 20px; }";
    html += "h1 { color: #03dac6; }";
    html += ".card { background-color: #1e1e1e; padding: 30px; border-radius: 15px; box-shadow: 0 4px 8px 0 rgba(0,0,0,0.2); }";
    html += ".btn { background-color: #bb86fc; color: black; padding: 15px 32px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; border-radius: 8px; margin-top: 20px; border: none;}";
    html += "</style></head><body>";
    
    html += "<div class='card'>";
    html += "<h1>Safe-Gotchi</h1>";
    html += "<p>New Audio Alert Recorded</p>";
    html += "<a href='/download' class='btn'>&#9658; PLAY AUDIO</a>";
    html += "</div></body></html>";
    
    server.send(200, "text/html", html);
  });

  server.on("/download", HTTP_GET, []() {
    File file = SPIFFS.open(FILE_NAME, "r");
    if (!file) { server.send(404, "text/plain", "No Audio Found"); return; }
    server.streamFile(file, "audio/wav");
    file.close();
  });

  server.begin();
}

void stopWebServer() {
  server.stop();
}

void handleWebServer() {
  server.handleClient();
}

#endif