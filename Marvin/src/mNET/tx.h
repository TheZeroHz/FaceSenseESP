#ifndef TX_H
#define TX_H
#include<Arduino.h>
#include"espnowConfig.h"
// Scan for slaves in AP mode
void ScanForSlave() {
  int16_t scanResults = WiFi.scanNetworks(false, false, false, 500, CHANNEL); // Scan only on one channel
  bool slaveFound = 0;
  memset(&slave, 0, sizeof(slave));
  if(SYS_DEBUG)Serial.println("");
  if (scanResults == 0) {
    if(SYS_DEBUG)Serial.println("No WiFi devices in AP Mode found");
  } else {
    if(SYS_DEBUG){Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");}
    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
      if (PRINTSCANRESULTS) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(SSID);
        Serial.print(" (");
        Serial.print(RSSI);
        Serial.print(")");
        Serial.println("");
      }
      delay(10);
      if (SSID.indexOf(DestinationSSID) == 0) {
        if(SYS_DEBUG){
        Serial.println("Found a Slave.");
        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");}
        // Get BSSID => Mac Address of the Slave
        int mac[6];
        if ( 6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          for (int ii = 0; ii < 6; ++ii ) {
            slave.peer_addr[ii] = (uint8_t) mac[ii];
          }
        }
        slave.channel = CHANNEL; // pick a channel
        slave.encrypt = 0; // no encryption
        slaveFound = 1;
        break;
      }
    }
  }

  if (slaveFound) {
    if(SYS_DEBUG)Serial.println("Slave Found, processing..");
  } else {
    if(SYS_DEBUG)Serial.println("Slave Not Found, trying again.");
  }
  WiFi.scanDelete();
}


// Check if the slave is already paired with the master.
// If not, pair the slave with master
bool manageSlave() {
  if (slave.channel == CHANNEL) {
    bool exists = esp_now_is_peer_exist(slave.peer_addr);
    if ( exists) {if(SYS_DEBUG)Serial.println("Already Paired");return true;} 
    else {
      esp_err_t addStatus = esp_now_add_peer(&slave);
      if (addStatus == ESP_OK) {if(SYS_DEBUG)Serial.println("Pair success");return true;} 
      else if (addStatus == ESP_ERR_ESPNOW_EXIST) {if(SYS_DEBUG)Serial.println("Peer Exists");return true;} 
      else {if(SYS_DEBUG)Serial.println("Not sure what happened");return false;}
    }} 
  else {if(SYS_DEBUG)Serial.println("No Slave found to process");return false;}
}


void Encode(){
  for(int i=0;i<TX_DATA.length();i++)data[i]=TX_DATA[i];
}
void send(){
  Encode();
  const uint8_t *peer_addr = slave.peer_addr;
  esp_err_t result = esp_now_send(peer_addr,data, TX_DATA.length());
  if(TX_DEBUG){
  if (result == ESP_OK) Serial.println("Success");
  else {Serial.println("Failed!");}
  }
}
// send data
void tx() {
  if (slave.channel == CHANNEL) {
    bool isPaired = manageSlave();
    if (isPaired) {
      send();
    } else {
      if(TX_DEBUG)Serial.println("Slave pair failed!");
      ScanForSlave();
      send();
    }
  }
  else {
      ScanForSlave();
      send();
  }
  TX_DATA="";
}

void reportTX(const uint8_t *mac_addr, esp_now_send_status_t status) {
if(TX_DEBUG)Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
#endif
