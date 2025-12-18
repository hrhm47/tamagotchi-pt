#ifndef AUDIO_RECORDER_H
#define AUDIO_RECORDER_H

#include <Arduino.h>
#include <Adafruit_ST7789.h>

void initMic();
void recordAudio(Adafruit_ST7789 &tft);

#endif