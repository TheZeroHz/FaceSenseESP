#include "Animator.h"
int Animator::getBrightNess() {
  return map(brightness, 0, 255, 0, 100);
}

void Animator::setBrightNess(int percent) {
  // Ensure the percent is within the valid range
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  brightness = map(percent, 0, 100, 0, 255);
  analogWrite(TFT_BL,brightness);  // Write the brightness value to the PWM channel
}

void Animator::enable() {
  isEnabled = true;
}
void Animator::disable() {
  isEnabled = false;
}

void Animator::setMode(bool mode) {
  anim_info.Mode = mode;
}
void Animator::play(String name) {
  anim_info.name = name;
}
void Animator::cls() {
  tft.fillScreen(TFT_BLACK);
}

// Implementation of setDIR
void Animator::setBaseDIR(String rootDIR) {
  dir = rootDIR;
}

// Implementation of countFrame
int Animator::countFrame(const char* DIR) {
  int count = 0;
  File root = SD_MMC.open(DIR);  // Open the directory specified by dir
  if (!root || !root.isDirectory()) {
    tft.println("doesn't exist or not a directory");  // If the directory doesn't exist or it's not a directory, return 0
    return 0;
  }

  File file = root.openNextFile();
  while (file) {
    count++;
    file.close();  // Close each file after counting it
    file = root.openNextFile();
  }
  root.close();
  return count;
}




// Implementation of countDir
int Animator::countDir(const char* DIR) {
  int count = 0;
  File root = SD_MMC.open(DIR);  // Open the directory specified by dir
  if (!root || !root.isDirectory()) {
    // If the directory doesn't exist or it's not a directory, return 0
    return 0;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      count++;
    }
    file.close();  // Close each directory after counting it
    file = root.openNextFile();
  }
  root.close();
  return count;
}

// Implementation of animate
void Animator::animate() {
  anim_info.state = RUNNING;
  String base_path = dir + "/" + anim_info.name;
  int catagory = countDir(base_path.c_str());
  int select = random(1, catagory + 1);
  String frame_path = base_path + "/" + (String)(select);

  int frameCount = countFrame(frame_path.c_str());
  //Serial.print("Found " + anim_info.name + " Animation Catagory:");
  //Serial.println(catagory);
  //Serial.print("Selected catagory:");
  //Serial.println(select);
  //Serial.print("Found Frame:");
  //Serial.println(frameCount);
  int start = 0, end = frameCount;
  if (anim_info.Mode){
    start = random(0, frameCount - 1);
    end = random(start, frameCount);
  }
  for (int i = start; i < end; i++) {
    String filePath = frame_path + "/frame" + i + ".jpg";
    //Serial.println("Printing " + filePath);
    drawSdJpeg(filePath.c_str(), 0, 0);
    if (i == frameCount) {
      i = 0;  // Reset to -1 so it becomes 0 on the next iteration
    }
  }
  anim_info.state = IDLE;
}

void Animator::drawSdJpeg(const char *filename, int xpos, int ypos) {
  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD_MMC.open( filename, FILE_READ);  // or, file handle reference for SD library
 
  if ( !jpegFile ) {
    //Serial.print("ERROR: File \""); //Serial.print(filename); //Serial.println ("\" not found!");
    return;
  }
  bool decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  if (decoded) {
    jpegRender(xpos, ypos);
  }
  else {
    //Serial.println("Jpeg file format not supported!");
  }
  jpegFile.close();
}



void Animator::jpegRender(int xpos, int ypos) {
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;
  bool swapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);
  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);
  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;
  max_x += xpos;
  max_y += ypos;
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }
    uint32_t mcu_pixels = win_w * win_h;
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= tft.height())
      JpegDec.abort();
  }
  tft.setSwapBytes(swapBytes);
}

void Animator::loop() {
  if (isEnabled) {
    animate();
  } else {
    //Serial.println("Animator is resting....");
    // Resting......
  }
}
