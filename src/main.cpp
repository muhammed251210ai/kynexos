/* **************************************************************************
 * KynexOs Sovereign Build v230.25 - The Velocity Sync
 * Geliştirici: Muhammed (Kynex)
 * Görev: 40MHz SPI Speed, Pin Uyumlu Başlatıcı, Ghost Trigger Shield
 * Donanım: ESP32-S3 N16R8 (Pinout: Sovereign Handheld V325)
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

#include "wallpaper.h"

// MUHAMMED: V325 DONANIM HARİTASI
#define TFT_BL     1
#define TFT_MISO   13
#define TFT_MOSI   11
#define TFT_SCK    12
#define TFT_CS     10
#define TFT_DC     9
#define TFT_RST    14
#define JOY_SELECT 6 

// WIN10 TEMA RENKLERİ
#define WIN_TASKBAR 0x10A2
#define WIN_START   0x03FF
#define WIN_ICON    0x3186

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
WebServer server(80);

unsigned long boot_lock_timer = 0;
bool system_ready = false;

// UI Çizim Motoru
void drawWin10SovereignUI() {
    Serial.println("[GUI] Yüksek Hızlı Wallpaper Çiziliyor...");
    drawWin10Background(&tft);
    
    // Alt Görev Çubuğu
    tft.fillRect(0, 215, 320, 25, WIN_TASKBAR);
    
    // Windows Başlat Logosu
    tft.fillRect(2, 217, 20, 20, WIN_START);
    tft.drawRect(2, 217, 20, 20, ILI9341_WHITE);
    tft.drawLine(12, 217, 12, 237, ILI9341_WHITE);
    tft.drawLine(2, 227, 22, 227, ILI9341_WHITE);

    // Saat Bilgisi
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(275, 224);
    tft.print("20:40"); 
    
    // Masaüstü "Retro-Go" İkonu
    tft.fillRect(20, 20, 40, 40, WIN_ICON); 
    tft.drawRect(20, 20, 40, 40, ILI9341_WHITE);
    tft.setCursor(15, 65);
    tft.print("RETRO-GO");

    tft.setCursor(100, 224);
    tft.setTextColor(0x07E0); 
    tft.print("40MHZ MODE ACTIVE");
    Serial.println("[GUI] Arayüz Hazır.");
}

// Retro-Go (OTA_1) Geçiş Köprüsü
void switchToRetroGo() {
    Serial.println("[SYSTEM] Retro-Go Yükleme Komutu Alındı!");
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(60, 110);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("OYUNLAR ACILIYOR...");
    
    const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
    if (target) { 
        esp_ota_set_boot_partition(target); 
        delay(1000); 
        ESP.restart(); 
    } else {
        Serial.println("[HATA] Retro-Go bölümü bulunamadı!");
        tft.fillScreen(ILI9341_RED);
        tft.setCursor(20, 150);
        tft.print("HATA: RETROGO YOK!");
    }
}

void setup() {
    // 1. Seri Port Teşhis
    Serial.begin(115200);
    delay(2000); 
    Serial.println("\n\n--- KYNEX-OS V325 HIGH SPEED BOOT ---");

    // 2. Güvenlik
    esp_task_wdt_init(30, true);
    esp_ota_mark_app_valid_cancel_rollback();

    // 3. Pin Konfigürasyonu
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    // 4. SPI ve 40MHz Ekran Başlatma
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    
    // MUHAMMED: Vites Yükseltildi -> 40,000,000 Hz
    tft.begin(40000000); 
    tft.setRotation(1);
    
    // RETRO-GO SYNC COMMANDS
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);
    
    // 5. UI Çiz
    drawWin10SovereignUI();

    // 6. WiFi
    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.on("/", []() { server.send(200, "text/plain", "Sovereign 40MHz Active"); });
    server.begin();

    boot_lock_timer = millis();
    system_ready = true;
    Serial.println("[SYSTEM] 40MHz Ekran Senkronizasyonu Tamamlandı.");
}

void loop() {
    server.handleClient();
    esp_task_wdt_reset();

    // Ghost Reset Fix (10 Saniye Koruması)
    if (system_ready && (millis() - boot_lock_timer > 10000)) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(500); 
            if (digitalRead(JOY_SELECT) == LOW) {
                switchToRetroGo();
            }
        }
    }
}
