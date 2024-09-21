/**********************************************
********** ESP32 V-3.02 MUST BE INSTALLED *****
***********************************************/
#include "esp_camera.h"
#include "esp_core_dump.h"
#define CAMERA_MODEL_ESP32S3_EYE  // Has PSRAM
#include "camera_pins.h"
#define UPLOAD_HEAD  //As This Code IS for Head of Marvin
#include <Marvin.h>


String USER_NAME = "", NER_USER_NAME = "";
unsigned long fr_max_time = 10000;
unsigned long fr_timer = 0, enrl_timer = 0;
bool pauseCamTask = false;
bool fr_res = false;
int8_t recognition_enabled = 0;
int8_t is_enrolling = 0;
int8_t detection_enabled = 1;


void CameraSetUP();


void setup() {
  Serial.begin(115200);
  animatorInit();
  espnowInit();
  setAnimBaseDir("/Animations");
  animMode(DONT_USE_INTERNAL_RANDOMNESS);
  animEnable();
  animShow("calm");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_240X240;
  config.pixel_format = PIXFORMAT_RGB565;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (config.pixel_format == PIXFORMAT_RGB565) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      log_i("Please Enable PSRAM!!!");
      return;
    }
  } else {
    log_i("Please Select Correct Image Format");
  }



  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  log_i("Camera Configs Initiated");
  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
  s->set_framesize(s, FRAMESIZE_240X240);
  CameraSetUP();
}

void loop() {
  if (isCmdAvailable()) {

    String grab = grabCmd();
    if (grab.indexOf("anim:") != -1) {
      detection_enabled=1;
      grab = grab.substring(5, grab.length());
      animEnable();
      animShow(grab);
    } else if (grab.indexOf("disp:") != -1) {
      setBrightness(100);
      grab = grab.substring(5, grab.length());
      grab.trim();
      Serial.print(grab);
      Serial.println(grab.compareTo("100%"));
      sendCmd(grab);
      if (grab.compareTo("cls") == 0) cls();
      else if (grab.compareTo("dis") == 0) {
        cls();
        animDisable();
      } else if (grab.compareTo("ena") == 0) {
        cls();
        animEnable();
      } else if (grab.compareTo("0%") == 0) {
        setBrightness(0);
      } else if (grab.compareTo("25%") == 0) {
        setBrightness(25);
      } else if (grab.compareTo("50%") == 0) {
        setBrightness(50);
      } else if (grab.compareTo("75%") == 0) {
        setBrightness(75);
      } else if (grab.compareTo("100%") == 0) {
        setBrightness(100);
      }
    } else if (grab.indexOf("face:") != -1) {
      grab = grab.substring(5, grab.length());
      if (grab.compareTo("shw0") == 0) {
        animDisable();
        detection_enabled = 0;
        recognition_enabled = 0;
        is_enrolling = 0;
      } else if (grab.compareTo("shw1") == 0) {
        detection_enabled = 0;
        recognition_enabled = 0;
        is_enrolling = 0;
      } else if (grab.compareTo("cam1") == 0) {
        animDisable();
        pauseCamTask = true;
        detection_enabled = 0;
        recognition_enabled = 0;
        is_enrolling = 0;
      } else if (grab.compareTo("cam0") == 0) {
        pauseCamTask = false;
        detection_enabled = 1;
        recognition_enabled = 0;
        is_enrolling = 0;
        animEnable();
        animShow("calm");
      } else if (grab.compareTo("who") == 0) {
        USER_NAME="";
        animDisable();
        fr_timer = millis();
        detection_enabled = 1;
        recognition_enabled = 1;
        is_enrolling = 0;

      } else if (grab.indexOf("enrl:") != -1) {
        detection_enabled = 1;
        recognition_enabled = 1;
        is_enrolling = 1;
        NER_USER_NAME = grab.substring(5, grab.length());
        Serial.println("Please Enroll=>" + NER_USER_NAME);
      }
    } else sendCmd("error:404");
  }

  if(recognition_enabled&&millis()-fr_timer>=fr_max_time){
    Serial.println("FR TIMEOUT!");
    recognition_enabled=0;
    fr_res=true;
}

  if (fr_res) {
    detection_enabled = 1;
    recognition_enabled = 0;
    is_enrolling = 0;
    fr_res = false;
    sendCmd("face:who:" + USER_NAME);
    animEnable();
    animShow("calm");
  }
  if (Serial.available()) sendCmd(Serial.readString());
}
