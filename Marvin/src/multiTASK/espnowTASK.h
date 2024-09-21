#ifndef ESPNOWTASK_H
#define ESPNOWTASK_H
#include<Arduino.h>
#include"../mNET/Marvin_Net.h"
#define espnow_core 1
#define espnow_stack 5000

TaskHandle_t espnow_handle = NULL;
Marvin_Net mnet;

//****************************************************************************************
//                                    espnow_T A S K                                     *
//****************************************************************************************

struct espnowMessage {
  uint8_t cmd;
  String txt;
  bool val;
  uint32_t value;
  uint32_t ret;
} espnowTxMessage, espnowRxMessage;

enum : uint8_t {
  SEND_DATA,
  GRAB_DATA,
  IS_AVAILABLE,
};

QueueHandle_t espnowSetQueue = NULL;
QueueHandle_t espnowGetQueue = NULL;

void espnowCreateQueues() {
  espnowSetQueue = xQueueCreate(10, sizeof(struct espnowMessage));
  espnowGetQueue = xQueueCreate(10, sizeof(struct espnowMessage));
}



void espnowTask(void* parameter) {
  espnowCreateQueues();
  if (!espnowSetQueue || !espnowGetQueue) {
    log_e("queues are not initialized");
    while (true) { ; }  // endless loop
  }
  mnet.init();
  struct espnowMessage espnowRxTaskMessage;
  struct espnowMessage espnowTxTaskMessage;
  while (true) {
    if (xQueueReceive(espnowSetQueue, &espnowRxTaskMessage, 1) == pdPASS) {
      if (espnowRxTaskMessage.cmd == IS_AVAILABLE) {
        espnowTxTaskMessage.cmd = IS_AVAILABLE;
        espnowTxTaskMessage.val= mnet.available();
        xQueueSend(espnowGetQueue, &espnowTxTaskMessage, portMAX_DELAY);
      }
	  else if (espnowRxTaskMessage.cmd == SEND_DATA) {
        espnowTxTaskMessage.cmd = SEND_DATA;
        mnet.Tx(espnowRxTaskMessage.txt);
        xQueueSend(espnowGetQueue, &espnowTxTaskMessage, portMAX_DELAY);
      }else if (espnowRxTaskMessage.cmd == GRAB_DATA) {
        espnowTxTaskMessage.cmd = GRAB_DATA;
        espnowTxTaskMessage.txt= mnet.Rx();
        xQueueSend(espnowGetQueue, &espnowTxTaskMessage, portMAX_DELAY);
      }
      else {
        log_i("Invalid Task Command for espnow");
      }
    }
      vTaskDelay(5/portTICK_PERIOD_MS);
  }
}

void espnowInit() {
  xTaskCreatePinnedToCore(
    espnowTask,              /* Function to implement the task */
    "espnow",            /* Name of the task */
    espnow_stack,        /* Stack size in words */
    NULL,                  /* Task input parameter */
    2 | portPRIVILEGE_BIT, /* Priority of the task */
    &espnow_handle,     /* Task handle. */
    espnow_core               /* Core where the task should run */
  );
}



struct espnowMessage transmitReceive(espnowMessage msg) {
  xQueueSend(espnowSetQueue, &msg, portMAX_DELAY);
  if (xQueueReceive(espnowGetQueue, &espnowRxMessage, portMAX_DELAY) == pdPASS) {
    if (msg.cmd != espnowRxMessage.cmd) {
      log_e("wrong reply from message queue");
    }
  }
  return espnowRxMessage;
}

bool isCmdAvailable(){
  espnowTxMessage.cmd = IS_AVAILABLE;
  espnowMessage RX = transmitReceive(espnowTxMessage);
  return RX.val;
}

void sendCmd(String CMD){
  espnowTxMessage.cmd = SEND_DATA;
  espnowTxMessage.txt =CMD;
  espnowMessage RX = transmitReceive(espnowTxMessage);
}

String grabCmd(){
  espnowTxMessage.cmd = GRAB_DATA;
  espnowMessage RX = transmitReceive(espnowTxMessage);
  return RX.txt;	
}

#endif
