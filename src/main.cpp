/* **************************************************************************
 * KynexOs Sovereign Build v230.32 - The OPI Awakening
 * Geliştirici: Muhammed (Kynex)
 * Görev: 8MB PSRAM Boot, 40MHz SPI, Aligned JPG Decoding
 * Donanım: ESP32-S3 N16R8 (V325 Pinout)
 * Talimat: Asla satır silmeden, optimize etmeden, tam ve tek parça kod.
 * **************************************************************************
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
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

unsigned long lock_timer = 0;
bool psram_ok = false;

// JPG Çizim Fonksiyonu (WDT Beslemeli)
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    esp_task_wdt_reset(); 
    return true;
}

void setup() {
    // 1. Seri Port ve Diagnostic
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n\n========================================");
    Serial.println("KYNEX-OS V230.32 - OPI AWAKENING START");
    Serial.println("========================================");

    // 2. PSRAM KONTROLÜ (8MB OPI)
    if (psramInit()) {
        Serial.printf("[SYSTEM] 8MB OPI PSRAM Basariyla Uyandi! Bos Alan: %d KB\n", ESP.getFreePsram() / 1024);
        psram_ok = true;
    } else {
        Serial.println("[HATA] PSRAM Uyanamadi! OPI Hatasi Olabilir.");
    }

    // 3. Güvenlik
    esp_task_wdt_init(30, true);
    esp_ota_mark_app_valid_cancel_rollback();

    // 4. Ekran Pinleri ve SPI Başlatma
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(40000000); // 40MHz Yüksek Hız
    tft.setRotation(1);
    
    // Retro-Go Renk ve Yön Senkronizasyonu
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);

    tft.fillScreen(0x0000);
    
    // 5. JPG Motorunu Çalıştır
    Serial.println("[SYSTEM] Windows 10 JPG Çözülüyor...");
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    
    // Resim verisi kontrolü ve Çizimi
    if (wallpaper_jpg_len > 1024) {
        TJpgDec.drawJpg(0, 0, wallpaper_jpg, wallpaper_jpg_len);
        Serial.println("[SYSTEM] Sovereign Wallpaper Cizildi!");
    } else {
        Serial.println("[HATA] Wallpaper array okunamadi veya eksik!");
        tft.setTextColor(0xF800);
        tft.setCursor(20, 100);
        tft.print("HATA: HEX VERISI BOZUK!");
    }

    // 6. Arayüz Süsleri
    tft.fillRect(0, 215, 320, 25, 0x10A2);
    tft.fillRect(2, 217, 20, 20, 0x03FF);
    tft.drawRect(2, 217, 20, 20, 0xFFFF);
    
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.setCursor(275, 224);
    tft.print("23:15");
    
    tft.setCursor(60, 224);
    tft.setTextColor(0x07E0);
    if(psram_ok) {
        tft.print("PSRAM 8MB ACTIVE - STABLE");
    } else {
        tft.print("SOVEREIGN JPG STABLE");
    }

    lock_timer = millis();
    Serial.println("[SYSTEM] Sistem Hazir. 5 saniyelik tus kilidi devrede.");
}

void loop() {
    esp_task_wdt_reset();

    // 5 Saniye Tuş Koruması (Ghost Trigger Kalkanı)
    if (millis() - lock_timer > 5000) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(200);
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
