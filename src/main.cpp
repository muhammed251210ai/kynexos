/* **************************************************************************
 * KynexOs Sovereign Build v230.27 - JPG Fix Edition
 * Geliştirici: Muhammed (Kynex)
 * Görev: Aligned Hex JPG Decoding, Anti-Crash Shield
 * Talimat: Asla satır silmeden, optimize etmeden, tam ve tek parça kod.
 * **************************************************************************
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TJpg_Decoder.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"

// MUHAMMED: Yeni düzelttiğimiz header dosyası
#include "wallpaper.h"

// PİNLER (V325 Standart)
#define TFT_BL     1
#define TFT_MISO   13
#define TFT_MOSI   11
#define TFT_SCK    12
#define TFT_CS     10
#define TFT_DC     9
#define TFT_RST    14
#define JOY_SELECT 6 

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);

// JPG Çizim Fonksiyonu
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    esp_task_wdt_reset(); 
    return true;
}

void drawWin10Sovereign() {
    Serial.println("[GUI] Wallpaper oluşturuluyor...");
    
    // Güvenlik Kontrolü: Veri en az 1 KB olmalı
    if (wallpaper_jpg_len > 1024) {
        TJpgDec.drawJpg(0, 0, wallpaper_jpg, wallpaper_jpg_len);
    } else {
        tft.fillScreen(0x0011);
        tft.setCursor(20, 100);
        tft.setTextColor(ILI9341_RED);
        tft.setTextSize(2);
        tft.print("HATA: HEX VERISI GECERSIZ!");
        Serial.println("[HATA] Wallpaper dizisi boş veya hatalı formatta!");
    }

    // Taskbar ve İkonlar
    tft.fillRect(0, 215, 320, 25, 0x10A2);
    tft.fillRect(2, 217, 20, 20, 0x03FF);
    tft.setCursor(100, 224);
    tft.setTextColor(0x07E0);
    tft.setTextSize(1);
    tft.print("SOVEREIGN JPG ONLINE");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- SOVEREIGN OS KERNEL V230.27 ---");

    esp_task_wdt_init(30, true);
    esp_ota_mark_app_valid_cancel_rollback();

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(40000000); // 40MHz
    tft.setRotation(1);

    // Sync Commands
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);

    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    
    drawWin10Sovereign();

    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    Serial.println("[SYSTEM] Önyükleme tamamlandı.");
}

void loop() {
    esp_task_wdt_reset();
    if (digitalRead(JOY_SELECT) == LOW) {
        delay(500);
        if (digitalRead(JOY_SELECT) == LOW) {
            const esp_partition_t* p = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
            if (p) { esp_ota_set_boot_partition(p); delay(1000); ESP.restart(); }
        }
    }
}
