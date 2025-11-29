#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// --- SCREEN PINS ---
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC   16
#define TFT_RST  17
#define TFT_BL    4 

// --- MIC PINS (SPH0645) ---
#define I2S_WS   15
#define I2S_SD   32
#define I2S_SCK  14
#define I2S_PORT I2S_NUM_0

// --- BUTTON PINS (2-Button Setup) ---
#define BTN_NAV   25  // The button that works (Moves arrow)
#define BTN_ENTER 26  // The action button (Send / Panic)

// --- AUDIO SETTINGS ---
#define SAMPLE_RATE 16000
#define RECORD_TIME_SECONDS 10
#define FILE_NAME "/alert.wav"

#endif