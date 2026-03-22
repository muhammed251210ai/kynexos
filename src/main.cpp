/* **************************************************************************
 * KynexOs Sovereign Build v230.44 - The Grand Architect
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Stealth Boot, Win10 Start Menu, Settings, TR Keyboard, Preferences
 * Donanım: ESP32-S3 N16R8 (V325 Pinout)
 * Talimat: Asla satır silmeden, optimize etmeden, tam ve tek parça kod.
 * **************************************************************************
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"
#include "wallpaper.h"

// DONANIM HARİTASI
#define TFT_BL     1
#define TFT_MISO   13
#define TFT_MOSI   11
#define TFT_SCK    12
#define TFT_CS     10
#define TFT_DC     9
#define TFT_RST    14
#define TOUCH_CS   16
#define JOY_SELECT 6

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS);
Preferences prefs;

// SİSTEM DURUMLARI
enum Page { DESKTOP, START_MENU, SETTINGS, KEYBOARD_SSID, KEYBOARD_PASS };
Page currentPage = DESKTOP;

bool whiteTheme = false;
bool wifiEnabled = false;
String wifiSSID = "";
String wifiPASS = "";
String tempInput = "";

void renderDesktop() {
    drawSovereignWallpaper(&tft, whiteTheme);
    // Taskbar
    tft.fillRect(0, 215, 320, 25, whiteTheme ? 0xAD75 : 0x10A2);
    // WIN TUŞU (Sol Alt)
    tft.fillRect(2, 217, 35, 21, 0x03FF);
    tft.drawRect(2, 217, 35, 21, 0xFFFF);
    tft.setCursor(8, 224); tft.setTextColor(0xFFFF); tft.print("WIN");
    // Durum Çubuğu
    tft.setCursor(240, 224); 
    tft.print(wifiEnabled ? "WIFI: ON" : "STEALTH");
}

void renderStartMenu() {
    tft.fillRect(0, 80, 130, 135, whiteTheme ? 0xFFFF : 0x2104);
    tft.drawRect(0, 80, 130, 135, 0x03FF);
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setCursor(10, 105); tft.print("> AYARLAR");
    tft.setCursor(10, 145); tft.print("> OYUNLAR (OTA1)");
    tft.setCursor(10, 185); tft.print("> RESTART");
}

void renderSettings() {
    tft.fillScreen(whiteTheme ? 0xFFFF : 0x0000);
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setTextSize(2);
    tft.setCursor(10, 10); tft.print("SISTEM AYARLARI");
    tft.setTextSize(1);
    
    // WiFi Toggle
    tft.drawRect(10, 50, 300, 40, 0x03FF);
    tft.setCursor(20, 65); tft.print("WiFi Gucu: ");
    tft.setTextColor(wifiEnabled ? 0x07E0 : 0xF800);
    tft.print(wifiEnabled ? "ACIK" : "KAPALI");
    
    // SSID/PASS Alanları
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.drawRect(10, 100, 145, 40, 0x3186);
    tft.setCursor(20, 115); tft.print("SSID GIR");
    
    tft.drawRect(165, 100, 145, 40, 0x3186);
    tft.setCursor(175, 115); tft.print("SIFRE GIR");

    // Tema Değiştir
    tft.fillRect(10, 150, 300, 35, 0x03FF);
    tft.setCursor(100, 162); tft.setTextColor(0xFFFF);
    tft.print(whiteTheme ? "KOYU TEMA" : "BEYAZ TEMA");

    // Geri Dön
    tft.fillRect(10, 195, 300, 40, 0x07E0);
    tft.setCursor(120, 210); tft.print("KAYDET VE CIK");
}

void setup() {
    // 1. Stealth Mode: Radyoları en başta sustur!
    WiFi.mode(WIFI_OFF);
    btStop();
    esp_wifi_stop();

    Serial.begin(115200);
    delay(1000);
    
    prefs.begin("sov_v325", false);
    whiteTheme = prefs.getBool("theme", false);
    wifiSSID = prefs.getString("ssid", "");
    wifiPASS = prefs.getString("pass", "");

    if (psramInit()) Serial.println("OPI PSRAM Active");

    esp_task_wdt_init(30, true);
    esp_ota_mark_app_valid_cancel_rollback();

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000); tft.setRotation(1);
    
    // Display Sync
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);

    touch.begin();
    renderDesktop();
    Serial.println("--- Sovereign OS v230.44 Ready ---");
}

void loop() {
    esp_task_wdt_reset();

    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int tx = map(p.x, 200, 3800, 0, 320);
        int ty = map(p.y, 200, 3800, 0, 240);

        if (currentPage == DESKTOP) {
            if (tx < 50 && ty > 210) { // WIN TUŞU
                currentPage = START_MENU;
                renderStartMenu();
                delay(300);
            }
        } 
        else if (currentPage == START_MENU) {
            if (tx < 130 && ty > 100 && ty < 130) { // AYARLAR
                currentPage = SETTINGS;
                renderSettings();
                delay(300);
            } else if (tx < 130 && ty > 140 && ty < 170) { // OYUNLAR
                const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
                if (target) { esp_ota_set_boot_partition(target); ESP.restart(); }
            } else {
                currentPage = DESKTOP;
                renderDesktop();
                delay(300);
            }
        }
        else if (currentPage == SETTINGS) {
            if (ty > 50 && ty < 90) { // WiFi Toggle
                wifiEnabled = !wifiEnabled;
                if(wifiEnabled) { esp_wifi_start(); WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str()); }
                else { WiFi.disconnect(); esp_wifi_stop(); }
                renderSettings();
                delay(300);
            }
            if (ty > 150 && ty < 185) { // Tema Toggle
                whiteTheme = !whiteTheme;
                prefs.putBool("theme", whiteTheme);
                renderSettings();
                delay(300);
            }
            if (ty > 195) { // Kaydet ve Çık
                prefs.putString("ssid", wifiSSID);
                prefs.putString("pass", wifiPASS);
                currentPage = DESKTOP;
                renderDesktop();
                delay(300);
            }
        }
    }

    // Acil Geçiş
    if (digitalRead(JOY_SELECT) == LOW) {
        const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
        if (target) { esp_ota_set_boot_partition(target); delay(500); ESP.restart(); }
    }
}
