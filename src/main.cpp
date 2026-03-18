/* * KynexOs v200.0 - Sovereign Primary OS (Win10)
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Primary OS (OTA_0), Anti-Rollback, Switch to RetroGo (OTA_1)
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
#include <TJpg_Decoder.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"

// PINLER
#define TFT_BL 1
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 14
#define TFT_MOSI 11
#define TFT_SCK 12
#define TFT_MISO 13
#define TOUCH_CS 16
#define JOY_SELECT 6

// RENKLER
#define WIN_BLUE 0x0011
#define WIN_TASKBAR 0x10A2
#define WIN_START 0x03FF

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
WebServer server(80);
File fsUploadFile;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

void drawWin10UI() {
    if (FFat.exists("/wallpaper.jpg")) {
        TJpgDec.drawFsJpg(0, 0, "/wallpaper.jpg", FFat);
    } else {
        tft.fillScreen(WIN_BLUE);
    }
    tft.fillRect(0, 215, 320, 25, WIN_TASKBAR);
    tft.fillRect(2, 217, 20, 20, WIN_START);
    tft.drawRect(2, 217, 20, 20, ILI9341_WHITE);
    tft.drawLine(12, 217, 12, 237, ILI9341_WHITE);
    tft.drawLine(2, 227, 22, 227, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(260, 224);
    tft.print("10:24 AM");
    tft.fillRect(20, 20, 40, 40, 0x3186); 
    tft.drawRect(20, 20, 40, 40, ILI9341_WHITE);
    tft.setCursor(15, 65);
    tft.print("Oyunlar");
    tft.setCursor(130, 224);
    tft.setTextColor(0x07E0); 
    tft.print("IP: 192.168.4.1");
}

void handleWebRoot() {
    String h = "<html><body style='background:#1e1e1e;color:white;'><h1>Sovereign Explorer</h1>";
    h += "<form action='/upload' method='POST' enctype='multipart/form-data'><input type='file' name='u'><input type='submit' value='Upload'></form></body></html>";
    server.send(200, "text/html", h);
}

void handleFileUpload() {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
        fsUploadFile = FFat.open("/" + upload.filename, FILE_WRITE);
    } else if(upload.status == UPLOAD_FILE_WRITE && fsUploadFile){
        fsUploadFile.write(upload.buf, upload.currentSize);
    } else if(upload.status == UPLOAD_FILE_END && fsUploadFile){
        fsUploadFile.close();
        ESP.restart();
    }
}

// RETRO-GO (OTA_1) GEÇİŞ KÖPRÜSÜ
void switchToRetroGo() {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(60, 110);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("OYUNLAR ACILIYOR...");
    
    // MUHAMMED: Artık RetroGo ota_1 bölümünde!
    const esp_partition_t* p = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    if (p) { 
        esp_ota_set_boot_partition(p); 
        delay(100); 
        ESP.restart(); 
    }
}

void setup() {
    esp_ota_mark_app_valid_cancel_rollback();
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setRotation(1);
    FFat.begin(true, "/ffat", 10, "storage");
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    drawWin10UI();
    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.on("/", handleWebRoot);
    server.on("/upload", HTTP_POST, [](){ server.send(303); }, handleFileUpload);
    server.begin();
}

void loop() {
    server.handleClient();
    if (digitalRead(JOY_SELECT) == LOW) {
        delay(50);
        if (digitalRead(JOY_SELECT) == LOW) switchToRetroGo();
    }
}
