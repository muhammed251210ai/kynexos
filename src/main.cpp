/* * KynexOs v191.0 - Win10 Sovereign Edition
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Windows 10 UI, Taskbar, Start Menu Icon, Web FM, 8MB Offset
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

// RENKLER (Win10 Palette)
#define WIN_BLUE 0x0011
#define WIN_TASKBAR 0x10A2
#define WIN_START 0x03FF

// NESNELER
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
WebServer server(80);
File fsUploadFile;

// JPG DECODER ÇIKTI
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

// WIN10 ARAYÜZ ÇİZİMİ
void drawWin10UI() {
    // 1. Duvar Kağıdı Kontrolü
    if (FFat.exists("/wallpaper.jpg")) {
        TJpgDec.drawFsJpg(0, 0, "/wallpaper.jpg", FFat);
    } else {
        tft.fillScreen(WIN_BLUE); // Klasik Win10 Mavisi
    }

    // 2. Alt Görev Çubuğu (Taskbar)
    tft.fillRect(0, 215, 320, 25, WIN_TASKBAR);
    
    // 3. Başlat Butonu (Win Logosu Stili)
    tft.fillRect(2, 217, 20, 20, WIN_START);
    tft.drawRect(2, 217, 20, 20, ILI9341_WHITE);
    tft.drawLine(12, 217, 12, 237, ILI9341_WHITE);
    tft.drawLine(2, 227, 22, 227, ILI9341_WHITE);

    // 4. Saat ve Sistem Bilgisi (Sağ Alt)
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(260, 224);
    tft.print("10:24 AM");
    
    // 5. Masaüstü İkonu (Retro-Go)
    tft.fillRect(20, 20, 40, 40, 0x3186); // İkon kutusu
    tft.drawRect(20, 20, 40, 40, ILI9341_WHITE);
    tft.setCursor(15, 65);
    tft.print("Retro-Go");

    // 6. Masaüstü İkonu (Web FM)
    tft.fillRect(80, 20, 40, 40, 0xFBE0); // Turuncu İkon
    tft.drawRect(80, 20, 40, 40, ILI9341_WHITE);
    tft.setCursor(82, 65);
    tft.print("Web FM");

    // Sistem Durumu
    tft.setCursor(130, 224);
    tft.setTextColor(0x07E0); // Yeşil
    tft.print("IP: 192.168.4.1");
}

void handleWebRoot() {
    String h = "<html><head><title>Win10 Sovereign FM</title><style>body{background:#1e1e1e;color:white;font-family:Segoe UI,sans-serif;} .win-btn{background:#0078d4;color:white;padding:10px;border:none;}</style></head><body>";
    h += "<h1><img src='https://upload.wikimedia.org/wikipedia/commons/4/48/Windows_logo_-_2012_%28dark_blue%29.svg' width='30'> Sovereign Explorer</h1>";
    File root = FFat.open("/");
    File file = root.openNextFile();
    while(file) {
        h += "<div>- " + String(file.name()) + " (" + String(file.size()/1024) + " KB)</div>";
        file = root.openNextFile();
    }
    h += "<hr><form action='/upload' method='POST' enctype='multipart/form-data'><input type='file' name='u'><input class='win-btn' type='submit' value='Upload'></form></body></html>";
    server.send(200, "text/html", h);
}

void handleFileUpload() {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
        String fn = upload.filename;
        if(!fn.startsWith("/")) fn = "/" + fn;
        fsUploadFile = FFat.open(fn, FILE_WRITE);
    } else if(upload.status == UPLOAD_FILE_WRITE && fsUploadFile){
        fsUploadFile.write(upload.buf, upload.currentSize);
    } else if(upload.status == UPLOAD_FILE_END && fsUploadFile){
        fsUploadFile.close();
        ESP.restart();
    }
}

void switchToRetroGo() {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(60, 110);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("RESTARTING...");
    const esp_partition_t* p = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    if (p) { esp_ota_set_boot_partition(p); delay(500); ESP.restart(); }
}

void setup() {
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);

    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setRotation(1);

    if(!FFat.begin(true, "/ffat", 10, "storage")) {
        tft.fillScreen(ILI9341_BLUE);
        tft.print("BSOD: Storage Error!");
    }

    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    
    drawWin10UI();

    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.on("/", handleWebRoot);
    server.on("/upload", HTTP_POST, [](){ server.sendHeader("Location", "/"); server.send(303); }, handleFileUpload);
    server.begin();
}

void loop() {
    server.handleClient();
    if (digitalRead(JOY_SELECT) == LOW) {
        delay(50);
        if (digitalRead(JOY_SELECT) == LOW) switchToRetroGo();
    }
}
