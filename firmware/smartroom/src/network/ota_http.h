#pragma once
#include <stdint.h>

// json {"ota":{"sha256","size","version"}} 

struct OtaHttpConfig {
  const char *baseUrl;  // HTTP_URL
  const char *deviceId;
  const char *username;  //bk
  const char *password;
};

void otaHttpBegin(const OtaHttpConfig &cfg);  

bool otaHttpStart(const char *sha256, uint32_t size, const char *version);

void otaHttpState(char *out, uint32_t outSize);

void otaHttpVerify(bool serverReachable, uint32_t now);

int otaHttpProgress();                    
const char *otaHttpTarget();             
bool otaHttpJustInstalled(uint32_t now);  