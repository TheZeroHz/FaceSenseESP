#ifndef RX_H
#define RX_H
#include<Arduino.h>
#include"espnowConfig.h"
String Decode(const uint8_t *data,int data_len){
  String msg;
  for(int i=0;i<data_len;i++)msg=msg+(String)((char)data[i]);
  return msg;
}

#ifdef UPLOAD_HEAD
void rx(const esp_now_recv_info *recv_info, const uint8_t *data, int data_len) {
  //const uint8_t *mac_addr = recv_info->src_addr; // Get MAC address from recv_info
  RX_DATA=Decode(data,data_len);
}
#endif


#ifdef UPLOAD_BODY
void rx(const uint8_t *mac_addr, const uint8_t *data, int data_len){
  //const uint8_t *mac = mac_addr; // Get MAC address from recv_info
  RX_DATA=Decode(data,data_len);
}
#endif


#endif
