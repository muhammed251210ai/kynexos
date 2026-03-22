/* **************************************************************************
 * KynexOs Sovereign Build v230.49 - The Diagnostic Sovereign
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Win10 Start Menu, Dual Joystick Test, Touch Paint Test, Settings
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
#define TFT_SCK    12
#define TFT_MOSI   11
#define TFT_MISO   13
#define TFT_CS     10
#define TFT_DC     9
#define TFT_RST    14
#define TFT_BL     1
#define TOUCH_CS   16
#define JOY_SELECT 6

// JOYSTICK ANALOG PİNLERİ (V325 ADC MAP)
#define J1_X 5  // ADC1_CH4
#define J1_Y 4  // ADC1_CH3
#define J2_X 7  // ADC1_CH6
#define J2_Y 15 // ADC2_CH4

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS);
Preferences prefs;

enum SystemState { DESKTOP, START_MENU, SETTINGS, TOUCH_TEST, JOY_TEST };
SystemState currentState = DESKTOP;

void renderDesktop() {
    drawRetroWallpaper(&tft);
    tft.fillRect(0, 215, 320, 25, 0x10A2); // Taskbar
    drawWin10Logo(&tft, 5, 219, 18);       // Win Icon
    tft.setTextColor(0xFFFF); tft.setTextSize(1);
    tft.setCursor(260, 224); tft.print("SOVEREIGN");
}

void renderStartMenu() {
    tft.fillRect(0, 60, 140, 155, 0x2104);
    tft.drawRect(0, 60, 140, 155, RETRO_CYAN);
    tft.setTextColor(0xFFFF); tft.setTextSize(1);
    tft.setCursor(10, 80);  tft.print("> AYARLAR");
    tft.setCursor(10, 115); tft.print("> DOKUNMA TESTI");
    tft.setCursor(10, 150); tft.print("> JOYSTICK TESTI");
    tft.setCursor(10, 185); tft.print("> KAPAT");
}

void runJoystickTest() {
    tft.fillScreen(0x0000);
    tft.setTextColor(0x07FF); tft.setTextSize(2);
    tft.setCursor(45, 10); tft.print("JOYSTICK TESTI");
    tft.setTextSize(1);
    tft.setCursor(10, 225); tft.print("SELECT TUSU: CIKIS");

    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int v1x = analogRead(J1_X); int v1y = analogRead(J1_Y);
        int v2x = analogRead(J2_X); int v2y = analogRead(J2_Y);

        // Joystick 1 Görselleştirme
        tft.fillRect(20, 50, 130, 80, 0x1084);
        tft.drawRect(20, 50, 130, 80, 0xFFFF);
        tft.setCursor(25, 60); tft.setTextColor(0xFFFF);
        tft.printf("JOY 1 (SOL)\nX: %d\nY: %d", v1x, v1y);
        tft.fillCircle(20 + map(v1x, 0, 4095, 10, 120), 50 + map(v1y, 0, 4095, 10, 70), 5, 0xF800);

        // Joystick 2 Görselleştirme
        tft.fillRect(170, 50, 130, 80, 0x1084);
        tft.drawRect(170, 50, 130, 80, 0xFFFF);
        tft.setCursor(175, 60);
        tft.printf("JOY 2 (SAG)\nX: %d\nY: %d", v2x, v2y);
        tft.fillCircle(170 + map(v2x, 0, 4095, 10, 120), 50 + map(v2y, 0, 4095, 10, 70), 5, 0x07E0);
        
        delay(50);
    }
    currentState = DESKTOP; renderDesktop();
}

void runTouchTest() {
    tft.fillScreen(0xFFFF);
    tft.setTextColor(0x0000); tft.setCursor(10, 10);
    tft.print("DOKUNMA TESTI (CIZIM YAPIN)");
    tft.setCursor(10, 225); tft.print("SELECT TUSU: CIKIS");
    
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        if (touch.touched()) {
            TS_Point p = touch.getPoint();
            int tx = map(p.x, 200, 3700, 0, 320);
            int ty = map(p.y, 240, 3800, 0, 240);
            tft.fillCircle(tx, ty, 2, 0xF800);
        }
    }
    currentState = DESKTOP; renderDesktop();
}

void setup() {
    WiFi.mode(WIFI_OFF);
    Serial.begin(115200);
    
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000); tft.setRotation(1);
    
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);

    touch.begin(); touch.setRotation(1);
    renderDesktop();
    esp_task_wdt_init(30, true);
}

void loop() {
    esp_task_wdt_reset();

    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int tx = map(p.x, 200, 3700, 0, 320);
        int ty = map(p.y, 240, 3800, 0, 240);

        if (currentState == DESKTOP) {
            if (tx < 50 && ty > 210) { // WIN Butonu
                currentState = START_MENU;
                renderStartMenu();
                delay(300);
            }
        } 
        else if (currentState == START_MENU) {
            if (tx < 140 && ty > 70 && ty < 100) { // Ayarlar
                tft.fillScreen(0x0000); tft.setCursor(100, 120); tft.print("AYARLAR YUKLENIYOR...");
                delay(1000); currentState = DESKTOP; renderDesktop();
            }
            else if (tx < 140 && ty > 105 && ty < 135) { // Dokunma Testi
                currentState = TOUCH_TEST;
                runTouchTest();
            }
            else if (tx < 140 && ty > 140 && ty < 170) { // Joystick Testi
                currentState = JOY_TEST;
                runJoystickTest();
            }
            else { // Menü dışı tıklama
                currentState = DESKTOP;
                renderDesktop();
                delay(300);
            }
        }
    }

    // SELECT TUSU ILE RETRO-GO (Masaüstündeyken aktif)
    if (currentState == DESKTOP && digitalRead(JOY_SELECT) == LOW) {
        const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
        if (target) { esp_ota_set_boot_partition(target); delay(500); ESP.restart(); }
    }
}
