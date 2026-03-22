/* **************************************************************************
 * KynexOs Sovereign Build v230.40 - Stealth Edition
 * Geliştirici: Muhammed (Kynex)
 * Görev: WiFi/BT Disabled at Boot, OPI PSRAM Active, Power-Safe Startup
 * Donanım: ESP32-S3 N16R8 (V325 Pinout)
 * Talimat: Asla satır silmeden, optimize etmeden, tam ve tek parça kod.
 * **************************************************************************
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"

// MUHAMMED: Animasyonlu duvar kağıdı motoru
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

unsigned long boot_lock_timer = 0;
unsigned long anim_timer = 0;
bool system_stable = false;

void drawSovereignInterface() {
    Serial.println("[GUI] Arayüz Katmanları Oluşturuluyor...");
    drawHeroBackground(&tft); // wallpaper.h'daki mavi gradyan ve pencereler
    
    // Alt Görev Çubuğu (Win10 Stili)
    tft.fillRect(0, 215, 320, 25, 0x10A2);
    tft.fillRect(2, 217, 20, 20, 0x03FF);
    
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.setCursor(15, 224);
    tft.print("KYNEX-OS V230.40");
    
    tft.setCursor(220, 224);
    tft.setTextColor(0xF800); // Kırmızı (WiFi Kapalı Bildirimi)
    tft.print("STEALTH MODE (OFF)");
    
    // Oyunlar İkonu
    tft.fillRect(20, 20, 40, 40, 0x3186); 
    tft.drawRect(20, 20, 40, 40, 0xFFFF);
    tft.setCursor(15, 65);
    tft.setTextColor(0xFFFF);
    tft.print("RETRO-GO");
}

void setup() {
    // 1. Radyo Frekanslarını Hemen Sustur (Enerji Şokunu Önle)
    WiFi.mode(WIFI_OFF);
    btStop();
    esp_wifi_stop();
    
    // 2. Seri Port ve Diagnostic
    Serial.begin(115200);
    delay(2000); 
    Serial.println("\n\n--- SOVEREIGN OS V325: STEALTH BOOT ---");
    Serial.println("[SYSTEM] WiFi ve Bluetooth Kapali (Enerji Koruma Modu).");

    // 3. PSRAM UYANDIRMA (8MB OPI)
    if (psramInit()) {
        Serial.printf("[SYSTEM] 8MB PSRAM Ayakta. Bos Alan: %d KB\n", ESP.getFreePsram() / 1024);
    }

    // 4. Güvenlik ve İzleyici Köpek
    esp_task_wdt_init(30, true);
    esp_ota_mark_app_valid_cancel_rollback();

    // 5. Ekran Donanımını Hazırla (20MHz Ultra Stabil)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000); // Güç tüketimini azaltmak için 20MHz
    tft.setRotation(1);
    
    // Retro-Go Renk ve Yön Senkronizasyonu
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);

    // 6. UI Başlatma
    initSovereignWallpaper();
    drawSovereignInterface();

    boot_lock_timer = millis();
    system_stable = true;
    Serial.println("[SYSTEM] Arayüz Stabil. Sovereign Stealth Online.");
}

void loop() {
    esp_task_wdt_reset();

    // Animasyon Güncelleme (100ms aralıkla düşük güç tüketimi)
    if (system_stable && (millis() - anim_timer > 100)) {
        updateStars(&tft);
        anim_timer = millis();
    }

    // Retro-Go Köprüsü (10 Saniye Sonra Aktif)
    if (system_stable && (millis() - boot_lock_timer > 10000)) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(500); // Debounce
            if (digitalRead(JOY_SELECT) == LOW) {
                Serial.println("[ACTION] Retro-Go Atlaması Onaylandı!");
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
