/* **************************************************************************
 * KynexOs Sovereign Build v230.37 - The Eternal Glow
 * Geliştirici: Muhammed (Kynex)
 * Görev: Sequenced Boot, 8MB PSRAM Sync, 40MHz SPI, Animated GFX
 * Donanım: ESP32-S3 N16R8 (V325 Pinout)
 * Talimat: Asla satır silmeden, optimize etmeden, tam ve tek parça kod.
 * **************************************************************************
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <WiFi.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"

// MUHAMMED: Animasyon motorunu dahil ettik
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

unsigned long boot_timer = 0;
unsigned long anim_timer = 0;
bool is_stable = false;

void drawInterface() {
    Serial.println("[GUI] Arka plan olusturuluyor...");
    drawHeroBackground(&tft);
    
    // Görev Çubuğu
    tft.fillRect(0, 215, 320, 25, 0x10A2);
    tft.fillRect(2, 217, 20, 20, 0x03FF);
    
    tft.setTextColor(0xFFFF);
    tft.setTextSize(1);
    tft.setCursor(100, 224);
    tft.print("SOVEREIGN ETERNAL v230.37");
    
    // Masaüstü İkonu
    tft.fillRect(20, 20, 40, 40, 0x3186); 
    tft.drawRect(20, 20, 40, 40, 0xFFFF);
    tft.setCursor(15, 65);
    tft.print("OYUNLAR");
}

void setup() {
    // 1. Kademeli Başlangıç (Enerji Patlamasını Önle)
    Serial.begin(115200);
    delay(3000); 
    Serial.println("\n\n--- SOVEREIGN OS: THE ETERNAL GLOW STARTING ---");

    // 2. PSRAM UYANDIRMA
    if (psramInit()) {
        Serial.printf("[SYSTEM] 8MB OPI PSRAM Aktif. Bos Alan: %d KB\n", ESP.getFreePsram() / 1024);
    }

    // 3. İzleyici Köpek (Watchdog)
    esp_task_wdt_init(30, true);
    esp_ota_mark_app_valid_cancel_rollback();

    // 4. Ekran Hazırlığı
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(40000000); // 40MHz
    tft.setRotation(1);
    
    // Retro-Go Sync
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);

    // 5. UI ve Animasyon Başlatma
    initSovereignStars();
    drawInterface();

    // WiFi (İhtiyaç yoksa kapalı tutmak en stabilidir, ama senin için SoftAP açıyoruz)
    WiFi.softAP("Kynex-Sovereign", "*muhammed*");

    boot_timer = millis();
    is_stable = true;
    Serial.println("[SYSTEM] Sistem Stabil. Sovereign Online.");
}

void loop() {
    esp_task_wdt_reset();

    // 50ms'de bir animasyon güncelle (İşlemciyi yormadan)
    if (is_stable && (millis() - anim_timer > 50)) {
        updateStars(&tft);
        anim_timer = millis();
    }

    // Retro-Go Geçiş (10 Saniye Buton Kilidi)
    if (is_stable && (millis() - boot_timer > 10000)) {
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
