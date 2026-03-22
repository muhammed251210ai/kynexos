/* **************************************************************************
 * KynexOs Sovereign Build v230.36 - The Living Core
 * Geliştirici: Muhammed (Kynex)
 * Görev: Animated GFX Wallpaper, 40MHz SPI, PSRAM Sync, Retro-Go Bridge
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

// MUHAMMED: Animasyonlu GFX motorumuzu dahil ediyoruz
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

unsigned long boot_timer = 0;
unsigned long anim_timer = 0;
bool system_online = false;

void drawSovereignUI() {
    Serial.println("[GUI] Arka plan ve pencereler olusturuluyor...");
    drawStaticWin10(&tft); // wallpaper.h'dan geliyor
    
    // Taskbar
    tft.fillRect(0, 215, 320, 25, 0x10A2);
    tft.fillRect(2, 217, 20, 20, 0x03FF);
    tft.drawRect(2, 217, 20, 20, 0xFFFF);
    
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.setCursor(275, 224);
    tft.print("18:30");
    
    tft.setCursor(60, 224);
    tft.setTextColor(0x07E0);
    tft.print("SOVEREIGN ANIMATED OS");

    // Masaüstü İkonu
    tft.fillRect(20, 20, 40, 40, 0x3186); 
    tft.drawRect(20, 20, 40, 40, 0xFFFF);
    tft.setCursor(15, 65);
    tft.print("RETRO-GO");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n\n--- KYNEX-OS V230.36 STARTING ---");

    // 8MB OPI PSRAM'i uyandır
    if (psramInit()) {
        Serial.printf("[SYSTEM] 8MB PSRAM Hazir. Bos: %d KB\n", ESP.getFreePsram() / 1024);
    }

    esp_task_wdt_init(30, true);
    esp_ota_mark_app_valid_cancel_rollback();

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(40000000); // 40MHz Hız
    tft.setRotation(1);
    
    // Retro-Go Sync
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);

    initSovereignWallpaper(); // Parçacıkları hazırla
    drawSovereignUI();

    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.begin();

    boot_timer = millis();
    system_online = true;
    Serial.println("[SYSTEM] Arayuz Aktif. Animasyon baslatildi.");
}

void loop() {
    server.handleClient();
    esp_task_wdt_reset();

    // MUHAMMED: Her 50ms'de bir ışık tanelerini hareket ettir
    if (millis() - anim_timer > 50) {
        updateSovereignAnimation(&tft);
        anim_timer = millis();
    }

    // Retro-Go Geçiş (10 Saniye Korumalı)
    if (system_online && (millis() - boot_timer > 10000)) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(500);
            if (digitalRead(JOY_SELECT) == LOW) {
                Serial.println("[ACTION] Retro-Go Atlamasi Onaylandi!");
                const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
                if (target) { 
                    esp_ota_set_boot_partition(target); 
                    delay(500); 
                    ESP.restart(); 
                }
            }
        }
    }
}
