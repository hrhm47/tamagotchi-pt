#include <Arduino.h>

void setup() {
    Serial.begin(115200);     // Start serial communication
    delay(1000);              // Give time for the USB serial to connect
    Serial.println("Hello from ESP32!");
}

void loop() {
    Serial.println("Loop is running...");
    delay(1000);
}