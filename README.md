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


