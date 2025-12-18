#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>

bool loginToApi();
int sendApiReport();
int sendApiMessage(const String message, const int reportId);
void sendApiVoiceRecording(const int reportId);

#endif