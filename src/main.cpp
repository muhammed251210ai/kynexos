/* **************************************************************************
 * KynexOs Sovereign Build v230.51 - The Full Control
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Paint App, Settings Fix, Screen Inversion Fix, J2 Rotation Fix
 * Donanım: ESP32-S3 N16R8 (Retro-Go V325 Pinout)
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

// PIN TANIMLARI
#define TFT_SCK 12
#define TFT_MOSI 11
#define TFT_MISO 13
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 14
#define TFT_BL 1
#define TOUCH_CS 16
#define JOY_SELECT 0 // GPIO 0

// JOYSTICK PİNLERİ
#define J1_X 5
#define J1_Y 4
#define J2_X 7
#define J2_Y 15

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS);
Preferences prefs;

enum State { DESKTOP, START_MENU, SETTINGS, TOUCH_TEST, JOY_TEST, PAINT };
State currentState = DESKTOP;
uint16_t paintColor = 0xF800; // Varsayılan Kırmızı
bool whiteTheme = false;

void renderDesktop() {
    drawRetroWallpaper(&tft);
    tft.fillRect(0, 215, 320, 25, 0x10A2);
    drawWin10Logo(&tft, 5, 219, 18);
    tft.setTextColor(0xFFFF); tft.setCursor(220, 224); tft.print("V230.51 OK");
}

void renderStartMenu() {
    tft.fillRect(0, 40, 140, 175, 0x2104);
    tft.drawRect(0, 40, 140, 175, 0x07FF);
    tft.setTextColor(0xFFFF); tft.setTextSize(1);
    tft.setCursor(10, 60);  tft.print("> AYARLAR");
    tft.setCursor(10, 95);  tft.print("> PAINT");
    tft.setCursor(10, 130); tft.print("> DOKUNMA TEST");
    tft.setCursor(10, 165); tft.print("> JOYSTICK TEST");
    tft.setCursor(10, 200); tft.print("> RETRO-GO");
}

void runPaintApp() {
    tft.fillScreen(0xFFFF);
    // Renk Paleti (Sağda)
    tft.fillRect(280, 0, 40, 240, 0xC618);
    tft.fillRect(285, 10, 30, 30, 0xF800); // Kırmızı
    tft.fillRect(285, 50, 30, 30, 0x07E0); // Yeşil
    tft.fillRect(285, 90, 30, 30, 0x001F); // Mavi
    tft.fillRect(285, 130, 30, 30, 0xFFE0); // Sarı
    tft.fillRect(285, 170, 30, 30, 0x0000); // Silgi
    tft.fillRect(285, 210, 30, 25, 0xF81F); // ÇIKIŞ
    
    while(true) {
        esp_task_wdt_reset();
        if (touch.touched()) {
            TS_Point p = touch.getPoint();
            int tx = map(p.x, 3700, 200, 0, 320); // Düzeltilmiş X
            int ty = map(p.y, 3800, 240, 0, 240); // Düzeltilmiş Y
            
            if (tx > 280) {
                if (ty > 10 && ty < 40) paintColor = 0xF800;
                else if (ty > 50 && ty < 80) paintColor = 0x07E0;
                else if (ty > 90 && ty < 120) paintColor = 0x001F;
                else if (ty > 130 && ty < 160) paintColor = 0xFFE0;
                else if (ty > 170 && ty < 200) paintColor = 0xFFFF; // Silgi
                else if (ty > 210) break; // Çıkış
                delay(100);
            } else {
                tft.fillCircle(tx, ty, 2, paintColor);
            }
        }
        if (digitalRead(JOY_SELECT) == LOW) break;
    }
    currentState = DESKTOP; renderDesktop();
}

void runJoystickTest() {
    tft.fillScreen(0x0000);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int v1x = 4095 - analogRead(J1_X); 
        int v1y = analogRead(J1_Y);

        // MUHAMMED: J2 ROTASYON FIX (Up->Right, Right->Down Düzeltildi)
        int raw2x = analogRead(J2_X);
        int raw2y = analogRead(J2_Y);
        int v2x = raw2y; // Y ekseni X'e
        int v2y = 4095 - raw2x; // X ekseni ters çevrilip Y'ye

        tft.fillRect(20, 50, 130, 100, 0x1084);
        tft.setCursor(25, 60); tft.setTextColor(0xFFFF);
        tft.printf("SOL JOY\nX:%d Y:%d", v1x, v1y);
        tft.fillCircle(20 + map(v1x, 0, 4095, 5, 125), 50 + map(v1y, 0, 4095, 5, 95), 3, 0xF800);

        tft.fillRect(170, 50, 130, 100, 0x1084);
        tft.setCursor(175, 60);
        tft.printf("SAG JOY\nX:%d Y:%d", v2x, v2y);
        tft.fillCircle(170 + map(v2x, 0, 4095, 5, 125), 50 + map(v2y, 0, 4095, 5, 95), 3, 0x07E0);
        delay(30);
    }
    currentState = DESKTOP; renderDesktop();
}

void renderSettingsUI() {
    tft.fillScreen(whiteTheme ? 0xFFFF : 0x0000);
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setCursor(100, 20); tft.setTextSize(2); tft.print("AYARLAR");
    tft.setTextSize(1);
    tft.drawRect(50, 60, 220, 40, 0x07FF);
    tft.setCursor(70, 75); tft.print(whiteTheme ? "KOYU TEMA YAP" : "BEYAZ TEMA YAP");
    tft.fillRect(50, 120, 220, 40, 0xF800);
    tft.setCursor(120, 135); tft.setTextColor(0xFFFF); tft.print("GERI DON");
}

void setup() {
    WiFi.mode(WIFI_OFF);
    Serial.begin(115200);
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000); tft.setRotation(3); // EKRAN TERSLİĞİ İÇİN 3 YAPILDI
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);
    touch.begin(); touch.setRotation(3); 
    renderDesktop();
    esp_task_wdt_init(30, true);
}

void loop() {
    esp_task_wdt_reset();
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int tx = map(p.x, 3700, 200, 0, 320); 
        int ty = map(p.y, 3800, 240, 0, 240);

        if (currentState == DESKTOP) {
            if (tx < 50 && ty > 210) { 
                currentState = START_MENU; renderStartMenu(); delay(300); 
            }
        } 
        else if (currentState == START_MENU) {
            if (ty > 60 && ty < 90) { currentState = SETTINGS; renderSettingsUI(); }
            else if (ty > 90 && ty < 125) runPaintApp();
            else if (ty > 125 && ty < 160) {
                tft.fillScreen(0xFFFF); tft.setCursor(10, 10); tft.setTextColor(0x0000);
                tft.print("DOKUNMA TEST (SELECT:CIKIS)");
                while(digitalRead(JOY_SELECT)==HIGH) {
                    if(touch.touched()){
                        TS_Point tp = touch.getPoint();
                        int txx = map(tp.x, 3700, 200, 0, 320);
                        int tyy = map(tp.y, 3800, 240, 0, 240);
                        tft.drawCircle(txx, tyy, 10, 0x07FF); // GEÇİCİ DAİRELER BURADA
                    }
                    esp_task_wdt_reset();
                }
                currentState = DESKTOP; renderDesktop();
            }
            else if (ty > 160 && ty < 195) runJoystickTest();
            else { currentState = DESKTOP; renderDesktop(); }
            delay(300);
        }
        else if (currentState == SETTINGS) {
            if (ty > 60 && ty < 100) { whiteTheme = !whiteTheme; renderSettingsUI(); delay(300); }
            if (ty > 120 && ty < 160) { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
    }
    if (currentState == DESKTOP && digitalRead(JOY_SELECT) == LOW) {
        const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
        if (target) { esp_ota_set_boot_partition(target); delay(500); ESP.restart(); }
    }
}
