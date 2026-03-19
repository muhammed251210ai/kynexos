/* **************************************************************************
 * KynexOs Sovereign Build v230.22 - The Pure Kernel
 * Geliştirici: Muhammed (Kynex)
 * Görev: Windows 10 UI, Anti-Rollback, Zero-Image GFX, Retro-Go Bridge
 * Donanım: ESP32-S3 N16R8 (DIO 40MHz)
 * Talimat: Asla satır silmeden, optimize etmeden, tam ve tek parça kod.
 * **************************************************************************
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

// MUHAMMED: Saf GFX tabanlı duvar kağıdı motoru (Resim gerektirmez)
#include "wallpaper.h"

// PIN TANIMLAMALARI (Sovereign Standart)
#define TFT_BL 1
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 14
#define TFT_MOSI 11
#define TFT_SCK 12
#define TFT_MISO 13
#define TOUCH_CS 16
#define JOY_SELECT 6

// WIN10 TEMA RENKLERİ
#define WIN_TASKBAR 0x10A2
#define WIN_START   0x03FF
#define WIN_ICON    0x3186

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
WebServer server(80);

unsigned long system_uptime = 0;
bool system_locked = true;

// UI Çizim Motoru
void drawWin10SovereignUI() {
    Serial.println("[GUI] Sovereign Wallpaper Motoru Calisiyor...");
    // MUHAMMED: Resim dosyası olmadan Win10 atmosferini wallpaper.h ile çiziyoruz
    drawWin10Background(&tft);
    
    Serial.println("[GUI] Gorev Cubugu ve Bileşenler Yukleniyor...");
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
    tft.print("19:55"); 
    
    // Masaüstü "Oyunlar" İkonu
    tft.fillRect(20, 20, 40, 40, WIN_ICON); 
    tft.drawRect(20, 20, 40, 40, ILI9341_WHITE);
    tft.setCursor(15, 65);
    tft.print("RETRO-GO");

    // Sistem Durum Yazısı
    tft.setCursor(100, 224);
    tft.setTextColor(0x07E0); // Yeşil renk
    tft.print("SOVEREIGN ONLINE");
    
    Serial.println("[GUI] Arayuz Basariyla Olusturuldu.");
}

// Retro-Go (OTA_1) Geçiş Köprüsü
void switchToRetroGo() {
    Serial.println("[SYSTEM] Retro-Go Yukleme Komutu Alindi!");
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
        Serial.println("[HATA] Retro-Go bolumu haritada bulunamadi!");
        tft.fillScreen(ILI9341_RED);
        tft.setCursor(20, 150);
        tft.print("HATA: RETROGO YOK!");
    }
}

void setup() {
    // 1. Seri Haberleşme ve Teşhis
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- KYNEX-OS SOVEREIGN BOOTING ---");
    Serial.print("Boot Reason: "); Serial.println(esp_reset_reason());

    // 2. Güvenlik Mühürleri (Anti-Rollback)
    esp_task_wdt_init(20, true); // Bekçi köpeği 20 saniye
    esp_ota_mark_app_valid_cancel_rollback();

    // 3. Pin ve Donanım Hazırlığı
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setRotation(1);
    
    // 4. UI Oluşturma
    drawWin10SovereignUI();

    // 5. Network Hizmetleri
    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.on("/", []() {
        server.send(200, "text/plain", "Sovereign Kernel v230.22 Online");
    });
    server.begin();

    system_uptime = millis();
    system_locked = false;
    Serial.println("--- SOVEREIGN IS READY AND STABLE ---");
}

void loop() {
    server.handleClient();
    esp_task_wdt_reset(); // Bekçi köpeğini besle

    // MUHAMMED: Hayalet resetleri engellemek için ilk 5 saniye tuş koruması
    if (!system_locked && (millis() - system_uptime > 5000)) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(200); // Filtre
            if (digitalRead(JOY_SELECT) == LOW) {
                switchToRetroGo();
            }
        }
    }
}
