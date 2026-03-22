/* **************************************************************************
 * KynexOs Sovereign Build v230.39 - The Power Shield
 * Geliştirici: Muhammed (Kynex)
 * Görev: Power Spike Prevention, OPI PSRAM Sync, Sequence Boot
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
#include "esp_wifi.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"

// MUHAMMED: Animasyon motoru
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
unsigned long anim_timer = 0;
bool system_online = false;

void drawSovereignInterface() {
    Serial.println("[GUI] Katmanlar ciziliyor...");
    drawHeroBackground(&tft);
    
    tft.fillRect(0, 215, 320, 25, 0x10A2);
    tft.fillRect(2, 217, 20, 20, 0x03FF);
    
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.setCursor(15, 224);
    tft.print("KYNEX-OS V230.39");
    
    tft.setCursor(240, 224);
    tft.print("POWER-SAFE");
    
    tft.fillRect(20, 20, 40, 40, 0x3186); 
    tft.drawRect(20, 20, 40, 40, 0xFFFF);
    tft.setCursor(15, 65);
    tft.print("RETRO-GO");
}

void setup() {
    // 1. Seri Port ve Gecikmeli Baslangic
    Serial.begin(115200);
    delay(3000); 
    Serial.println("\n\n--- SOVEREIGN OS V325: POWER SHIELD BOOT ---");

    // 2. PSRAM UYANDIRMA
    if (psramInit()) {
        Serial.printf("[SYSTEM] 8MB OPI PSRAM Aktif. Bos Alan: %d KB\n", ESP.getFreePsram() / 1024);
    }

    // 3. WiFi Guc Yonetimi (Reset Engellemek Icin Kritik)
    WiFi.mode(WIFI_AP);
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM); // Guc tasarrufu modu aktif
    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    Serial.println("[SYSTEM] WiFi SoftAP Dusuk Guc Modunda Baslatildi.");

    // 4. Guvenlik Duvarlari
    esp_task_wdt_init(30, true);
    esp_ota_mark_app_valid_cancel_rollback();

    // 5. Ekran Hazirligi (20MHz Stabil Hiz)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000); // Muhammed, SPI hizini 20MHz'e cektik (Daha az guc tuketimi)
    tft.setRotation(1);
    
    // Retro-Go Sync
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);

    // 6. UI Kurulumu
    initSovereignWallpaper();
    drawSovereignInterface();

    server.begin();
    boot_lock_timer = millis();
    system_online = true;
    Serial.println("[SYSTEM] Sistem Stabil ve Sovereign Online.");
}

void loop() {
    server.handleClient();
    esp_task_wdt_reset();

    // Animasyon (İşlemciyi yormamak için her 100ms'de bir)
    if (system_online && (millis() - anim_timer > 100)) {
        updateAnimation(&tft);
        anim_timer = millis();
    }

    // Retro-Go Geçiş (10 Saniye Buton Kilidi)
    if (system_online && (millis() - boot_lock_timer > 10000)) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(500); 
            if (digitalRead(JOY_SELECT) == LOW) {
                Serial.println("[ACTION] Retro-Go Atlamasi Baslatiliyor...");
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
