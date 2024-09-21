#include <string>
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "esp32-hal-ledc.h"
#include "sdkconfig.h"
#include "Arduino.h"
#include <vector>
#include <TFT_eSPI.h>  // Include the TFT display library
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include "face_recognition_tool.hpp"
#include "face_recognition_112_v1_s8.hpp"
#pragma GCC diagnostic error "-Wformat"
#pragma GCC diagnostic warning "-Wstrict-aliasing"
#include "esp32-hal-log.h"
#include "FS.h"
#include "LittleFS.h"
#include "SD_MMC.h"
#include <SPI.h>
// JPEG decoder library
#include <JPEGDecoder.h>


//############ Shared Variable #################
extern String USER_NAME,NER_USER_NAME;
extern unsigned long fr_max_time,fr_timer;
extern bool pauseCamTask;
extern int8_t recognition_enabled;
extern int8_t is_enrolling;
extern int8_t detection_enabled;
extern bool fr_res;


// Return the minimum of two values a and b
#define minimum(a,b)     (((a) < (b)) ? (a) : (b))
#define SD_MMC_CMD 38  //Please do not modify it.
#define SD_MMC_CLK 39  //Please do not modify it.
#define SD_MMC_D0 40   //Please do not modify it.

FaceRecognition112V1S8 recognizer;
// TFT display object
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft); // Sprite object for off-screen drawing
#define FACE_ID_SAVE_NUMBER 30

#define FACE_COLOR_WHITE 0x00FFFFFF
#define FACE_COLOR_BLACK 0x00000000
#define FACE_COLOR_RED 0x000000FF
#define FACE_COLOR_GREEN 0x0000FF00
#define FACE_COLOR_BLUE 0x00FF0000
#define FACE_COLOR_YELLOW (FACE_COLOR_RED | FACE_COLOR_GREEN)
#define FACE_COLOR_CYAN (FACE_COLOR_BLUE | FACE_COLOR_GREEN)
#define FACE_COLOR_PURPLE (FACE_COLOR_BLUE | FACE_COLOR_RED)

struct User {
  int id;
  String name;
  String role;
};

User person;

User searchUserById(int id) {
  fs::File file = SD_MMC.open("/users.txt", "r");
  User user;
  user.id = -1;  // Initialize id to -1 indicating user not found

  if (!file) {
    tft.println("Failed to open users.txt for reading.");
    return user;
  }

  String line;
  while (file.available()) {
    line = file.readStringUntil('\n');
    int userId = line.substring(0, line.indexOf('|')).toInt();
    if (userId == id) {
      // Found the user
      String data = line.substring(line.indexOf('|') + 1);
      int separatorIndex = data.indexOf('|');
      user.id = id;
      user.name = data.substring(0, separatorIndex);
      user.role = data.substring(separatorIndex + 1);
      break;
    }
  }

  file.close();

  if (user.id == -1) {
    tft.println("User not found.");
  }

  return user;
}




bool addUser(User newUser) {
  fs::File file = SD_MMC.open("/users.txt", "a");

  if (!file) {
    tft.println("Failed to open users.txt for appending.");
    fs::File file = SD_MMC.open("/users.txt", "w");
    return false;
  }

  // Format: id|name|role
  String line = String(newUser.id) + "|" + newUser.name + "|" + newUser.role + "\n";
  file.print(line);

  file.close();

  tft.println("User added successfully.");
  return true;
}




void draw_frame_on_tft(fb_data_t *fb) {
  tft.startWrite();
  tft.setAddrWindow(0, 0, fb->width, fb->height);
  tft.pushImage(0, 0, fb->width, fb->height, (uint16_t *)fb->data);
  tft.endWrite();
  vTaskDelay(50);
}


void drawBGR888Image(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *data) {
    tft.startWrite();
    tft.setAddrWindow(0, 0, w,h);
  uint16_t buffer[w];  // Buffer to hold a row of pixels
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      uint32_t index = (j * w + i) * 3;
      uint8_t blue = data[index];
      uint8_t green = data[index + 1];
      uint8_t red = data[index + 2];
      buffer[i] = (red & 0xF8) << 8 | (green & 0xFC) << 3 | (blue >> 3);
    }
    tft.pushColors(buffer, w, true);  // Send the row of pixels
  }
tft.endWrite();
}

// Function to draw a rectangle using a sprite
void pushFaceBox(TFT_eSprite* sprite, int x, int y, int w, int h, uint16_t color) {
    sprite->drawFastHLine(x, y, w*0.5, color);        // Top horizontal line
    sprite->drawFastHLine(x, y + (h*0.5) - 1, w*0.5, color);  // Bottom horizontal line
    sprite->drawFastVLine(x, y, h*0.5, color);        // Left vertical line
    sprite->drawFastVLine(x + (w*0.5) - 1, y, h*0.5, color);  // Right vertical line   
}



static void draw_face_boxes(fb_data_t *fb, std::list<dl::detect::result_t> *results, int face_id) {
  int x, y, w, h;
  uint32_t color = FACE_COLOR_YELLOW;
  if (face_id < 0) {
    color = FACE_COLOR_RED;
  } else if (face_id > 0) {
    color = FACE_COLOR_GREEN;
  }
  if (fb->bytes_per_pixel == 2) {
    //color = ((color >> 8) & 0xF800) | ((color >> 3) & 0x07E0) | (color & 0x001F);
    color = ((color >> 16) & 0x001F) | ((color >> 3) & 0x07E0) | ((color << 8) & 0xF800);
  }
  int i = 0;
  for (std::list<dl::detect::result_t>::iterator prediction = results->begin(); prediction != results->end(); prediction++, i++) {
    // rectangle box
    x = (int)prediction->box[0];
    y = (int)prediction->box[1];
    w = (int)prediction->box[2] - x + 1;
    h = (int)prediction->box[3] - y + 1;
    if ((x + w) > fb->width) {
      w = fb->width - x;
    }
    if ((y + h) > fb->height) {
      h = fb->height - y;
    }
    Serial.printf("\nFace Box:\n");
    Serial.printf("(x0,y0)=(%d,%d)\n",x,y);
    Serial.printf("(x1,y1)=(%d,%d)\n",x,y+h);
    Serial.printf("(x2,y2)=(%d,%d)\n",x,y);
    Serial.printf("(x3,y3)=(%d,%d)\n",x+w,y);
    // Draw the rectangle as a series of fast lines
    pushFaceBox(&spr,x,y,w,h,color);
    fb_gfx_drawFastHLine(fb, x, y, w, color);
    fb_gfx_drawFastHLine(fb, x, y + h - 1, w, color);
    fb_gfx_drawFastVLine(fb, x, y, h, color);
    fb_gfx_drawFastVLine(fb, x + w - 1, y, h, color);
    // landmarks (left eye, mouth left, nose, right eye, mouth right)
    int x0, y0, j;
    //Serial.printf("Face LandMarks:\n");
    for (j = 0; j < 10; j += 2) {
      x0 = (int)prediction->keypoint[j];
      y0 = (int)prediction->keypoint[j + 1];
      //Serial.printf("(x,y)(%d,%d)\n",x0,y0);
      fb_gfx_fillRect(fb, x0, y0, 3, 3, color);
    }
  }
}

static int run_face_recognition(fb_data_t *fb, std::list<dl::detect::result_t> *results) {
  std::vector<int> landmarks = results->front().keypoint;
  int id = -1;
  Tensor<uint8_t> tensor;
  tensor.set_element((uint8_t *)fb->data).set_shape({ fb->height, fb->width, 3 }).set_auto_free(false);

  int enrolled_count = recognizer.get_enrolled_id_num();

  if (enrolled_count < FACE_ID_SAVE_NUMBER && is_enrolling) {
    User newPerson;
    id = recognizer.enroll_id(tensor, landmarks, " ", true);
    newPerson.id = id;
    newPerson.name = NER_USER_NAME;
    newPerson.role = "admin";
    tft.println("trying to add " + newPerson.name);
    //delay(10000);
    if (id != -1 && addUser(newPerson)) {
      tft.println("Successfull");
    }
    tft.setTextColor(TFT_GREEN);
    tft.println("Face Enrolled :-)");
    is_enrolling = 0;
    //ESP.restart();
  }

  face_info_t recognize = recognizer.recognize(tensor, landmarks);
  if (recognize.id >= 0) {
    User knownPerson = searchUserById(recognize.id);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(0, 10);
    tft.printf("%s : %.2f", knownPerson.name, recognize.similarity);
    USER_NAME=knownPerson.name;
    fr_res=true;
    tft.setRotation(2);         // Set the rotation of the TFT display if needed
    tft.fillScreen(TFT_BLACK);  // Clear the display
    } 
  else {
    tft.setTextColor(TFT_RED);
    tft.setCursor(0, 10);
    tft.printf("! _ ! Sorry");
  }
  return recognize.id;
}

void runCAM(void *pvParameters){
  person.name = "";
  person.id = -1;
  person.role = "";
  camera_fb_t *fb = NULL;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  bool detected = false;
  int face_id = 0;
  size_t out_len = 0, out_width = 0, out_height = 0;
  uint8_t *out_buf = NULL;
  bool s = false;
  HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
  HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);

  while (true) {
    if(!pauseCamTask){
    retry:
    face_id = -1;
    fb = esp_camera_fb_get();
    if (!fb) {
      tft.println("Camera capture failed");

      goto retry;
    } else {
      // Display the captured frame on TFT
        fb_data_t rfb;
        if (detection_enabled||recognition_enabled||is_enrolling) {
        out_len = fb->width * fb->height * 3;
        out_width = fb->width;
        out_height = fb->height;
        out_buf = (uint8_t *)malloc(out_len);
        if (!out_buf) {
          Serial.println("out_buf malloc failed in bgr888");
        }
        else{
        s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
        rfb.width = out_width;
        rfb.height = out_height;
        rfb.data = out_buf;
        rfb.bytes_per_pixel = 3;
        rfb.format = FB_BGR888;
        if (detection_enabled) {
          std::list<dl::detect::result_t> &candidates = s1.infer((uint8_t *)out_buf, { (int)out_height, (int)out_width, 3 });
          std::list<dl::detect::result_t> &results = s2.infer((uint8_t *)out_buf, { (int)out_height, (int)out_width, 3 }, candidates);
          if (results.size() > 0) {
            if (recognition_enabled) {
              face_id = run_face_recognition(&rfb, &results);
            }
            draw_face_boxes(&rfb, &results, face_id);
          }
        }
        }
          
        }
        else{
        rfb.width = fb->width;
        rfb.height = fb->height;
        rfb.data = fb->buf;
        rfb.bytes_per_pixel = 2;
        rfb.format = FB_RGB565;
        draw_frame_on_tft(&rfb);
        }
      esp_camera_fb_return(fb);
      fb = NULL;
      if(out_buf!=NULL){
        free(out_buf);out_buf=NULL;
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 milliseconds
      }
      if((detection_enabled==1&&recognition_enabled==1&&is_enrolling==1)||(detection_enabled==1&&recognition_enabled==1&&is_enrolling==1)){
      spr.pushSprite(0, 0);
      spr.deleteSprite();
      spr.createSprite(160,128);  // Create a sprite of 120x120 pixels
      }

    }
    }
    else {vTaskDelay(pdMS_TO_TICKS(500));Serial.println("Cam Is Paused");}
  }
}



void CameraSetUP() {
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
    tft.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    tft.println("No SD_MMC card attached");
    return;
  }

  Serial.print("SD_MMC Card Type: ");
  if (cardType == CARD_MMC) {
    tft.println("MMC");
  } else if (cardType == CARD_SD) {
    tft.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    tft.println("SDHC");
  } else {
    tft.println("UNKNOWN");
  }

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  //tft.setRotation(2);         // Set the rotation of the TFT display if needed
  tft.fillScreen(TFT_BLACK);  // Clear the display
  spr.setColorDepth(16);  // Set color depth (16-bit)
  spr.createSprite(160,128);  // Create a sprite of 120x120 pixels
  recognizer.set_partition(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "fr");
  recognizer.set_ids_from_flash();  // load ids from flash partition
    // Create the task pinned to core 0
    xTaskCreatePinnedToCore(
        runCAM,     // Task function
        "CamTask",      // Name of the task
        16384,                   // Stack size (increase if needed)
        NULL,                   // Task input parameter
        1,                      // Priority (0-24)
        NULL,                   // Task handle
        0                       // Core (0 or 1)
    );


}
