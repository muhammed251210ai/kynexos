/* * KynexOs v190.0 - PlatformIO Professional Build
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: JPG Wallpaper, Web Server, 8MB Offset (0x800000), Dual-Boot
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
#include <TJpg_Decoder.h> // Muhammed: Wallpaper için gerekli
#include "esp_ota_ops.h"
#include "esp_partition.h"

// PIN TANIMLAMALARI
#define TFT_BL 1
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 14
#define TFT_MOSI 11
#define TFT_SCK 12
#define TFT_MISO 13
#define TOUCH_CS 16
#define JOY_SELECT 6

// NESNELER
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
WebServer server(80);
File fsUploadFile;

// JPG Çizim Fonksiyonu (TJpg_Decoder için)
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

void handleWebRoot() {
    String h = "<html><head><style>body{background:#111;color:#0f0;font-family:sans-serif;} .file{padding:10px;border-bottom:1px solid #333;}</style></head><body>";
    h += "<h1>KYNEX SOVEREIGN COMMAND CENTER</h1>";
    h += "<h3>System: 0x800000 (OTA_0)</h3>";
    File root = FFat.open("/");
    File file = root.openNextFile();
    while(file) {
        h += "<div class='file'>" + String(file.name()) + " [" + String(file.size()/1024) + " KB]</div>";
        file = root.openNextFile();
    }
    h += "<hr><form action='/upload' method='POST' enctype='multipart/form-data'>";
    h += "<input type='file' name='u'> <input type='submit' value='Upload to Sovereign'></form></body></html>";
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
        ESP.restart(); // Muhammed: Wallpaper yüklendiyse yenilenmesi için restart
    }
}

void switchToRetroGo() {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(20, 100);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("BOOTING RETRO-GO...");
    const esp_partition_t* p = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    if (p) { esp_ota_set_boot_partition(p); delay(500); ESP.restart(); }
}

void drawWallpaper() {
    if (FFat.exists("/wallpaper.jpg")) {
        TJpgDec.drawFsJpg(0, 0, "/wallpaper.jpg", FFat);
    } else {
        tft.fillScreen(0x0011);
        tft.setCursor(40, 100);
        tft.setTextColor(ILI9341_GREEN);
        tft.setTextSize(2);
        tft.print("KYNEX OS ACTIVE");
        tft.setCursor(40, 130);
        tft.print("No wallpaper.jpg");
    }
}

void setup() {
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);

    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setRotation(1);

    if(!FFat.begin(true, "/ffat", 10, "storage")) {
        tft.fillScreen(ILI9341_RED);
        tft.print("Storage Error!");
    }

    // Wallpaper Çözücü Ayarları
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    drawWallpaper();

    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.on("/", handleWebRoot);
    server.on("/upload", HTTP_POST, [](){ server.sendHeader("Location", "/"); server.send(303); }, handleFileUpload);
    server.begin();

    tft.setCursor(10, 220);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.print("IP: 192.168.4.1 | SELECT to Switch");
}

void loop() {
    server.handleClient();
    if (digitalRead(JOY_SELECT) == LOW) {
        delay(50);
        if (digitalRead(JOY_SELECT) == LOW) switchToRetroGo();
    }
}
