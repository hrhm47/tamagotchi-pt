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

#endif