/* **************************************************************************
 * KynexOs Sovereign Build v230.35 - The OPI Harmony
 * Geliştirici: Muhammed (Kynex)
 * Görev: N16R8 OPI Memory Fix, 40MHz SPI, Safe JPG Decoding
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
#include <TJpg_Decoder.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"

// MUHAMMED: İçinde hex kodları olan Windows 10 resmi
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
bool psram_active = false;

// JPG Çizim Fonksiyonu (WDT Beslemeli)
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    esp_task_wdt_reset(); // MUHAMMED: İzleyici köpeği sürekli besliyoruz
    return true;
}

void setup() {
    // 1. Teşhis Portunu Aç
    Serial.begin(115200);
    delay(2500); // Sistemin kendine gelmesi için zaman tanı
    Serial.println("\n\n========================================");
    Serial.println("KYNEX-OS V230.35 - OPI HARMONY BOOT");
    Serial.println("========================================");

    // 2. N16R8 PSRAM (OPI) Senkronizasyonu
    // MUHAMMED: Burası cihazının kaderini belirliyor!
    if (psramInit()) {
        Serial.printf("[SYSTEM] 8MB OPI PSRAM Basariyla Uyandi! Bos Alan: %d KB\n", ESP.getFreePsram() / 1024);
        psram_active = true;
    } else {
        Serial.println("[HATA] PSRAM Uyanamadi! Cihaz dar bogaz yasayabilir.");
    }

    // 3. İzleyici Köpek (Süreyi çok artırdık ki JPG çizerken reset atmasın)
    esp_task_wdt_init(60, true); 
    esp_ota_mark_app_valid_cancel_rollback();

    // 4. Pinler ve Ekran
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(40000000); // 40MHz
    tft.setRotation(1);
    
    // Retro-Go Renk Uyumu
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);

    tft.fillScreen(0x0000);
    
    // 5. JPG Motorunu Çalıştır
    Serial.println("[SYSTEM] Windows 10 Resmi (JPG) Ciziliyor...");
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    
    if (wallpaper_jpg_len > 1024) {
        // Çizimi başlat
        TJpgDec.drawJpg(0, 0, wallpaper_jpg, wallpaper_jpg_len);
        Serial.println("[SYSTEM] Sovereign Wallpaper Ekranda!");
    } else {
        Serial.println("[HATA] Hex verisi eksik veya hatali format!");
        tft.setTextColor(0xF800);
        tft.setCursor(20, 100);
        tft.print("RESIM VERISI BOZUK!");
    }

    // 6. UI Bileşenleri
    tft.fillRect(0, 215, 320, 25, 0x10A2); // Görev Çubuğu
    tft.fillRect(2, 217, 20, 20, 0x03FF);  // Başlat Butonu
    tft.drawRect(2, 217, 20, 20, 0xFFFF);
    
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.setCursor(275, 224);
    tft.print("00:15");
    
    tft.setCursor(60, 224);
    tft.setTextColor(0x07E0);
    if(psram_active) {
        tft.print("OPI PSRAM ACTIVE - STABLE");
    } else {
        tft.print("OPI ERROR - RUNNING SAFE");
    }

    // 7. Ağ Servisleri
    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.begin();

    boot_lock_timer = millis();
    Serial.println("[SYSTEM] Hazir! Tuş kalkanı 10 saniye aktif.");
}

void loop() {
    server.handleClient();
    esp_task_wdt_reset();

    // 10 Saniye Tuş Koruması (Hayalet tetiklemeyi önlemek için)
    if (millis() - boot_lock_timer > 10000) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(500);
            if (digitalRead(JOY_SELECT) == LOW) {
                Serial.println("[ACTION] Retro-Go Atlamasi Istendi!");
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
