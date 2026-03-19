/* * KynexOs v230.18 - Raw Vision Edition
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Aligned Memory Access, Ghost Trigger Shield, Win10 UI
 * Talimat: Asla satır silmeden, optimize etmeden, tam ve tek parça kod.
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TJpg_Decoder.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"

// MUHAMMED: Aligned olarak oluşturulan resim verisi
#include "wallpaper_data.h"

#define TFT_BL 1
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 14
#define TFT_MOSI 11
#define TFT_SCK 12
#define TFT_MISO 13
#define JOY_SELECT 6

#define WIN_BLUE 0x0011
#define WIN_TASKBAR 0x10A2
#define WIN_START 0x03FF

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
WebServer server(80);

unsigned long boot_lock_timer = 0;

// Çizim fonksiyonu (Bekçi köpeği dostu)
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    esp_task_wdt_reset(); 
    return true;
}

void drawWin10UI() {
    Serial.println("[GUI] Görüntü motoru baslatiliyor...");
    tft.fillScreen(WIN_BLUE);
    yield();
    
    // MUHAMMED: Resim çizimi (Hizalanmiş veri üzerinden)
    if (wallpaper_jpg_len > 0) {
        Serial.printf("[GUI] Wallpaper ciziliyor (%u bytes)...\n", wallpaper_jpg_len);
        TJpgDec.drawJpg(0, 0, wallpaper_jpg, wallpaper_jpg_len);
    }
    yield();

    // Arayüz Elemanları
    tft.fillRect(0, 215, 320, 25, WIN_TASKBAR);
    tft.fillRect(2, 217, 20, 20, WIN_START);
    tft.drawRect(2, 217, 20, 20, ILI9341_WHITE);
    tft.drawLine(12, 217, 12, 237, ILI9341_WHITE);
    tft.drawLine(2, 227, 22, 227, ILI9341_WHITE);

    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(265, 224);
    tft.print("19:35"); 
    
    tft.fillRect(20, 20, 40, 40, 0x3186); 
    tft.drawRect(20, 20, 40, 40, ILI9341_WHITE);
    tft.setCursor(15, 65);
    tft.print("Oyunlar");

    tft.setCursor(100, 224);
    tft.setTextColor(0x07E0); 
    tft.print("Sovereign Online");
    Serial.println("[GUI] Arayüz tamamlandi.");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- SOVEREIGN OS BOOTING ---");

    esp_task_wdt_init(30, true);
    esp_ota_mark_app_valid_cancel_rollback();

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setRotation(1);
    
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    
    drawWin10UI();

    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.on("/", []() { server.send(200, "text/plain", "KynexOS Active"); });
    server.begin();

    boot_lock_timer = millis();
    Serial.println("--- SYSTEM READY ---");
}

void loop() {
    server.handleClient();
    esp_task_wdt_reset();

    // Ghost Reset Fix (10 Saniye Koruması)
    if (millis() - boot_lock_timer > 10000) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(200);
            if (digitalRead(JOY_SELECT) == LOW) {
                const esp_partition_t* p = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
                if (p) { esp_ota_set_boot_partition(p); delay(500); ESP.restart(); }
            }
        }
    }
}
