Here is the complete **Developer Documentation** for the "Safe-Gotchi" project. You can hand this directly to your friend/team member. It covers the current state of the prototype, the hardware map, the code architecture, and the roadmap for the next phase.

---

# 🛡️ Project Safe-Gotchi: Developer Documentation v1.0

## 1. Project Overview
**Safe-Gotchi** is a wearable IoT safety device for children disguised as a virtual pet.
* **Core Function:** It allows a child to silently record audio ("Black Box Mode") or send preset status messages to their parents in an emergency.
* **Current Status:** The device has a working UI Menu, a "Panic" trigger (Double-click), high-quality Audio Recording (16-bit WAV), and an Offline Wi-Fi Server for playback.

---

## 2. Hardware Configuration (Pinout Map)
**Controller:** ESP32-WROOM-32D (DevKit V1)

### A. Display (Adafruit 1.14" ST7789)
*Connected via Hardware VSPI.*
| Pin Label | ESP32 GPIO | Function |
| :--- | :--- | :--- |
| **MOSI/SDA** | **23** | SPI Data Input |
| **SCK/SCL** | **18** | SPI Clock |
| **CS** | **5** | Chip Select |
| **DC** | **16** | Data/Command Control |
| **RST** | **17** | Reset |
| **BL** | **4** | Backlight Control |
| **VCC** | **3.3V** | Power (Shared) |
| **GND** | **GND** | Ground (Shared) |

### B. Microphone (SPH0645 I2S MEMS)
*Connected via I2S Interface.*
| Pin Label | ESP32 GPIO | Function |
| :--- | :--- | :--- |
| **BCLK** | **14** | Bit Clock |
| **LRcl** | **15** | Word Select (Left/Right) |
| **DOUT** | **32** | Data Output |
| **SEL** | **3.3V** | Channel Select (Right Channel) |
| **3V** | **3.3V** | Power (Shared) |
| **GND** | **GND** | Ground (Shared) |

### C. Input Buttons
*Wired using "Trench Method" (Across breadboard gap) to prevent shorting.*
| Button | ESP32 GPIO | Logic Type | Function |
| :--- | :--- | :--- | :--- |
| **NAV** | **25** | `INPUT_PULLUP` | Cycle through menu items. |
| **ENTER**| **26** | `INPUT_PULLUP` | Single Click: Select / Double Click: Panic. |

---

## 3. Software Architecture (File by File)

The project is modular. Do not put everything in `main.cpp`.

### 📂 `src/PinConfig.h`
**Role:** The Central Settings File.
* **Purpose:** Stores all pin numbers and global constants. If you need to move a wire, change the number here, and it updates everywhere.
* **Key Settings:**
    * `SAMPLE_RATE`: Set to `16000` (16kHz). Good balance between voice clarity and file size.
    * `RECORD_TIME_SECONDS`: Set to `10`. Determines how long the panic mode listens.

### 📂 `src/AudioRecorder.h`
**Role:** The "Ears" (Audio Processing Engine).
* **Function `initMic()`:**
    * Configures I2S.
    * **Crucial Setting:** `.use_apll = true`. This enables the Audio PLL (High precision clock) to remove static noise and "robot" glitches.
    * **Buffers:** Uses 8 buffers of 512 bytes to ensure smooth streaming without data loss.
* **Function `recordAudio(&tft)`:**
    * **Compression Logic:** Reads **32-bit** raw data from the Mic but bit-shifts it (`>> 14`) to save it as **16-bit** PCM. This makes writing to memory 2x faster, preventing audio gaps.
    * **UI Integration:** Draws the "Recording..." screen and the "Dot Animation" while recording.
    * **Storage:** Saves file as `/alert.wav` in SPIFFS.

### 📂 `src/WifiServer.h`
**Role:** The "Voice" (Connectivity & Playback).
* **Function `startWebServer(&tft)`:**
    * Creates a Wi-Fi Access Point (Hotspot) named **"SafeGotchi-Audio"**.
    * IP Address: `192.168.4.1`.
    * Hosts a mini-website stored in the ESP32 code.
* **Routes:**
    * `/`: Serves the HTML "Dark Mode" Player Card.
    * `/download`: Streams the `/alert.wav` file from SPIFFS to the user's phone browser.

### 📂 `src/main.cpp`
**Role:** The "Brain" (Logic & State Machine).
* **`setup()`:** Initializes hardware. Checks if `SPIFFS` is formatted.
* **`loop()`:** Handles the Menu System.
* **Logic Flow:**
    1.  **Menu Mode:** Checks for button presses.
    2.  **Navigation (Pin 25):** Increments `currentSelection` and redraws screen.
    3.  **Action (Pin 26):**
        * **Single Click:** Calls `triggerSendMessage()` (Mock logic showing "Notified: Parents").
        * **Double Click (Window < 500ms):** Calls `triggerPanic()`.
    4.  **Panic Mode:** Breaks out of the loop, starts `recordAudio()`, then launches `startWebServer()`.

---

## 4. PlatformIO Configuration (`platformio.ini`)
**Critical for Memory Management.**
You must include `board_build.partitions = min_spiffs.csv`.
* **Why?** The default ESP32 map has very little storage. This setting reallocates memory to give us ~1.5MB of space for audio files. Without this, `SPIFFS.begin()` will fail.

---
## ~~ 5. Next Steps: Development Roadmap ~~

~~ Here is the plan for the next phase of development. ~~

### ~~Phase A: Cloud Connectivity (Priority: High) ~~
* ~~ **Goal:** Instead of hosting a local Offline Server, the device should upload the audio to the Internet. ~~
* ~~**Task:** ~~
  ~~  1.  Modify `WifiServer.h` to connect to a **Router** (SSID/Password) instead of making a Hotspot.~~
  ~~  2.  Implement `HTTPClient` with `multipart/form-data`. ~~
  ~~  3.  Upload `/alert.wav` + `JSON Data` (Device ID, Message Type) to a PHP/Node.js server.~~

### ~~Phase B: The Mobile App Integration~~
*~~ **Goal:** The parent receives a notification when "Panic" is pressed.~~
* ~~**Task:**~~
   ~~ 1.  Create a PHP API endpoint (e.g., `upload_audio.php`).~~
   ~~ 2.  When ESP32 uploads a file, the PHP script triggers **Firebase Cloud Messaging (FCM)**.~~
   ~~ 3.  Flutter App receives push notification: "🚨 SOS Alert from Safe-Gotchi!".~~

###~~ Phase C: Smart Wi-Fi Manager~~
* ~~**Goal:** Allow the user to set their Wi-Fi password without hardcoding it.~~
* ~~**Task:** Re-integrate the `WiFiManager` library.~~
   ~~ * *Logic:* On boot, try to connect. If fail -> Create Hotspot -> User enters Password -> Save to Flash -> Reboot & Connect.~~

---

## ~~6. Debugging Cheat Sheet~~

~~| Symptom | Cause | Solution |~~
~~| :--- | :--- | :--- |~~
~~| **"SPIFFS Error" on Boot** | Partition table missing. | Add `min_spiffs.csv` to `platformio.ini` and run "Erase Flash". |~~
~~| **Menu scrolls automatically** | Floating Pin. | Check Pin 25 wire. Ensure "Trench Method" wiring for buttons. |~~
~~| **"Panic" triggers immediately** | Pin 26 Shorted. | Button rotated 90 degrees wrong. Rotate button. |~~
~~| **Audio is static/robot** | Buffer Underrun. | Ensure `use_apll=true` in config and `bits_per_sample=16` in Recorder. |~~
~~| **Screen is Black** | Backlight loose. | Check Pin 4 connection. |~~

