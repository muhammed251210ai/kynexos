/* **************************************************************************
 * KynexOs Sovereign Build v230.23 - The Iron Gate
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Ghost Switch Shield, Forced Serial Log, Pure GFX
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

#include "wallpaper.h"

// PIN TANIMLARI
#define TFT_BL 1
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 14
#define TFT_MOSI 11
#define TFT_SCK 12
#define TFT_MISO 13
#define JOY_SELECT 6

#define WIN_TASKBAR 0x10A2
#define WIN_START   0x03FF

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
WebServer server(80);

unsigned long boot_lock_timer = 0;
bool gate_open = false;

void drawSovereignUI() {
    drawWin10Background(&tft);
    tft.fillRect(0, 215, 320, 25, WIN_TASKBAR);
    tft.fillRect(2, 217, 20, 20, WIN_START);
    tft.drawRect(2, 217, 20, 20, ILI9341_WHITE);
    tft.drawLine(12, 217, 12, 237, ILI9341_WHITE);
    tft.drawLine(2, 227, 22, 227, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(275, 224);
    tft.print("20:00"); 
    tft.fillRect(20, 20, 40, 40, 0x3186); 
    tft.drawRect(20, 20, 40, 40, ILI9341_WHITE);
    tft.setCursor(15, 65);
    tft.print("RETRO-GO");
    tft.setCursor(100, 224);
    tft.setTextColor(0x07E0); 
    tft.print("IRON GATE ACTIVE");
}

void setup() {
    // 1. Seri Portu Hemen Aç (Teşhis için en önemli adım)
    Serial.begin(115200);
    delay(3000); // Muhammed, seri monitörü açman için sana 3 saniye süre!
    Serial.println("\n\n====================================");
    Serial.println("KYNEX-OS SOVEREIGN: IRON GATE START");
    Serial.println("====================================");

    esp_task_wdt_init(30, true);
    esp_ota_mark_app_valid_cancel_rollback();

    // 2. Hayalet Tuşu Mühürle
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    // 3. Ekran Başlatma
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setRotation(1);
    
    drawSovereignUI();

    // 4. WiFi
    WiFi.softAP("Kynex-Sovereign", "*muhammed*");
    server.on("/", []() { server.send(200, "text/plain", "Sovereign Stable"); });
    server.begin();

    boot_lock_timer = millis();
    gate_open = true;
    Serial.println("[SYSTEM] Sistem Stabil ve Ayakta.");
}

void loop() {
    server.handleClient();
    esp_task_wdt_reset();

    // MUHAMMED: İlk 10 saniye boyunca Select tuşu ne derse desin sistem reset ATMAYACAK!
    if (gate_open && (millis() - boot_lock_timer > 10000)) {
        if (digitalRead(JOY_SELECT) == LOW) {
            delay(500); // Çok güçlü filtre
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
