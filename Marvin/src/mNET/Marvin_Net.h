#ifndef MARVIN_NET_H
#define MARVIN_NET_H
#include<Arduino.h>
#include"espnowConfig.h"
#include"tx.h"
#include"rx.h"

class Marvin_Net{
public:
void init();
bool available();
String Rx();
void Tx(String DaTa);
};

void Marvin_Net::init(){
  InitESPNow();
  esp_now_register_send_cb(reportTX);
  esp_now_register_recv_cb(rx);
  ScanForSlave();
}
bool Marvin_Net::available(){
  if(RX_DATA.length())return true;
  return false;
}
String Marvin_Net::Rx(){
  String cpy=RX_DATA;
  RX_DATA="";
  return cpy;
}
void Marvin_Net::Tx(String DaTa){
  TX_DATA=DaTa;
  tx();
}

#endif
