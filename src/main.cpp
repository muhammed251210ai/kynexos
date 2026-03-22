/* **************************************************************************
 * KynexOs Sovereign Build v230.42 - The High-Definition Sync
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Settings, TR QWERTY Engine, WiFi Memory, Theme Switcher
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

// SİSTEM DEĞİŞKENLERİ
enum Page { DESKTOP, START_MENU, SETTINGS, KEYBOARD };
Page currentPage = DESKTOP;

bool whiteTheme = false;
bool autoConnect = false;
String currentSSID = "";
String currentPASS = "";
String inputBuffer = ""; // Klavye girişi için

void renderDesktop() {
    drawSovereignHDWallpaper(&tft, whiteTheme);
    // Taskbar
    tft.fillRect(0, 215, 320, 25, whiteTheme ? 0xAD75 : 0x10A2);
    // Win Tuşu
    tft.fillRect(2, 217, 30, 21, 0x03FF);
    tft.drawRect(2, 217, 30, 21, 0xFFFF);
    tft.setCursor(6, 224); tft.setTextColor(0xFFFF); tft.print("WIN");
    // Saat
    tft.setCursor(270, 224); tft.print("21:00");
}

void renderSettings() {
    tft.fillScreen(whiteTheme ? 0xFFFF : 0x0000);
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setTextSize(2);
    tft.setCursor(10, 10); tft.print("AYARLAR");
    
    tft.setTextSize(1);
    // WiFi Bölümü
    tft.drawRect(10, 40, 300, 40, 0x03FF);
    tft.setCursor(20, 55); tft.print("WiFi: " + (currentSSID == "" ? "Bagli Degil" : currentSSID));
    
    // Tema Butonu
    tft.fillRect(10, 90, 145, 40, 0x3186);
    tft.setCursor(20, 105); tft.setTextColor(0xFFFF);
    tft.print(whiteTheme ? "KOYU TEMA YAP" : "BEYAZ TEMA YAP");
    
    // Retro-Go Butonu
    tft.fillRect(165, 90, 145, 40, 0xF800);
    tft.setCursor(175, 105); tft.print("RETRO-GO'YA GIT");

    // Kaydet / Geri
    tft.fillRect(10, 180, 300, 40, 0x07E0);
    tft.setCursor(110, 195); tft.print("KAYDET VE CIK");
}

// MUHAMMED: Basit TR Klavye Taslağı
void renderKeyboard(String label) {
    tft.fillScreen(0x2104);
    tft.setTextColor(0xFFFF);
    tft.setCursor(10, 10); tft.print(label + ": " + inputBuffer);
    tft.drawRect(5, 30, 310, 180, 0xFFFF);
    tft.setCursor(20, 50); tft.print("Q W E R T Y U I O P");
    tft.setCursor(20, 80); tft.print("A S D F G H J K L");
    tft.setCursor(20, 110); tft.print("Z X C V B N M . @");
    tft.fillRect(200, 160, 100, 30, 0x07E0);
    tft.setCursor(220, 170); tft.print("TAMAM");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // HAFIZAYI YÜKLE
    prefs.begin("sov_sys", false);
    whiteTheme = prefs.getBool("theme", false);
    autoConnect = prefs.getBool("auto_wifi", false);
    currentSSID = prefs.getString("ssid", "");
    currentPASS = prefs.getString("pass", "");

    if (psramInit()) Serial.println("PSRAM OK");

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000); // 20MHz (Görüntü kalitesi ve guc tasarrufu icin en ideali)
    tft.setRotation(1);
    
    // Sync
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);

    touch.begin();
    renderDesktop();

    if (autoConnect && currentSSID != "") {
        WiFi.begin(currentSSID.c_str(), currentPASS.c_str());
    }
}

void loop() {
    esp_task_wdt_reset();

    // Animasyon sadece masaüstünde çalışsın
    if (currentPage == DESKTOP) {
        updateStarsHD(&tft, whiteTheme);
        delay(10);
    }

    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int tx = map(p.x, 200, 3800, 0, 320);
        int ty = map(p.y, 200, 3800, 0, 240);

        if (currentPage == DESKTOP) {
            if (tx < 50 && ty > 210) { // WIN TUŞU
                currentPage = START_MENU;
                tft.fillRect(0, 100, 100, 115, whiteTheme ? 0xFFFF : 0x2104);
                tft.drawRect(0, 100, 100, 115, 0x03FF);
                tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
                tft.setCursor(10, 120); tft.print("AYARLAR");
                tft.setCursor(10, 160); tft.print("KAPAT");
                delay(300);
            }
        } 
        else if (currentPage == START_MENU) {
            if (tx < 100 && ty > 110 && ty < 140) { // AYARLAR SEÇİLDİ
                currentPage = SETTINGS;
                renderSettings();
                delay(300);
            } else {
                currentPage = DESKTOP;
                renderDesktop();
                delay(300);
            }
        }
        else if (currentPage == SETTINGS) {
            if (tx < 150 && ty > 90 && ty < 130) { // TEMA DEGİSTİR
                whiteTheme = !whiteTheme;
                prefs.putBool("theme", whiteTheme);
                renderSettings();
                delay(300);
            }
            if (tx > 10 && ty > 180) { // KAYDET VE CIK
                currentPage = DESKTOP;
                renderDesktop();
                delay(300);
            }
        }
    }

    // SELECT TUŞU İLE ACİL RETRO-GO GEÇİŞİ
    if (digitalRead(JOY_SELECT) == LOW) {
        const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
        if (target) { esp_ota_set_boot_partition(target); delay(500); ESP.restart(); }
    }
}
