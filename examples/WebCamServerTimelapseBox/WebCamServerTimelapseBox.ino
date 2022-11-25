

/*
  AutoConnect combined with ESP32 Camera Sensor.

  Before running this sketch, you need to upload the webcamview.html file
  contained in the data folder to the ESP32 module using ESP32 upload tool
  with SPIFFS.

  Copyright (c) 2021 Hieromon Ikasamo.
  This software is released under the MIT License.
  https://opensource.org/licenses/MIT

  This example is an interface to the ESP32 Camera Driver, and contains an HTTP
  server that can convert frame data captured by the ESP32 Camera Driver into
  JPEG format and respond as HTTP content.
  The supported camera sensor devices and general limitations follow the ESP32
  Camera Driver specification.

  It is the HTTP Server instance of EDP-IDF that sends out the JPEG image
  frames captured by the camera sensor as HTTP content. The HTTP server, which
  is invoked by ESP32Cam class, can coexist with the ESP32 Arduino core
  WebServer library, and can be used in sketches with a web interface, such as
  AutoConnect, by isolating the ports of each http server.

  Related information is as follows:
    ESP32 Camera Driver Repository on the GitHub:
      https://github.com/espressif/esp32-camera
    ESP-IDF HTTP Server documentation:
      https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html

  Note:
    1. AutoConnect v1.3.2 or later is required to experience this sketch.
    2. It is necessary to upload the webcamview.html file that the data folder
       contains to the SPIFFS file system of the ESP module. To upload the
       webcamview.html file, use ESP32 Sketch Data Upload tool menu from the
       Arduino IDE, or the upload facility via esptool.py built into the build
       systems such as PlatformIO.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <AutoConnect.h>
#include "ESP32WebCam.h"
#include "src/ESPFtpServer/ESPFtpServer.h"
#include "SD_MMC.h"
#include <FastLED.h>
#include <EEPROM.h>
//#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "settings.h"

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3003000)
#warning "Requires FastLED 3.3 or later; check github for latest code."
#endif

String esp_hostname = ESP_HOSTNAME; // min. 3 characters
String esp_password = ESP_PASSWORD;

// Initialize Telegram BOT
//String BOTtoken = TELEGRAM_BOT_TOKEN;  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
//String CHAT_ID = TELEGRAM_CHAT_ID;

//bool sendPhoto = false;
//WiFiClientSecure clientTCP;
//UniversalTelegramBot bot(BOTtoken, clientTCP);

//Checks for new messages every 1 second.
//unsigned long botRequestDelay = TELEGRAM_BOT_POLL_INTERVAL;
//unsigned long lastTimeBotRan;


FtpServer ftpSrvSD(FTP_CTRL_PORT_SD, FTP_DATA_PORT_PASV_SD);   //set #define FTP_DEBUG in ESP32FtpServer.h to see ftp verbose on serial

bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}


void listDir(char * dir){
 
  File root = SD_MMC.open(dir);
 
  File file = root.openNextFile();
  File save_file;
  
  while(file){
      std::string filename = file.name();
      Serial.print("FILE: ");
      Serial.println(filename.c_str());

      if (hasEnding(filename, ".jpg"))
        save_file = file;
        
      file = root.openNextFile();
  }

  String latest_picture =  save_file.name();
  Serial.print("latest picture: ");
  Serial.println(latest_picture);
  
  file.close();
  root.close();
}

// Handle what happens when you receive new messages
//void handleNewMessages(int numNewMessages) {
//  Serial.println("handleNewMessages");
//  Serial.println(String(numNewMessages));
//
//  for (int i=0; i<numNewMessages; i++) {
//    // Chat id of the requester
//    String chat_id = String(bot.messages[i].chat_id);
//    if (chat_id != CHAT_ID){
//      bot.sendMessage(chat_id, "Unauthorized user", "");
//      continue;
//    }
//    
//    // Print the received message
//    String text = bot.messages[i].text;
//    Serial.println(text);
//
//    String from_name = bot.messages[i].from_name;
//
//    if (text == "/start") {
//      String welcome = "Welcome, " + from_name + ".\n";
//      welcome += "Use the following commands to control your outputs.\n\n";
//      welcome += "/led_on to turn GPIO ON \n";
//      welcome += "/led_off to turn GPIO OFF \n";
//      welcome += "/state to request current GPIO state \n";
//      bot.sendMessage(chat_id, welcome, "");
//    }
//
//    if (text == "/led_on") {
//      bot.sendMessage(chat_id, "LED state set to ON", "");
//      //ledState = HIGH;
//      //digitalWrite(ledPin, ledState);
//    }
//    
//    if (text == "/led_off") {
//      bot.sendMessage(chat_id, "LED state set to OFF", "");
//      //ledState = LOW;
//      //digitalWrite(ledPin, ledState);
//    }
//    
//    if (text == "/state") {
//      if (/*digitalRead(ledPin)*/true){
//        bot.sendMessage(chat_id, "LED is ON", "");
//      }
//      else{
//        bot.sendMessage(chat_id, "LED is OFF", "");
//      }
//    }
//  }
//}


// Image sensor settings page
const char  CAMERA_SETUP_PAGE[] = R"*(
{
  "title": "Camera",
  "uri": "/_setting",
  "menu": true,
  "element": [
    {
      "name": "css",
      "type": "ACStyle",
      "value": ".noorder label{display:inline-block;min-width:150px;padding:5px;} .noorder select{width:160px} .magnify{width:20px}"
    },
    {
      "name": "res",
      "type": "ACSelect",
      "label": "Resolution",
      "option": [
        "UXGA(1600x1200)",
        "SXGA(1280x1024)",
        "XGA(1024x768)",
        "SVGA(800x600)",
        "VGA(640x480)",
        "CIF(400x296)",
        "QVGA(320x240)",
        "HQVGA(240x176)",
        "QQVGA(160x120)"
      ],
      "selected": 4
    },
    {
      "name": "qua",
      "type": "ACRange",
      "label": "Quality",
      "value": 10,
      "min": 10,
      "max": 63,
      "magnify": "infront"
    },
    {
      "name": "con",
      "type": "ACRange",
      "label": "Contrast",
      "value": 0,
      "min": -2,
      "max": 2,
      "magnify": "infront"
    },
    {
      "name": "bri",
      "type": "ACRange",
      "label": "Brightness",
      "value": 0,
      "min": -2,
      "max": 2,
      "magnify": "infront"
    },
    {
      "name": "sat",
      "type": "ACRange",
      "label": "Saturation",
      "value": 0,
      "min": -2,
      "max": 2,
      "magnify": "infront"
    },
    {
      "name": "se",
      "type": "ACSelect",
      "label": "Special Effect",
      "option": [
        "No Effect",
        "Negative",
        "Grayscale",
        "Red Tint",
        "Green Tint",
        "Blue Tint",
        "Sepia"
      ],
      "selected": 1
    },
    {
      "name": "awb",
      "type": "ACCheckbox",
      "label": "AWB",
      "labelposition": "infront",
      "checked": true
    },
    {
      "name": "wbg",
      "type": "ACCheckbox",
      "label": "AWB Gain",
      "labelposition": "infront",
      "checked": true
    },
    {
      "name": "wbm",
      "type": "ACSelect",
      "label": "WB Mode",
      "option": [
        "Auto",
        "Sunny",
        "Cloudy",
        "Office",
        "Home"
      ],
      "selected": 1
    },
    {
      "name": "aec",
      "type": "ACCheckbox",
      "label": "AEC SENSOR",
      "labelposition": "infront",
      "checked": true
    },
    {
      "name": "dsp",
      "type": "ACCheckbox",
      "label": "AEC DSP",
      "labelposition": "infront",
      "checked": true
    },
    {
      "name": "ael",
      "type": "ACRange",
      "label": "AE Level",
      "value": 0,
      "min": -2,
      "max": 2,
      "magnify": "infront"
    },
    {
      "name": "exp",
      "type": "ACRange",
      "label": "Exposure",
      "value": 204,
      "min": 0,
      "max": 1200,
      "magnify": "infront",
      "style": "margin-left:20px;width:110px"
    },
    {
      "name": "agc",
      "type": "ACCheckbox",
      "label": "AGC",
      "labelposition": "infront",
      "checked": true
    },
    {
      "name": "agv",
      "type": "ACRange",
      "label": "AGC Gain (Nx)",
      "value": 5,
      "min": 1,
      "max": 31,
      "magnify": "infront"
    },
    {
      "name": "acl",
      "type": "ACRange",
      "label": "Gain Ceiling (2^)",
      "value": 0,
      "min": 1,
      "max": 7,
      "magnify": "infront"
    },
    {
      "name": "bpc",
      "type": "ACCheckbox",
      "label": "DPC Black",
      "labelposition": "infront",
      "checked": true
    },
    {
      "name": "wpc",
      "type": "ACCheckbox",
      "label": "DPC White",
      "labelposition": "infront",
      "checked": true
    },
    {
      "name": "gma",
      "type": "ACCheckbox",
      "label": "GMA enable",
      "labelposition": "infront",
      "checked": true
    },
    {
      "name": "lec",
      "type": "ACCheckbox",
      "label": "Lense Correction",
      "labelposition": "infront",
      "checked": true
    },
    {
      "name": "hmi",
      "type": "ACCheckbox",
      "label": "H-Mirror",
      "labelposition": "infront"
    },
    {
      "name": "vfl",
      "type": "ACCheckbox",
      "label": "V-Flip",
      "labelposition": "infront"
    },
    {
      "name": "dcw",
      "type": "ACCheckbox",
      "label": "DCW (Downsize EN)",
      "labelposition": "infront"
    },
    {
      "name": "set",
      "type": "ACSubmit",
      "value": "SET",
      "uri": "/set"
    }
  ]
}
)*";

// Transition destination for CAMERA_SETUP_PAGE
// It will invoke the handler as setSensor function for setting the image sensor.
// The `response` is ana attribute added in AutoConnect v1.3.2 to suppress the
// HTTP response from AutoConnect. AutoConnectAux handlers with the
// `"response":false` can return their own HTTP response.
const char  CAMERA_SETUP_EXEC[] = R"*(
{
  "title": "Camera",
  "uri": "/set",
  "response": false,
  "menu": false
}
)*";

/**
 * Specify one of the following supported sensor modules to be actually applied.
 * However, not all of the devices have been fully tested. Activating the timer-
 * shot may cause a WDT RESET during streaming.
 */
const ESP32Cam::CameraId  model = ESP32Cam::CameraId::CAMERA_MODEL_AI_THINKER;
// const ESP32Cam::CameraId  model = ESP32Cam::CameraId::CAMERA_MODEL_ESP_EYE;
// const ESP32Cam::CameraId  model = ESP32Cam::CameraId::CAMERA_MODEL_M5STACK_ESP32CAM;
// const ESP32Cam::CameraId  model = ESP32Cam::CameraId::CAMERA_MODEL_M5STACK_NO_PSRAM;
// const ESP32Cam::CameraId  model = ESP32Cam::CameraId::CAMERA_MODEL_M5STACK_PSRAM;
// const ESP32Cam::CameraId  model = ESP32Cam::CameraId::CAMERA_MODEL_M5STACK_UNITCAM;
// const ESP32Cam::CameraId  model = ESP32Cam::CameraId::CAMERA_MODEL_M5STACK_V2_PSRAM;
// const ESP32Cam::CameraId  model = ESP32Cam::CameraId::CAMERA_MODEL_M5STACK_WIDE;
// const ESP32Cam::CameraId  model = ESP32Cam::CameraId::CAMERA_MODEL_TTGO_T_JOURNAL;
// const ESP32Cam::CameraId  model = ESP32Cam::CameraId::CAMERA_MODEL_WROVER_KIT;

// AutoConnectAUx entry point
const char*  const _sensorUrl = "/_setting";
const char*  const _setUrl = "/set";

// Endpoint that leads request to the root page to webcamview.html
const char*  const _viewerUrl = "/webcam"; 

// Viewer-UI content
const char*  const _webcamserver_html = "/webcamview.html";

// Specifying the time zone and assigning NTP.
// Required to add the correct local time to the export file name of the
// captured image. This assignment needs to be localized.
// This sketch works even if you omit the NTP server specification. In that
// case, the suffix timestamp of the captured image file is the elapsed time
// since the ESP module was powered on.
//const char*  const _tz = "JST-9";
//const char*  const _ntp1 = "ntp.nict.jp";
//const char*  const _ntp2 = "ntp.jst.mfeed.ad.jp";
const char*  const _tz = "UTC-1";
const char*  const _ntp1 = "0.de.pool.ntp.org";
const char*  const _ntp2 = "1.de.pool.ntp.org";

// You can change the URL assigned to interface with ESP32WebCam according to
// your needs without the sketch code modification.
// Allows changes to each endpoint by specifying the following build_option in
// the platformio.ini file of the PlatformIO build system
//
// - platformio.ini
//   build_flags =
//     -DESP32CAM_DEFAULT_PATH_CAPTURE='"path you wish to CAPTURE's endpoint"'
//     -DESP32CAM_DEFAULT_PATH_PROMPT='"path you wish to PROMPT's endpoint"'
//     -DESP32CAM_DEFAULT_PATH_STREAM='"path you wish to STREAM's endpoint"'

// Choose the file system properly to fit the SD card interface of the ESP32
// module you are using.
// In typical cases, SD is used for the VSPI interface, and MMC is used for the
// HS2 interface.
//const char*  const _fs = "sd";
const char*  const _fs = "mmc";

// ESP32WebCam instance; Hosts a different web server than the one hosted by
// AutoConect.
// If you want to assign a different port than the default, uncomment the
// following two lines and enable _cameraServerPort value. _cameraServerPort is
// the port on which the http server started by ESP32WebCam will listen.
//const uint16_t _cameraServerPort = 3000; // Default is 3000.
//ESP32WebCam webcam(_cameraServerPort);
ESP32WebCam webcam;

// Declare AutoConnect as usual.
// Routing in the root page and webcamview.html natively uses the request
// handlers of the ESP32 WebServer class, so it explicitly instantiates the
// ESP32 WebServer.
WebServer   portalServer;
AutoConnect portal(portalServer);
AutoConnectConfig config;

// Assemble the query string for webcamview.html according the configuration
// by the sketch.
// The webcamview.html accepts the following query parameters:
// - host=HOSTNAME                            (default: 0.0.0.0)
// - port=PORT_NUMBER                         (default: 3000)
// - stream=STREAMING_PATH                    (default: _stream)
// - capture=CAPTURING_PATH                   (default: _capture)
// - prompt=PROMPTING_PATH_FOR_REMOTE_CONTROL (default: _prompt)
// - setting=AUTOCONNECTAUX_SETTING_PATH      (default: undefined)
// - fs={sd|mmc}                              (default: mmc)
// - ac=AUTOCONNECT_ROOT_PATH                 (default: _ac) 
// - title=VIEWER_PAGE_HEADER_TITLE           (default: undefined)
String viewerUrl(void) {
  String  url = String(_viewerUrl)
              + "?host=" + WiFi.localIP().toString()
              + "&port=" + String(webcam.getServerPort())
              + "&capture=" + String(webcam.getCapturePath())
              + "&prompt=" + String(webcam.getPromptPath())
              + "&stream=" + String(webcam.getStreamPath())
              + "&fs=" + String(_fs)
              + "&ac=" AUTOCONNECT_URI
              + "&setting=" + String(_sensorUrl)
              + "&title=" + String(WiFi.getHostname());
  return url;
}

// Directs the request to the root to webcamview.html.
// This function exists only to assemble the query string.
void handleRootPage(void) {
  portalServer.sendHeader("Location", viewerUrl());
  portalServer.send(302, "text/plain", "");
  portalServer.client().stop();
}

// Read webcamview.html from SPIFFS and send it to the client as response.
// The query string given by handleRootPage is taken over tby the 302-redirect.
void handleViewPage(void) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi is disconnected.");
    Serial.println("Viewer will not be available until connection is restored.");
    portalServer.send(500, "text/plain", "WiFi is not connected. Viewer will not be available until connection is restored.");
    return;
  }

  File viewPage = SD_MMC.open(_webcamserver_html, FILE_READ);
  if (viewPage) {
    portalServer.streamFile(viewPage, "text/html");
    close(viewPage);
  }
  else {
    Serial.printf("Viewer %s could not load, check your data folder and upload it.\n", _webcamserver_html);
    portalServer.send(500, "text/plain", String(_webcamserver_html) + " open failed");
  }
}

// AutoConnectAux handler for CAMERA_SETUP_EXEC
// Read the camera status via the ESP32 Camera Driver and update it with the
// sensor parametes defined in each AutoConnect element in CAMERA_SETUP?PAGE.
String setSensor(AutoConnectAux& aux, PageArgument& args) {
  AutoConnectAux& cameraSetup = *portal.aux(_sensorUrl);
  camera_status_t status;

  // Take over the current settings
  webcam.sensor().getStatus(&status);

  // Framesize
  const String& resolution = cameraSetup["res"].as<AutoConnectSelect>().value();
  if (resolution == "UXGA(1600x1200)")
    status.framesize = FRAMESIZE_UXGA;
  else if (resolution == "SXGA(1280x1024)")
    status.framesize = FRAMESIZE_SXGA;
  else if (resolution == "XGA(1024x768)")
    status.framesize = FRAMESIZE_XGA;
  else if (resolution == "SVGA(800x600)")
    status.framesize = FRAMESIZE_SVGA;
  else if (resolution == "VGA(640x480)")
    status.framesize = FRAMESIZE_VGA;
  else if (resolution == "CIF(400x296)")
    status.framesize = FRAMESIZE_CIF;
  else if (resolution == "QVGA(320x240)")
    status.framesize = FRAMESIZE_QVGA;
  else if (resolution == "HQVGA(240x176)")
    status.framesize = FRAMESIZE_HQVGA;
  else if (resolution == "QQVGA(160x120)")
    status.framesize = FRAMESIZE_QQVGA;

  // Pixel granularity
  status.quality = cameraSetup["qua"].as<AutoConnectRange>().value;

  // Color solid adjustment
  status.contrast = cameraSetup["con"].as<AutoConnectRange>().value;
  status.brightness = cameraSetup["bri"].as<AutoConnectRange>().value;
  status.saturation = cameraSetup["sat"].as<AutoConnectRange>().value;

  // SE
  const String& se = cameraSetup["se"].as<AutoConnectSelect>().value();
  if (se == "No Effect")
    status.special_effect = 0;
  if (se == "Negative")
    status.special_effect = 1;
  if (se == "Grayscale")
    status.special_effect = 2;
  if (se == "Red Tint")
    status.special_effect = 3;
  if (se == "Green Tint")
    status.special_effect = 4;
  if (se == "Blue Tint")
    status.special_effect = 5;
  if (se == "Sepia")
    status.special_effect = 6;

  // White Balance effection
  status.awb = cameraSetup["awb"].as<AutoConnectCheckbox>().checked ? 1 : 0;
  status.awb_gain = cameraSetup["wbg"].as<AutoConnectCheckbox>().checked ? 1 : 0;
  const String& wbMode = cameraSetup["wbm"].as<AutoConnectSelect>().value();
  if (wbMode == "Auto")
    status.wb_mode = 0;
  else if (wbMode == "Sunny")
    status.wb_mode = 1;
  else if (wbMode == "Cloudy")
    status.wb_mode = 2;
  else if (wbMode == "Office")
    status.wb_mode = 3;
  else if (wbMode == "Home")
    status.wb_mode = 4;

  // Automatic Exposure Control, Turn off AEC to set the exposure level.
  status.aec = cameraSetup["aec"].as<AutoConnectCheckbox>().checked ? 1 : 0;
  status.aec2 = cameraSetup["dsp"].as<AutoConnectCheckbox>().checked ? 1 : 0;
  status.ae_level = cameraSetup["ael"].as<AutoConnectRange>().value;
  status.aec_value = cameraSetup["exp"].as<AutoConnectRange>().value;

  // Automatic Gain Control
  status.agc = cameraSetup["agc"].as<AutoConnectCheckbox>().checked ? 1 : 0;
  status.agc_gain = cameraSetup["agv"].as<AutoConnectRange>().value - 1;
  status.gainceiling = cameraSetup["acl"].as<AutoConnectRange>().value - 1;

  // Gamma (GMA) function is to compensate for the non-linear characteristics of
  // the sensor. Raw gamma compensates the image in the RAW domain.
  status.raw_gma = cameraSetup["gma"].as<AutoConnectCheckbox>().checked ? 1 : 0;
  
  // Defect pixel cancellation, Black pixel and White pixel
  status.bpc = cameraSetup["bpc"].as<AutoConnectCheckbox>().checked ? 1 : 0;
  status.wpc = cameraSetup["wpc"].as<AutoConnectCheckbox>().checked ? 1 : 0;

  // Lense correction, According to the area where each pixel is located,
  // the module calculates a gain for the pixel, correcting each pixel with its
  // gain calculated to compensate for the light distribution due to lens curvature.
  status.lenc = cameraSetup["lec"].as<AutoConnectCheckbox>().checked ? 1 : 0;

  // Mirror and Flip
  status.hmirror = cameraSetup["hmi"].as<AutoConnectCheckbox>().checked ? 1 : 0;
  status.vflip = cameraSetup["vfl"].as<AutoConnectCheckbox>().checked ? 1 : 0;

  // Reducing image frame UXGA to XGA
  status.dcw = cameraSetup["dcw"].as<AutoConnectCheckbox>().checked ? 1 : 0;

  // Reflecting the setting values on the sensor
  if (!webcam.sensor().setStatus(status))
    Serial.println("Failed to set camera sensor");

  // Sends a redirect to forward to the root page displaying the streaming from
  // the camera.
  aux.redirect(viewerUrl().c_str());

  return String();
}


String readFromFile(String filename_, int target_line_, int min_length_, int max_length_, String default_value_, String description_){
  // init Telegram Bot
  // read hostname and password from SD card
  File file = SD_MMC.open(filename_);
  String result = default_value_;
  
  if (!file) {
    Serial.print("Failed to open file: ");
    Serial.println(filename_);
    Serial.print("Using default ");
    Serial.print(description_);
    Serial.print(": ");
    Serial.println(default_value_);
  }else
  {
    std::vector<String> v;

    while (file.available()) {
      v.push_back(file.readStringUntil('\n'));
    }
    file.close();
    Serial.print("content of ");
    Serial.print(filename_);
    Serial.println(": ");
    for (String s : v) {
      Serial.println(s);
    }
    if (v[0].length() >= min_length_) {
      if(v[0].length() <= max_length_) {
        result = v[target_line_];
      }else
      {
        Serial.print("Hostname was too long, please use min. ");
        Serial.print(max_length_);
        Serial.println(" characters");
        Serial.print("Using default ");
        Serial.print(description_);
        Serial.print(": ");
        Serial.println(default_value_);
      }
    }else
    {
      Serial.print("Hostname was too short, please use min. ");
      Serial.print(min_length_);
      Serial.println(" characters");
      Serial.print("Using default ");
      Serial.print(description_);
      Serial.print(": ");
      Serial.println(default_value_);
    }
  }
  return result;
}

static int taskCore = 1;
 
void coreTask( void * pvParameters ){
 
    String taskMessage = "Task running on core ";
    taskMessage = taskMessage + xPortGetCoreID();
 
    while(true){
        Serial.println(taskMessage);
        delay(1000);
    }
 
}



void telegramTask_(){
//  if (millis() - lastTimeBotRan >= botRequestDelay){
//      Serial.println("check for telegram messages");
//      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
//      while(numNewMessages) {
//        Serial.println("got response");
//        handleNewMessages(numNewMessages);
//        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
//      }
//      lastTimeBotRan = millis(); 
//    }
//  if (millis() > lastTimeBotRan + botRequestDelay)  {
//    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
//    while (numNewMessages) {
//      Serial.println("got response");
//      handleNewMessages(numNewMessages);
//      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
//    }
//    lastTimeBotRan = millis();
//  }
//  File myFile;
//  if (millis() - lastTimeBotRan > botRequestDelay)  {
//    String file_name = webcam.getLatestFile();
//    Serial.print("Telegram Task got filename: ");
//    Serial.println(file_name);
//    
//    myFile = SD_MMC.open(file_name);
//
//      if (myFile) {
//        Serial.print(file_name);
//        Serial.print("....");
//  
//        //Content type for PNG image/png
//       // String sent = bot.sendPhotoByBinary(CHAT_ID, "image/jpeg", (int)myFile.size(), isMoreDataAvailable(myFile), getNextByte(myFile), nullptr, nullptr);
//
//        //bot.sendMessage(CHAT_ID, "Hi:","");         //THIS WORKS
//        //bot.sendMessage(CHAT_ID, CHAT_ID,"");       //THIS WORKS
//  
//        //if (sent) {
//        //  Serial.println("was successfully sent");  //THIS IS PRINTED IN MY SERIAL MONITOR
//        //} else {
//        //  Serial.println("was not sent");
//        //}
//  
//    } else {
//      // if the file didn't open, print an error:
//      Serial.println("error opening photo");
//    }
//    myFile.close();
//  }  

  
  
}
void telegramTask(void *parameter){
  String taskMessage = "Telegram Task running on core ";
  taskMessage = taskMessage + xPortGetCoreID();
  Serial.println(taskMessage);
  
  while(true){
    telegramTask_();
    vTaskDelay(100);
  }
}  

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  if (SD_MMC.begin ())
    Serial.println ("File system opened (" + String ("SD_MMC") + ")");
  else
    Serial.println ("File system could not be opened; ftp server will not work");

  esp_hostname = readFromFile("/hostname.txt", 0, MIN_LENGTH_HOSTNAME, MAX_LENGTH_HOSTNAME, ESP_HOSTNAME, "Hostname");
  esp_password = readFromFile("/password.txt", 0, MIN_LENGTH_PASSWORD, MAX_LENGTH_PASSWORD, ESP_PASSWORD, "Password");
  
  // Initialize the image sensor during the start phase of the sketch.
  esp_err_t err = webcam.sensorInit(model);
  if (err != ESP_OK)
    Serial.printf("Camera init failed 0x%04x\n", err);
  Serial.println("Camera init successful");
  
  // Loading the image sensor configurarion UI provided by AutoConnectAux.
  portal.load(FPSTR(CAMERA_SETUP_PAGE));
  if (portal.load(FPSTR(CAMERA_SETUP_EXEC)))
    portal.on(_setUrl, setSensor);
  Serial.println("AutoCOnnect Portal started");
  
  // The various configuration settings of AutoConnect are also useful for
  // using the ESP32WebCam class.
  config.apid = esp_hostname;
  config.psk  = esp_password;
  config.hostName = esp_hostname;
  config.autoReconnect = true;
  config.reconnectInterval = 1;
  config.ota = AC_OTA_BUILTIN;
  portal.config(config);

  if (portal.begin()) {
    // Allow hostname.local reach in the local segment by mDNS.
    MDNS.begin(WiFi.getHostname());
    MDNS.addService("http", "tcp", 80);
    Serial.printf("AutoConnect portal %s.local started\n", WiFi.getHostname());

    // By configuring NTP, the timestamp appended to the capture filename will
    // be accurate. But this procedure is optional. It does not affect ESP32Cam
    // execution.
    configTzTime(_tz, _ntp1 ,_ntp2);

    // Activate ESP32WebCam while WiFi is connected.
    err = webcam.startCameraServer();

    if (err == ESP_OK) {
      // This sketch has two endpoints. One assigns the root page as the
      // entrance, and the other assigns a redirector to lead to the viewer-UI
      // which we have placed in SPIFFS with the name webcamview.html.
      portalServer.on("/", handleRootPage);
      portalServer.on(_viewerUrl, handleViewPage);
      Serial.printf("Camera streaming server  %s ready, port %u\n", WiFi.localIP().toString().c_str(), webcam.getServerPort());
      Serial.printf("Endpoint Capture:%s, Prompt:%s, Stream:%s\n", webcam.getCapturePath(), webcam.getPromptPath(), webcam.getStreamPath());
    }
    else
      Serial.printf("Camera server start failed 0x%04x\n", err);
  }

  ftpSrvSD.begin (esp_hostname, esp_password.c_str());    //username, password for ftp.  set ports in ESPFtpServer.h  (default 21, 50009 for PASV)
  Serial.println("FTP Server Started");
  
  delay(200);
  
  ESP32WebCam::_initFastLED();
  //ESP32WebCam::_showLED();
  Serial.println("LED Controller Started");
  
  delay(200);
  
  String telegram_token = readFromFile("/telegram_token.txt", 0, 0, 200, TELEGRAM_BOT_TOKEN, "Telegram Bot Token");
  String telegram_chat_id = readFromFile("/telegram_chat_id.txt", 0, 0, 200, TELEGRAM_CHAT_ID, "Telegram Chat ID");

  ESP32WebCam::_initTelegramBot(telegram_token, telegram_chat_id);
  Serial.println("Telegram Bot initialized");
  
  //listDir("/");

  //lastTimeBotRan = millis();

//  xTaskCreatePinnedToCore(
//                    telegramTask,   /* Function to implement the task */
//                    "telegramTask", /* Name of the task */
//                    10000,      /* Stack size in words */
//                    NULL,       /* Task input parameter */
//                    0,          /* Priority of the task */
//                    NULL,       /* Task handle. */
//                    0);  /* Core where the task should run */
  }  
  
void loop() {
  // The handleClient is needed for WebServer class hosted with AutoConnect.
  // ESP-IDF Web Server component launched by the ESP32WebCam continues in a
  // separate task.
  portal.handleClient();  
  ftpSrvSD.handleFTP (SD_MMC);        //make sure in loop you call handleFTP()!

  /*
  File myFile;
  if (millis() - lastTimeBotRan > botRequestDelay)  
  {
    String file_name = webcam.getLatestFile();
    Serial.print("Telegram Task got filename: ");
    Serial.println(file_name);
    
    myFile = SD_MMC.open(file_name);

    if (myFile) {
      //Content type for PNG image/png
      //String sent = bot.sendPhotoByBinary(CHAT_ID, "image/jpeg", myFile.size(), isMoreDataAvailable(myFile), getNextByte(myFile), nullptr, nullptr);

      //bot.sendMessage(CHAT_ID, "Hi:","");         //THIS WORKS
      //bot.sendMessage(CHAT_ID, CHAT_ID,"");       //THIS WORKS
  
      //if (sent) {
      //  Serial.println("was successfully sent");  //THIS IS PRINTED IN MY SERIAL MONITOR
      //} else {
      //  Serial.println("was not sent");
      //}
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening photo");
    }
    
    myFile.close();
    lastTimeBotRan = millis();
  }  

  */
  //telegramTask_();
  //// Allow CPU to switch to other tasks.
  delay(1);
}
