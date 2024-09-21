#ifndef ESPNOWCONFIG_H
#define ESPNOWCONFIG_H
#include<Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h> // only for esp_wifi_set_channel()
esp_now_peer_info_t slave;
#define CHANNEL 10
#define PRINTSCANRESULTS 0
#define SYS_DEBUG 0
#define TX_DEBUG 0
#define RX_DEBUG 0

#ifdef UPLOAD_HEAD
const char *SourceSSID = "MarvinHead";
const char *SourcePASS = "MarvinHead_Password";
const char *DestinationSSID = "MarvinBody";
#endif

#ifdef UPLOAD_BODY
const char *SourceSSID = "MarvinBody";
const char *SourcePASS = "MarvinBody_Password";
const char *DestinationSSID = "MarvinHead";
#endif

String TX_DATA="";
String RX_DATA="";
uint8_t data[200];
void InitESPNow() {
  WiFi.mode(WIFI_MODE_APSTA);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  WiFi.softAP(SourceSSID,SourcePASS,CHANNEL,0);
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}
#endif
