/* * KynexOs v187.0 - Independent Mode (Storage Synced Edition)
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Dual-Boot RetroGo Launcher, Cyber-Grid UI, Smart Web Router, FFat Storage Fix
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
#define TFT_BL 1
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 14
#define TFT_MOSI 11
#define TFT_SCK 12
#define TOUCH_CS 16
#define MISO_PIN 13
#define JOY1_X 4
#define JOY1_Y 5
#define JOY1_SW 6
#define JOY2_X 7
#define JOY2_Y 15
#define JOY2_SW 17
#define SPEAKER_PIN 18

// NESNELER
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS); 
WebServer server(80);
Preferences prefs;
File fsUploadFile; 

// FONKSİYONLAR
void drawDesktop();
void handleWebRoot() {
    String h = "<html><head><title>Kynex Sovereign</title></head><body><h1>Kynex Sovereign Web FM</h1>";
    File root = FFat.open("/");
    File file = root.openNextFile();
    while(file) {
        h += "<div>" + String(file.name()) + " (" + String(file.size()/1024) + " KB)</div>";
        file = root.openNextFile();
    }
    h += "<hr><form action='/upload' method='POST' enctype='multipart/form-data'><input type='file' name='u'><input type='submit' value='Yukle'></form></body></html>";
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

// ANA SISTEME (RETRO-GO) DÖNÜŞ - MUHAMMED: OTA_1 (0x110000) BÖLÜMÜNE ATLAR
void switchToRetroGo() {
    tft.fillScreen(0x0000); 
    tft.setCursor(20, 100); 
    tft.setTextColor(0xFFFF);
    tft.setTextSize(2);
    tft.print("RETURNING TO MAIN SYSTEM...");
    
    // Önce Factory dene, bulamazsa bizim Retro-Go'nun olduğu OTA_1'i bul
    const esp_partition_t* retro_part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    if (retro_part == NULL) {
        retro_part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    }

    if (retro_part != NULL) {
        esp_ota_set_boot_partition(retro_part);
        delay(500); 
        ESP.restart(); 
    } else {
        tft.fillScreen(0xF800);
        tft.setCursor(20, 130);
        tft.print("RETRO-GO PARTITION NOT FOUND!");
    }
}

void setup() {
    // Ekran Işığı
    pinMode(TFT_BL, OUTPUT); 
    digitalWrite(TFT_BL, HIGH);

    // SPI ve Ekran Başlatma
    SPI.begin(TFT_SCK, MISO_PIN, TFT_MOSI, TFT_CS); 
    tft.begin(); 
    tft.setRotation(1);

    // MUHAMMED: KRİTİK FFAT FIX - "storage" etiketli bölümü mount ediyoruz.
    // Eğer mount başarısızsa formatla (true)
    if(!FFat.begin(true, "/ffat", 10, "storage")) {
        tft.fillScreen(0xF800);
        tft.setCursor(10, 10);
        tft.setTextColor(0xFFFF);
        tft.print("FFat Mount Error! Label 'storage' not found.");
        delay(5000);
    }

    // Ağ Ayarları
    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.on("/", handleWebRoot);
    server.on("/upload", HTTP_POST, [](){ server.sendHeader("Location", "/"); server.send(303); }, handleFileUpload);
    server.begin();

    // Arayüz Başlatma
    tft.fillScreen(0x0011); 
    tft.setTextColor(0x07E0); // Yeşil
    tft.setTextSize(2);
    tft.setCursor(50, 50); 
    tft.print("KYNEX OS ACTIVE");
    tft.setTextColor(0xFFFF);
    tft.setCursor(50, 80); 
    tft.print("IP: 192.168.4.1");
    tft.setCursor(50, 110);
    tft.setTextSize(1);
    tft.print("Press SELECT to start Retro-Go");
    
    pinMode(JOY1_SW, INPUT_PULLUP); // SELECT tuşu
}

void loop() {
    server.handleClient();
    
    // SELECT tuşuyla (GPIO 6) Retro-Go'ya geçiş
    if (digitalRead(JOY1_SW) == LOW) {
        delay(50); // Debounce
        if (digitalRead(JOY1_SW) == LOW) {
            switchToRetroGo();
        }
    }
}
