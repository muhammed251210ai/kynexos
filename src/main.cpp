/* * KynexOs v186.0 - Independent Mode (Secondary System)
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Dual-Boot RetroGo Launcher, Cyber-Grid UI, Smart Web Router
 * Talimat: Asla satır silmeden, optimize etmeden, tam ve tek parça kod.
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <FFat.h>
#include <Preferences.h>
#include "esp_ota_ops.h" 
#include "esp_partition.h"
#include "esp_image_format.h" 

// PINLER
#define TFT_BL 1, TFT_CS 10, TFT_DC 9, TFT_RST 14, TFT_MOSI 11, TFT_SCK 12, TOUCH_CS 16, MISO_PIN 13
#define JOY1_X 4, JOY1_Y 5, JOY1_SW 6, JOY2_X 7, JOY2_Y 15, JOY2_SW 17, SPEAKER_PIN 18

// NESNELER
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, 9, 10, 14);
XPT2046_Touchscreen ts(16); 
WebServer server(80);
Preferences prefs;
File fsUploadFile; 

// FONKSİYONLAR
void drawDesktop();
void handleWebRoot() {
    String h = "<html><body><h1>Kynex Sovereign Web FM</h1>";
    File root = FFat.open("/");
    File file = root.openNextFile();
    while(file) {
        h += "<div>" + String(file.name()) + " (" + String(file.size()/1024) + " KB)</div>";
        file = root.openNextFile();
    }
    h += "<form action='/upload' method='POST' enctype='multipart/form-data'><input type='file' name='u'><input type='submit' value='Yukle'></form></body></html>";
    server.send(200, "text/html", h);
}

void handleFileUpload() {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){ 
        String fn = upload.filename; 
        if(!fn.startsWith("/")) fn = "/" + fn; 
        if (fn.endsWith(".nes")) { FFat.mkdir("/roms/nes"); fn = "/roms/nes/" + upload.filename; }
        fsUploadFile = FFat.open(fn, FILE_WRITE); 
    }
    else if(upload.status == UPLOAD_FILE_WRITE && fsUploadFile){ fsUploadFile.write(upload.buf, upload.currentSize); }
    else if(upload.status == UPLOAD_FILE_END && fsUploadFile){ fsUploadFile.close(); }
}

// ANA SISTEME (RETRO-GO) DÖNÜŞ
void switchToRetroGo() {
    tft.fillScreen(0x0000); tft.setCursor(20, 100); tft.setTextColor(0xFFFF);
    tft.print("RETURNING TO MAIN SYSTEM...");
    const esp_partition_t* retro_part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (retro_part != NULL) {
        esp_ota_set_boot_partition(retro_part);
        delay(500); ESP.restart(); 
    }
}

void setup() {
    pinMode(1, OUTPUT); digitalWrite(1, HIGH);
    SPI.begin(12, 13, 11, 10); tft.begin(); tft.setRotation(1);
    FFat.begin(true); WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.on("/", handleWebRoot);
    server.on("/upload", HTTP_POST, [](){ server.sendHeader("Location", "/"); server.send(303); }, handleFileUpload);
    server.begin();
    tft.fillScreen(0x0011); tft.setCursor(50, 50); tft.print("KYNEX OS ACTIVE");
    tft.setCursor(50, 80); tft.print("IP: 192.168.4.1");
}

void loop() {
    server.handleClient();
    if (digitalRead(6) == LOW) switchToRetroGo(); // SELECT tuşuyla geri dönüş
}
