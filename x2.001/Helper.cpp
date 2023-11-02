/* 
 *  Helper.h - Deklaration
 *  Helper.cpp - Definition
 *  ***
 *  Diverse Hilfsmittel
 *  ***
 *  Michel 02.11.2023
 */

#include <Arduino.h>
#include "GlobalVars.h"

// MAC als String ausgeben
String getMacAsString() {
  uint8_t baseMac[6];
  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  char baseMacChr[18] = {0};
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);

  return String(baseMacChr);
}
