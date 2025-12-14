#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <LittleFS.h>

#include "PinConfig.h"
#include "AudioRecorder.h"
#include "WifiServer.h"
#include "ApiClient.h"

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// --- MENU DATA ---
// You can add as many as you want. The menu loops.
const char* messages[] = {
  "I am Safe",
  "Bus is Late",
  "Pick me up",
  "SOS ALERT"
};

// "Mock" data: Who gets the alert?
const char* notifyWho[] = {
  "Parents",       // For "I am Safe"
  "School & Mom",  // For "Bus is Late"
  "Dad (Driver)",  // For "Pick me up"
  "EVERYONE!"      // For "SOS ALERT"
};

int currentSelection = 0;
int totalMessages = 4;
bool inMenu = true;

// --- BUTTON TIMING ---
unsigned long lastEnterTime = 0;
int enterClicks = 0;
bool waitingForSingleClick = false;

// --- FUNCTIONS ---

void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  
  // 1. Draw Fixed Header
  tft.fillRect(0, 0, 240, 40, ST77XX_BLUE);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("MY STATUS:");
  
  // 2. Scrolling Logic
  int itemHeight = 30; // Reduced from 35 to fit more on screen
  int listTopY = 45;   // Start just below the header
  
  // Calculate which item should be at the top of the visible list
  // This keeps the selection visible on small 135px screens
  int visibleWindow = 3; 
  int firstVisibleItem = 0;
  
  if (currentSelection >= visibleWindow) {
    firstVisibleItem = currentSelection - (visibleWindow - 1);
  }

  // 3. Draw List
  for(int i = 0; i < totalMessages; i++) {
    // Relative position based on the scroll
    int relativeIndex = i - firstVisibleItem;
    int y = listTopY + (relativeIndex * itemHeight);
    
    // Only draw if within the visible area below header
    if (relativeIndex >= 0 && relativeIndex < visibleWindow) {
      if(i == currentSelection) {
        // Highlighted Item
        tft.fillRect(0, y - 2, 240, 26, 0x2124); // Dark Grey
        tft.setTextColor(ST77XX_GREEN);
        tft.setCursor(10, y);
        tft.print("> ");
        tft.println(messages[i]);
      } else {
        // Normal Item
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(10, y);
        tft.print("  ");
        tft.println(messages[i]);
      }
    }
  }
}

// Action 1: PANIC MODE (Double Click)
void triggerPanic() {
  waitingForSingleClick = false;
  enterClicks = 0;
  Serial.println("PANIC TRIGGERED!");
  inMenu = false;

  initMic();
  recordAudio(tft);

  int reportId = sendApiMessage("SOS ALERT!", true);

  if (reportId > 0) {
    tft.fillScreen(ST77XX_ORANGE);
    tft.setCursor(10, 80);
    tft.println("UPLOADING VOICE...");
    
    uploadAudioFile(reportId); // Send the SPIFFS file to your PHP server
  }

  // 3. Cleanup and return to menu
  tft.fillScreen(ST77XX_GREEN);
  tft.println("SENT TO CLOUD");
  delay(2000);
  inMenu = true;
  drawMenu(); //
}

// Action 2: SEND MESSAGE (Single Click)
void triggerSendMessage() {
  Serial.println("Sending Message...");
  enterClicks = 0;
  waitingForSingleClick = false;
  
  // 1. sending animation
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(60, 100);
  tft.println("Sending...");
  delay(1000);

  String selectedMessage = messages[currentSelection];
  int reportId = sendApiMessage(selectedMessage, false);
  delay(1000);

  if (reportId != -1) {
    Serial.println("Message sent successfully.");
    tft.fillScreen(ST77XX_GREEN);
    tft.setTextColor(ST77XX_BLACK);
  
    tft.setCursor(10, 40);
    tft.setTextSize(3);
    tft.println("SENT!");
  
    tft.setTextSize(2);
    tft.setCursor(10, 90);
    tft.println("Notified:");
  
    // Show who we notified based on the array
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(10, 120);
    tft.println(notifyWho[currentSelection]);
  } else {
    Serial.println("Failed to send message.");
    tft.fillScreen(ST77XX_RED);
    tft.setTextColor(ST77XX_BLACK);
  
    tft.setCursor(10, 40);
    tft.setTextSize(3);
    tft.println("Sending message FAILED!");
  }
  
  delay(3000); // Show for 3 seconds
  
  // 3. Return to Menu
  drawMenu();
}

void setup() {
  Serial.begin(115200);
  
  // Hardware Init
  pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
  tft.init(135, 240); tft.setRotation(1); tft.fillScreen(ST77XX_BLACK);
  
  // Button Init (2 Buttons)
  pinMode(BTN_NAV, INPUT_PULLUP);
  pinMode(BTN_ENTER, INPUT_PULLUP);

  if(!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  if (connectToWiFi(tft)) {
    // Try to login to the API so bearerToken is available for later uploads
    bool loggedIn = false;
    int loginAttempts = 0;
    while (!loggedIn && loginAttempts < 3) {
      if (loginToApi()) {
        loggedIn = true;
        Serial.println("API login succeeded");
      } else {
        loginAttempts++;
        Serial.printf("API login attempt %d failed\n", loginAttempts);
        delay(1500);
      }
    }

    if (!loggedIn) {
      // Let the user know we'll retry later (non-blocking retry can be added in loop)
      tft.fillScreen(ST77XX_YELLOW);
      tft.setTextColor(ST77XX_RED);
      tft.setTextSize(2);
      tft.setCursor(10, 60); tft.println("API LOGIN FAILED");
      tft.setTextSize(1);
      tft.setCursor(10, 100); tft.println("Will retry later...");
      delay(1500);
    }

    drawMenu();
  }

  //drawMenu();
}

void loop() {
  // If not in menu (Panic mode), keep server alive
  if (!inMenu) {
    handleWebServer(); // Keep handling client requests

    // --- NEW: Check for EXIT condition (BTN_NAV press) ---
    if (digitalRead(BTN_NAV) == LOW) {
      // 1. Clean Up
      stopWebServer();
      
      // 2. Reset State
      inMenu = true;
      
      // 3. Redraw Menu
      drawMenu();
      
      // Debounce
      while(digitalRead(BTN_NAV) == LOW); 
      delay(50);
      return; // Exit loop iteration to prevent running menu logic
    }
    return; // Stay in server mode
  }

  // --- 1. HANDLE NAVIGATION (Single Button Cycling) ---
  if (digitalRead(BTN_NAV) == LOW) {
    currentSelection++;
    // Logic: If we go past the last item, loop back to 0
    if (currentSelection >= totalMessages) {
      currentSelection = 0;
    }
    drawMenu();
    
    // Simple Debounce
    while(digitalRead(BTN_NAV) == LOW); // Wait for release
    delay(50); 
  }

  // --- 2. HANDLE ENTER BUTTON (Logic for Double vs Single) ---
  if (digitalRead(BTN_ENTER) == LOW) {
    unsigned long now = millis();
    
    // Check if this is a Double Click (Pressed again within 500ms)
    if (now - lastEnterTime < 500 && enterClicks == 1) {
       triggerPanic(); // DOUBLE CLICK DETECTED
    } else {
       enterClicks = 1;
       lastEnterTime = now;
       waitingForSingleClick = true;
    }
    
    while(digitalRead(BTN_ENTER) == LOW); // Wait for release
    delay(50); 
  }

  // --- 3. CHECK FOR SINGLE CLICK TIMEOUT ---
  if (waitingForSingleClick && (millis() - lastEnterTime > 500)) {
     triggerSendMessage();
  }
}


// #include <Arduino.h>

// // Pins
// #define BTN_NAV   25
// #define BTN_ENTER 26

// void setup() {
//   Serial.begin(115200);
  
//   // Use Internal Pullup (High by default, Low when pressed)
//   pinMode(BTN_NAV, INPUT_PULLUP);
//   pinMode(BTN_ENTER, INPUT_PULLUP);
  
//   Serial.println("--- BUTTON HARDWARE TEST ---");
// }

// void loop() {
//   // Read States
//   int navState = digitalRead(BTN_NAV);     // 1 = Open, 0 = Pressed
//   int enterState = digitalRead(BTN_ENTER); // 1 = Open, 0 = Pressed

//   // Print Status
//   Serial.print("NAV (Pin 25): ");
//   if(navState == 0) Serial.print("PRESSED  |  ");
//   else              Serial.print("OPEN     |  ");

//   Serial.print("ENTER (Pin 26): ");
//   if(enterState == 0) Serial.println("PRESSED");
//   else                Serial.println("OPEN");

//   delay(200); // Slow down so you can read it
// }