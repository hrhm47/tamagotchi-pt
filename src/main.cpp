#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPIFFS.h>

#include "PinConfig.h"
#include "AudioRecorder.h"
#include "WifiServer.h"

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
  
  // Header
  tft.fillRect(0, 0, 240, 40, ST77XX_BLUE);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("MY STATUS:");
  
  // List
  for(int i=0; i<totalMessages; i++) {
    int y = 60 + (i * 35); // Spacing
    
    if(i == currentSelection) {
      // Highlighted Item
      tft.fillRect(0, y-5, 240, 30, 0x2124); // Dark Grey Background
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

// Action 1: PANIC MODE (Double Click)
void triggerPanic() {
  Serial.println("PANIC TRIGGERED!");
  enterClicks = 0;
  waitingForSingleClick = false;
  inMenu = false;

  // 1. Visual Alarm
  tft.fillScreen(ST77XX_RED);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.setCursor(40, 80);
  tft.println("PANIC!");
  delay(500);

  // 2. Start Recording
  initMic();
  recordAudio(tft);

  // 3. Go to Wi-Fi
  startWebServer(tft);
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

  // 2. Success Screen (The "Mock" Feedback)
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

  if(!SPIFFS.begin(true)) return;

  drawMenu();
}

void loop() {
  // If not in menu (Panic mode), keep server alive
  if (!inMenu) {
    handleWebServer();
    return;
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