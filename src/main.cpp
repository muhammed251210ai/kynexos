/* * KynexOs v230.20 - Hero UI Edition
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Pure GFX Wallpaper, Zero-Image Boot, Ghost Key Shield
 * Talimat: Asla satır silmeden, optimize etmeden, tam ve tek parça kod.
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"

// MUHAMMED: Yeni Saf GFX Duvar Kağıdı Motoru
#include "wallpaper.h"

#define TFT_BL 1
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 14
#define TFT_MOSI 11
#define TFT_SCK 12
#define TFT_MISO 13
#define JOY_SELECT 6

// Win10 UI Renkleri
#define WIN_TASKBAR 0x10A2
#define WIN_START   0x03FF

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
WebServer server(80);

unsigned long boot_time = 0;

void drawWin10Interface() {
    Serial.println("[GUI] Sovereign Wallpaper Ciziliyor...");
    // MUHAMMED: Resim dosyası kullanmadan Win10 atmosferini oluşturuyoruz
    drawSovereignWallpaper(&tft);
    
    Serial.println("[GUI] Gorev Cubugu Olusturuluyor...");
    // Alt Görev Çubuğu
    tft.fillRect(0, 215, 320, 25, WIN_TASKBAR);
    
    // Windows Başlat Butonu
    tft.fillRect(2, 217, 20, 20, WIN_START);
    tft.drawRect(2, 217, 20, 20, ILI9341_WHITE);
    tft.drawLine(12, 217, 12, 237, ILI9341_WHITE);
    tft.drawLine(2, 227, 22, 227, ILI9341_WHITE);

    // Alt Bilgi Ekranı
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(270, 224);
    tft.print("19:45"); 
    
    // Masaüstü Simgesi
    tft.fillRect(20, 20, 40, 40, 0x2104); // Koyu gri ikon alanı
    tft.drawRect(20, 20, 40, 40, ILI9341_WHITE);
    tft.setCursor(15, 65);
    tft.print("OYUNLAR");

    tft.setCursor(110, 224);
    tft.setTextColor(0x07E0); 
    tft.print("SOVEREIGN ONLINE");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- KYNEX-OS HERO BOOT START ---");

    // Bekçi köpeği ve OTA Ayarları
    esp_task_wdt_init(15, true);
    esp_ota_mark_app_valid_cancel_rollback();

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP); // Donanımsal gürültü önleyici
    
    // Ekranı Başlat
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setRotation(1);
    
    // Arayüzü Çiz
    drawWin10Interface();

    // WiFi Erişim Noktası
    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.on("/", []() { server.send(200, "text/plain", "Sovereign UI Active"); });
    server.begin();

    boot_time = millis();
    Serial.println("--- SOVEREIGN IS READY ---");
}

void loop() {
    server.handleClient();
    esp_task_wdt_reset();

    // MUHAMMED: 5 saniye tuş koruması (Hayalet reset fix)
    if (millis() - boot_time > 5000) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(200);
            if (digitalRead(JOY_SELECT) == LOW) {
                Serial.println("[ACTION] Retro-Go Yukleniyor...");
                const esp_partition_t* p = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
                if (p) { 
                    esp_ota_set_boot_partition(p); 
                    delay(500); 
                    ESP.restart(); 
                }
            }
        }
    }
}
