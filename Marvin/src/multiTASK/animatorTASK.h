#ifndef ANIMATORTASK_H
#define ANIMATORTASK_H
#include<Arduino.h>
#include "../anim/Animator.h"

#define FS_NO_GLOBALS
//// fixed for freenove s3
#define SD_MMC_CMD 38 //Please do not modify it.
#define SD_MMC_CLK 39 //Please do not modify it. 
#define SD_MMC_D0  40 //Please do not modify it.

#define anim_core 1
#define Animator_stack 5000

TaskHandle_t animation_handle = NULL;
TFT_eSPI tft2 = TFT_eSPI();  // Invoke custom library
Animator animator(tft2);

//****************************************************************************************
//                                    Animator_T A S K                                   *
//****************************************************************************************


struct animMessage {
  uint8_t cmd;
  String txt;
  bool val;
  uint32_t value;
  uint32_t ret;
} animTxMessage, animRxMessage;

enum : uint8_t {
  SET_DIR,
  SET_MODE,
  SET_BRIGHTNESS,
  GET_BRIGHTNESS,
  RUN,
  DISABLE,
  ENABLE,
  CLS
};

QueueHandle_t animSetQueue = NULL;
QueueHandle_t animGetQueue = NULL;

void animCreateQueues() {
  animSetQueue = xQueueCreate(10, sizeof(struct animMessage));
  animGetQueue = xQueueCreate(10, sizeof(struct animMessage));
}

void animTask(void* parameter) {
  animCreateQueues();
  if (!animSetQueue || !animGetQueue) {
    log_e("queues are not initialized");
    while (true) { ; }  // endless loop
  }

  struct animMessage animRxTaskMessage;
  struct animMessage animTxTaskMessage;
  tft2.begin();
  tft2.setRotation(2);  // 0 & 2 Portrait. 1 & 3 landscape
  tft2.fillScreen(TFT_ORANGE);
      SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
    if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
      log_i("Card Mount Failed");
      return;
    }
    uint8_t cardType = SD_MMC.cardType();
    if(cardType == CARD_NONE){
        log_i("No SD_MMC card attached");
        return;
    }

    log_i("SD_MMC Card Type: ");
    if(cardType == CARD_MMC){
        log_i("MMC");
    } else if(cardType == CARD_SD){
        log_i("SDSC");
    } else if(cardType == CARD_SDHC){
        log_i("SDHC");
    } else {
        log_i("UNKNOWN");
    }
    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    log_i("SD_MMC Card Size: %lluMB\n", cardSize);
    while (true) {
    if (xQueueReceive(animSetQueue, &animRxTaskMessage, 1) == pdPASS) {
      if (animRxTaskMessage.cmd == SET_DIR) {
        animTxTaskMessage.cmd = SET_DIR;
        animator.setBaseDIR(animRxTaskMessage.txt);
        xQueueSend(animGetQueue, &animTxTaskMessage, portMAX_DELAY);
      } else if (animRxTaskMessage.cmd == SET_MODE) {
        animTxTaskMessage.cmd = SET_MODE;
        animator.setBrightNess(animRxTaskMessage.val);
        xQueueSend(animGetQueue, &animTxTaskMessage, portMAX_DELAY);
      } else if (animRxTaskMessage.cmd == SET_BRIGHTNESS) {
        animTxTaskMessage.cmd = SET_BRIGHTNESS;
        animator.setBrightNess(animRxTaskMessage.value);
        xQueueSend(animGetQueue, &animTxTaskMessage, portMAX_DELAY);
      } else if (animRxTaskMessage.cmd == GET_BRIGHTNESS) {
        animTxTaskMessage.cmd = GET_BRIGHTNESS;
        animTxTaskMessage.ret = animator.getBrightNess();
        xQueueSend(animGetQueue, &animTxTaskMessage, portMAX_DELAY);
      } else if (animRxTaskMessage.cmd == RUN) {
        animTxTaskMessage.cmd = RUN;
        animator.play(animRxTaskMessage.txt);
        xQueueSend(animGetQueue, &animTxTaskMessage, portMAX_DELAY);
      } else if (animRxTaskMessage.cmd == ENABLE) {
        animTxTaskMessage.cmd = ENABLE;
        animator.cls();
        animator.enable();
        xQueueSend(animGetQueue, &animTxTaskMessage, portMAX_DELAY);
      } else if (animRxTaskMessage.cmd == DISABLE) {
        animTxTaskMessage.cmd = DISABLE;
        animator.cls();
        animator.disable();
        xQueueSend(animGetQueue, &animTxTaskMessage, portMAX_DELAY);
      } else if (animRxTaskMessage.cmd == CLS) {
        animTxTaskMessage.cmd = CLS;
        animator.cls();
        xQueueSend(animGetQueue, &animTxTaskMessage, portMAX_DELAY);
      }

      else {
        log_i("Invalid Task Command for Animator");
      }
    }
    animator.loop();
    if (!animator.isEnabled) {
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  }
}





void animatorInit() {
  xTaskCreatePinnedToCore(
    animTask,              /* Function to implement the task */
    "Animator",            /* Name of the task */
    Animator_stack,        /* Stack size in words */
    NULL,                  /* Task input parameter */
    2 | portPRIVILEGE_BIT, /* Priority of the task */
    &animation_handle,     /* Task handle. */
    anim_core               /* Core where the task should run */
  );
}




struct animMessage transmitReceive(animMessage msg) {
  xQueueSend(animSetQueue, &msg, portMAX_DELAY);
  if (xQueueReceive(animGetQueue, &animRxMessage, portMAX_DELAY) == pdPASS) {
    if (msg.cmd != animRxMessage.cmd) {
      log_e("wrong reply from message queue");
    }
  }
  return animRxMessage;
}



void setAnimBaseDir(String DIR){
  animTxMessage.cmd = SET_DIR;
  animTxMessage.txt =DIR;
  animMessage RX = transmitReceive(animTxMessage);
}
void animMode(bool mode){
  animTxMessage.cmd =SET_MODE;
  animTxMessage.val=mode;
  animMessage RX = transmitReceive(animTxMessage);
}

void setBrightness(uint32_t percentage) {
  animTxMessage.cmd = SET_BRIGHTNESS;
  animTxMessage.value= percentage;
  animMessage RX = transmitReceive(animTxMessage);
}
uint32_t getBrightness(){
  animTxMessage.cmd = GET_BRIGHTNESS;
  animMessage RX = transmitReceive(animTxMessage);
  return RX.ret;
}

void animShow(String Name) {
  animTxMessage.cmd = RUN;
  animTxMessage.txt = Name;
  animMessage RX = transmitReceive(animTxMessage);
}

void animEnable() {
  animTxMessage.cmd = ENABLE;
  animMessage RX = transmitReceive(animTxMessage);
}

void animDisable() {
  animTxMessage.cmd = DISABLE;
  animMessage RX = transmitReceive(animTxMessage);
}

void cls() {
  animTxMessage.cmd = CLS;
  animMessage RX = transmitReceive(animTxMessage);
}

#endif
