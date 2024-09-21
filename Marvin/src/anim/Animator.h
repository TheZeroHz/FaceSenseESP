#ifndef ANIMATOR_H
#define ANIMATOR_H

#define FS_NO_GLOBALS
#define USE_INTERNAL_RANDOMNESS true
#define DONT_USE_INTERNAL_RANDOMNESS false
#define RUNNING 1
#define IDLE 0

#include <TFT_eSPI.h>
#include "FS.h"
#include "SPI.h"
#include "SD_MMC.h"
#include <JPEGDecoder.h>
#include <Arduino.h>
#include <esp_random.h>


struct Information {
  String name;
  int sequence;
  bool Mode;
  int state;
};

class Animator {
private:
  int brightness;
  String dir;
  Information anim_info;
  int countFrame(const char* DIR);
  int countDir(const char* DIR);
  TFT_eSPI& tft;  // Reference to TFT_eSPI object
  void animate();
public:
  bool isEnabled;
  // Constructor with member initializer list
  Animator(TFT_eSPI& tft_object)
    : tft(tft_object) {
  }
  void setBaseDIR(String rootDIR);
  void loop();
  void cls();
  void setMode(bool mode);
  void play(String name);
  void enable();
  void disable();
  void setBrightNess(int percent);
  int getBrightNess();
  void drawSdJpeg(const char *filename, int xpos, int ypos);
  void jpegRender(int xpos, int ypos);
};

#endif
