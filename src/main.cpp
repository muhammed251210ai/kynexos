/* **************************************************************************
 * KynexOs Sovereign Build v230.26 - JPG Vision Edition
 * Geliştirici: Muhammed (Kynex)
 * Görev: 40MHz SPI, Aligned JPG Decoding, WDT Protection
 * Donanım: ESP32-S3 N16R8 (V325 Pinout)
 * Talimat: Asla satır silmeden, optimize etmeden, tam ve tek parça kod.
 * **************************************************************************
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TJpg_Decoder.h> // JPG Kütüphanesi geri döndü!
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"

#include "wallpaper.h"

// V325 DONANIM HARİTASI
#define TFT_BL     1
#define TFT_MISO   13
#define TFT_MOSI   11
#define TFT_SCK    12
#define TFT_CS     10
#define TFT_DC     9
#define TFT_RST    14
#define JOY_SELECT 6 

#define WIN_TASKBAR 0x10A2
#define WIN_START   0x03FF

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
WebServer server(80);

unsigned long boot_safe_timer = 0;

// JPG Çizim Fonksiyonu (İşlemciye her blokta nefes aldırır)
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    esp_task_wdt_reset(); // MUHAMMED: Çizim sırasında reset atmayı engeller
    return true;
}

void drawSovereignWin10() {
    Serial.println("[GUI] JPG Wallpaper cozülüyor...");
    
    // Önce arka planı temizle (Yükleme sırasında siyah görünmesin)
    tft.fillScreen(0x0011); 

    // MUHAMMED: Gerçek JPG çizimi burada başlar
    if (wallpaper_jpg_len > 100) {
        TJpgDec.drawJpg(0, 0, wallpaper_jpg, wallpaper_jpg_len);
    } else {
        Serial.println("[HATA] Wallpaper verisi boş veya çok kısa!");
        tft.setCursor(10, 100);
        tft.setTextColor(ILI9341_RED);
        tft.print("RESIM VERISI EKLE!");
    }
    
    // Win10 Arayüz Elemanları
    tft.fillRect(0, 215, 320, 25, WIN_TASKBAR);
    tft.fillRect(2, 217, 20, 20, WIN_START);
    tft.drawRect(2, 217, 20, 20, ILI9341_WHITE);
    tft.drawLine(12, 217, 12, 237, ILI9341_WHITE);
    tft.drawLine(2, 227, 22, 227, ILI9341_WHITE);

    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(275, 224);
    tft.print("21:28"); 
    
    tft.fillRect(20, 20, 40, 40, 0x3186); 
    tft.drawRect(20, 20, 40, 40, ILI9341_WHITE);
    tft.setCursor(15, 65);
    tft.print("RETRO-GO");

    tft.setCursor(100, 224);
    tft.setTextColor(0x07E0); 
    tft.print("JPG VISION ACTIVE");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- SOVEREIGN OS JPG BOOT ---");

    esp_task_wdt_init(30, true); // Süreyi 30 saniyeye çıkardık
    esp_ota_mark_app_valid_cancel_rollback();

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(40000000); // 40MHz Hız
    tft.setRotation(1);
    
    // Ekran Senkronizasyonu
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);
    
    // JPG Motoru Ayarları
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    
    // Arayüzü Çiz
    drawSovereignWin10();

    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.begin();

    boot_safe_timer = millis();
    Serial.println("--- SYSTEM STABLE WITH JPG ---");
}

void loop() {
    server.handleClient();
    esp_task_wdt_reset();

    // 10 saniye tuş koruması
    if (millis() - boot_safe_timer > 10000) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(500);
            if (digitalRead(JOY_SELECT) == LOW) {
                const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
                if (target) { esp_ota_set_boot_partition(target); delay(1000); ESP.restart(); }
            }
        }
    }
}
