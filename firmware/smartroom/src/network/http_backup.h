#pragma once
#include <stdint.h>

void httpBegin(const char *baseUrl, const char *deviceId, const char *username, const char *password);
bool httpEnabled();
bool httpSend(const char *json);  
bool httpSucceeded();          
void httpPause(bool paused, uint32_t waitMs);