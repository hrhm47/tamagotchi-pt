#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <LittleFS.h>

#include "PinConfig.h"
#include "AudioRecorder.h"
#include "WifiServer.h"
#include "ApiClient.h"

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Messages that are displayed in the menu.
const char* messages[] = {
  "Keinuilla",      // At the swings
  "Aulassa",        // In the lobby
  "Ruokalassa",     // In the cafeteria
  "Kaytavalla",     // In the hallway
  "Pukuhuoneessa",  // In the locker room
  "Vessassa",       // In the toilet
};

int currentSelection = 0;
int totalMessages = 6;
bool inMenu = true;

unsigned long lastEnterTime = 0;
int enterClicks = 0;
bool waitingForSingleClick = false;

void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  
  tft.fillRect(0, 0, 240, 40, ST77XX_BLUE);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("ILMOITA KIUSAAMINEN:");
  
  int itemHeight = 30;
  int listTopY = 45;
  int visibleOnWindow = 3; 
  int firstVisibleItem = 0;
  
  if (currentSelection >= visibleOnWindow) {
    firstVisibleItem = currentSelection - (visibleOnWindow - 1);
  }

  for(int i = 0; i < totalMessages; i++) {
    int relativeIndex = i - firstVisibleItem;
    int y = listTopY + (relativeIndex * itemHeight);

    if (relativeIndex >= 0 && relativeIndex < visibleOnWindow) {
      if(i == currentSelection) {
        tft.fillRect(0, y - 2, 240, 26, 0x2124); // Dark Grey
        tft.setTextColor(ST77XX_GREEN);
        tft.setCursor(10, y);
        tft.print("> ");
        tft.println(messages[i]);
      } else {
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(10, y);
        tft.print("  ");
        tft.println(messages[i]);
      }
    }
  }
}

void triggerPanic() {
  waitingForSingleClick = false;
  enterClicks = 0;
  Serial.println("PANIC TRIGGERED!");
  inMenu = false;

  initMic();
  recordAudio(tft);
  int reportId = sendApiReport();

  if (reportId > 0) {
    tft.fillScreen(ST77XX_ORANGE);
    tft.setCursor(10, 80);
    tft.println("Lahetetaan nauhoitusta...");
    sendApiVoiceRecording(reportId);
  }

  tft.fillScreen(ST77XX_GREEN);
  tft.println("LAHETETTY!");
  delay(2000);
  inMenu = true;
  drawMenu();
}

void triggerSendMessage() {
  Serial.println("Sending Message...");
  enterClicks = 0;
  waitingForSingleClick = false;
  
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(60, 100);
  tft.println("Lahetetaan...");
  delay(1000);

  int reportId = sendApiReport();
  int msgId = -1;

  if (reportId != -1) {
    Serial.printf("Report created with ID: %d\n", reportId);

    Serial.println("Sending Message Content...");
    String selectedMessage = messages[currentSelection];
    msgId = sendApiMessage(selectedMessage, reportId);
  } else {
    Serial.println("Failed to create report.");
  }

  delay(1000);

  if (msgId != -1) {
    Serial.println("Message sent successfully.");
    tft.fillScreen(ST77XX_GREEN);
    tft.setTextColor(ST77XX_BLACK);
  
    tft.setCursor(10, 40);
    tft.setTextSize(3);
    tft.println("LAHETETTY!");
  
    tft.setTextSize(2);
    tft.setCursor(10, 90);
  
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(10, 120);
  } else {
    Serial.println("Failed to send message.");
    tft.fillScreen(ST77XX_RED);
    tft.setTextColor(ST77XX_BLACK);
  
    tft.setCursor(10, 40);
    tft.setTextSize(3);
    tft.println("VIESTIN LAHETYS EPAONNISTUI!");
  }
  
  delay(3000);
  drawMenu();
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
  tft.init(135, 240); tft.setRotation(1); tft.fillScreen(ST77XX_BLACK);
  
  pinMode(BTN_NAV, INPUT_PULLUP);
  pinMode(BTN_ENTER, INPUT_PULLUP);

  if(!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  if (connectToWiFi(tft)) {
    bool loggedIn = false;
    int loginAttempts = 0;
    while (!loggedIn && loginAttempts < 3) {
      if (loginToApi()) {
        loggedIn = true;
      } else {
        loginAttempts++;
        Serial.printf("API login attempt %d failed\n", loginAttempts);
        delay(1500);
      }
    }

    if (!loggedIn) {
      tft.fillScreen(ST77XX_YELLOW);
      tft.setTextColor(ST77XX_RED);
      tft.setTextSize(2);
      tft.setCursor(10, 60); tft.println("Kirjautuminen epaonnistui!");
      tft.setTextSize(1);
      tft.setCursor(10, 100); tft.println("Tarkista nettiyhteys");
      delay(1500);
    }

    drawMenu();
  }
}

void loop() {
  if (!inMenu) {
    if (digitalRead(BTN_NAV) == LOW) {;
      inMenu = true;
      drawMenu();
      
      // Debounce
      while(digitalRead(BTN_NAV) == LOW); 
      delay(50);
      return; // Exit loop iteration to prevent running menu logic
    }
    return;
  }

  // Handle looping through menu items
  if (digitalRead(BTN_NAV) == LOW) {
    currentSelection++;
    if (currentSelection >= totalMessages) {
      currentSelection = 0;
    }
    drawMenu();
    
    while(digitalRead(BTN_NAV) == LOW); // Wait for release
    delay(50); 
  }

  // handle enter button (Single and double press)
  if (digitalRead(BTN_ENTER) == LOW) {
    unsigned long now = millis();
    
    // Check if this is a Double Click
    if (now - lastEnterTime < 500 && enterClicks == 1) {
       triggerPanic();
    } else {
       enterClicks = 1;
       lastEnterTime = now;
       waitingForSingleClick = true;
    }
    
    while(digitalRead(BTN_ENTER) == LOW); // Wait for release
    delay(50); 
  }

  // Check for Single Click timeout
  if (waitingForSingleClick && (millis() - lastEnterTime > 500)) {
     triggerSendMessage();
  }
}