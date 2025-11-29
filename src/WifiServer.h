#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "PinConfig.h"
#include <Adafruit_ST7789.h>

WebServer server(80);

void startWebServer(Adafruit_ST7789 &tft) {
  // 1. Create Hotspot
  WiFi.softAP("SafeGotchi-Audio");
  IPAddress IP = WiFi.softAPIP();

  // 2. UI Update
  tft.fillScreen(ST77XX_BLUE);
  tft.setTextColor(ST77XX_WHITE); 
  
  tft.setTextSize(2);
  tft.setCursor(10, 20); tft.println("DONE!");
  
  tft.setTextSize(1);
  tft.setCursor(10, 60); tft.println("1. Connect Wi-Fi to:");
  tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(2);
  tft.println(" SafeGotchi");
  
  tft.setTextColor(ST77XX_WHITE); tft.setTextSize(1);
  tft.setCursor(10, 110); tft.println("2. Open Browser:");
  tft.setTextColor(ST77XX_GREEN); tft.setTextSize(2);
  tft.print(" "); tft.println(IP);

  // 3. Better Web Page Design
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
    // FIXED: Correct MIME type for WAV audio
    server.streamFile(file, "audio/wav");
    file.close();
  });

  server.begin();
}

void handleWebServer() {
  server.handleClient();
}

#endif