/* **************************************************************************
 * KynexOs Sovereign Build v230.30 - The True Hero
 * Geliştirici: Muhammed (Kynex)
 * Görev: Pin Sync, 40MHz Speed, Ghost Trigger Shield, True Smooth GFX Hero
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
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"

// MUHAMMED: Hataları giderilmiş True Smooth Gradient motorunu dahil ettik
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

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
WebServer server(80);

unsigned long boot_lock_timer = 0;
bool gate_unlocked = false;

// UI Arayüz Elemanları (Gradyanın Üzerine Çizilir)
void drawSovereignUIElements() {
    Serial.println("[GUI] Gorev Cubugu ve Ikonlar Ciziliyor...");
    
    // Alt Görev Çubuğu
    tft.fillRect(0, 215, 320, 25, 0x10A2);
    
    // Windows Başlat Logosu (Görev Çubuğunda)
    tft.fillRect(2, 217, 20, 20, 0x03FF);
    tft.drawRect(2, 217, 20, 20, ILI9341_WHITE);
    
    // Saat Bilgisi
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(275, 224);
    tft.print("22:15"); 
    
    // Sistem Durum Yazısı
    tft.setCursor(100, 224);
    tft.setTextColor(0x07E0); // Yeşil renk
    tft.print("SOVEREIGN TRUE HERO");
    
    // Masaüstü "Oyunlar" İkonu
    tft.fillRect(20, 20, 40, 40, 0x3186); // Koyu gri ikon alanı
    tft.drawRect(20, 20, 40, 40, ILI9341_WHITE);
    tft.setCursor(15, 65);
    tft.print("OYUNLAR");
    
    Serial.println("[GUI] Arayuz Hazir.");
}

void setup() {
    Serial.begin(115200);
    delay(3000); 
    Serial.println("\n\n--- SOVEREIGN TRUE HERO BOOT ---");

    esp_task_wdt_init(30, true);
    esp_ota_mark_app_valid_cancel_rollback();

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH); 
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(40000000); // 40MHz High Speed
    tft.setRotation(1);
    
    // Sync Commands
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);
    
    // Muhammed, önce orjinal Hero duvar kağıdını derin gradyan ile çiziyoruz
    drawSovereignHeroWallpaper(&tft);
    
    // Sonra üzerine arayüz elemanlarını yerleştiriyoruz
    drawSovereignUIElements();

    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.begin();

    boot_lock_timer = millis();
    gate_unlocked = true;
    Serial.println("[SYSTEM] Sistem Stabil ve Kahramanca Ayakta.");
}

void loop() {
    server.handleClient();
    esp_task_wdt_reset();

    // 10 saniye tuş koruması (Ghost Trigger Fix)
    if (gate_unlocked && (millis() - boot_lock_timer > 10000)) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(500); // Çok güçlü filtre
            if (digitalRead(JOY_SELECT) == LOW) {
                Serial.println("[ACTION] Retro-Go Atlamasi Onaylandi!");
                const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
                if (target) { 
                    esp_ota_set_boot_partition(target); 
                    delay(1000); 
                    ESP.restart(); 
                }
            }
        }
    }
}
