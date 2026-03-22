/* **************************************************************************
 * KynexOs Sovereign Build v230.41 - The Sovereign Registry
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Settings UI, TR Keyboard, WiFi Manager, Theme Engine, Preferences
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

// V325 DONANIM HARİTASI
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
enum SystemState { DESKTOP, START_MENU, SETTINGS_WIFI };
SystemState currentState = DESKTOP;

bool whiteTheme = false;
bool autoConnect = false;
String wifiSSID = "";
String wifiPASS = "";

unsigned long lastAnim = 0;

void drawDesktop() {
    drawHeroBackground(&tft, whiteTheme);
    // Taskbar
    tft.fillRect(0, 215, 320, 25, whiteTheme ? 0xC618 : 0x10A2);
    // Win Tuşu (Başlat)
    tft.fillRect(2, 217, 25, 21, 0x03FF); 
    tft.drawRect(2, 217, 25, 21, 0xFFFF);
    tft.setCursor(4, 224); tft.setTextColor(0xFFFF); tft.print("WIN");
    
    // Masaüstü İkonu
    tft.fillRect(20, 20, 45, 45, 0x3186);
    tft.drawRect(20, 20, 45, 45, 0xFFFF);
    tft.setCursor(15, 70); tft.print("RETRO-GO");
}

void drawStartMenu() {
    tft.fillRect(0, 80, 120, 135, whiteTheme ? 0xFFFF : 0x2104);
    tft.drawRect(0, 80, 120, 135, 0x03FF);
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setCursor(10, 100); tft.print("> AYARLAR");
    tft.setCursor(10, 140); tft.print("> DOSYALAR");
    tft.setCursor(10, 180); tft.print("> KAPAT");
}

void drawWifiSettings() {
    tft.fillScreen(whiteTheme ? 0xFFFF : 0x0000);
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setTextSize(2);
    tft.setCursor(10, 10); tft.print("WiFi Ayarlari");
    
    tft.setTextSize(1);
    tft.drawRect(10, 40, 300, 30, 0x03FF); // SSID Kutusu
    tft.setCursor(15, 50); tft.print("SSID: " + wifiSSID);
    
    tft.drawRect(10, 80, 300, 30, 0x03FF); // PASS Kutusu
    tft.setCursor(15, 90); tft.print("Sifre: ****");
    
    // Seçenekler
    tft.fillRect(10, 130, 140, 30, 0x03FF);
    tft.setCursor(20, 140); tft.setTextColor(0xFFFF); tft.print(autoConnect ? "OTO: ACIK" : "OTO: KAPALI");
    
    tft.fillRect(170, 130, 140, 30, 0xF800);
    tft.setCursor(180, 140); tft.print("AGI UNUT");
    
    tft.fillRect(10, 180, 300, 40, 0x07E0);
    tft.setCursor(110, 195); tft.print("KAYDET VE BAGLAN");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    prefs.begin("sov_sys", false);
    whiteTheme = prefs.getBool("theme", false);
    autoConnect = prefs.getBool("auto_wifi", false);
    wifiSSID = prefs.getString("ssid", "");
    wifiPASS = prefs.getString("pass", "");

    if (psramInit()) Serial.println("PSRAM OK");

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(40000000); tft.setRotation(1);
    touch.begin();

    initSovereignWallpaper();
    drawDesktop();

    if (autoConnect && wifiSSID != "") {
        WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
        Serial.println("Otomatik baglanti baslatildi.");
    }
}

void loop() {
    esp_task_wdt_reset();

    if (currentState == DESKTOP && millis() - lastAnim > 50) {
        updateStars(&tft, whiteTheme);
        lastAnim = millis();
    }

    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        // Basitleştirilmiş koordinat eşleme (320x240)
        int tx = map(p.x, 200, 3800, 0, 320);
        int ty = map(p.y, 200, 3800, 0, 240);

        if (currentState == DESKTOP) {
            // WIN TUŞU
            if (tx < 40 && ty > 210) {
                currentState = START_MENU;
                drawStartMenu();
                delay(300);
            }
        } 
        else if (currentState == START_MENU) {
            if (tx < 120 && ty > 90 && ty < 120) {
                currentState = SETTINGS_WIFI;
                drawWifiSettings();
                delay(300);
            } else {
                currentState = DESKTOP;
                drawDesktop();
                delay(300);
            }
        }
        else if (currentState == SETTINGS_WIFI) {
            // Kaydet ve Bağlan
            if (ty > 180) {
                prefs.putBool("auto_wifi", autoConnect);
                prefs.putString("ssid", wifiSSID);
                prefs.putString("pass", wifiPASS);
                WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
                currentState = DESKTOP;
                drawDesktop();
            }
            // Tema Değiştirme (Gizli alan: Başlık civarı)
            if (ty < 30) {
                whiteTheme = !whiteTheme;
                prefs.putBool("theme", whiteTheme);
                drawWifiSettings();
                delay(300);
            }
        }
    }

    // Retro-Go Atlaması (Select Tuşu)
    if (digitalRead(JOY_SELECT) == LOW) {
        const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
        if (target) { esp_ota_set_boot_partition(target); delay(500); ESP.restart(); }
    }
}
