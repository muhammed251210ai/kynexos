/* **************************************************************************
 * KynexOs Sovereign Build v230.55 - The Absolute Axis
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Absolute Touch Mapping, J2 Inversion Fix, Games/Tests Nested Menus
 * Donanım: ESP32-S3 N16R8 (V325 Pinout - Screen Rotation 1)
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

// DONANIM PİNLERİ
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

// SİSTEM DURUMLARI
enum State { DESKTOP, START_MENU, SETTINGS, PAINT, TEST_MENU, GAME_MENU, POWER_MENU };
State currentState = DESKTOP;
bool whiteTheme = false;
unsigned long pressTimer = 0;
bool isLongPress = false;
uint16_t paintColor = 0xF800;

// Dokunmatik Koordinat Düzeltme Fonksiyonu
int getTX(int rawX) { return map(rawX, 3700, 200, 0, 320); } // MUHAMMED: Mirror Fix Uygulandı
int getTY(int rawY) { return map(rawY, 3800, 240, 0, 240); } // MUHAMMED: Mirror Fix Uygulandı

void renderDesktop() {
    drawRetroWallpaper(&tft);
    tft.fillRect(0, 215, 320, 25, 0x10A2);
    drawWin10Logo(&tft, 5, 219, 18);
    tft.setTextColor(0xFFFF); tft.setTextSize(1); tft.setCursor(200, 224); tft.print("ABSOLUTE AXIS OK");
}

void renderStartMenu() {
    tft.fillRect(0, 20, 140, 195, 0x2104);
    tft.drawRect(0, 20, 140, 195, 0x07FF);
    tft.setTextColor(0xFFFF); tft.setCursor(10, 40);  tft.print("> AYARLAR");
    tft.setCursor(10, 75);  tft.print("> PAINT");
    tft.setCursor(10, 110); tft.print("> TESTLER");
    tft.setCursor(10, 145); tft.print("> OYUNLAR");
    tft.setCursor(10, 180); tft.print("> RETRO-GO");
}

void renderGameMenu() {
    tft.fillRect(141, 110, 130, 105, 0x1084);
    tft.drawRect(141, 110, 130, 105, 0xF81F);
    tft.setTextColor(0xFFFF);
    tft.setCursor(150, 130); tft.print("1. 3D KUBE");
    tft.setCursor(150, 160); tft.print("2. YILAN");
    tft.setCursor(150, 190); tft.print("3. PONG 2P");
}

void renderTestMenu() {
    tft.fillRect(141, 75, 130, 70, 0x1084);
    tft.drawRect(141, 75, 130, 70, 0x07FF);
    tft.setTextColor(0xFFFF);
    tft.setCursor(150, 95); tft.print("1. DOKUNMA");
    tft.setCursor(150, 125); tft.print("2. JOYSTICK");
}

// UYGULAMALAR & OYUNLAR -----------------------------------------------

void runPaintApp() {
    tft.fillScreen(0xFFFF);
    tft.fillRect(280, 0, 40, 240, 0xC618);
    tft.fillRect(285, 210, 30, 25, 0xF81F); // Çıkış butonu
    while(true) {
        esp_task_wdt_reset();
        if (touch.touched()) {
            TS_Point p = touch.getPoint();
            int tx = getTX(p.x); int ty = getTY(p.y);
            if (tx > 280 && ty > 210) break;
            if (tx < 280) tft.fillCircle(tx, ty, 2, paintColor);
        }
        if (digitalRead(JOY_SELECT) == LOW) break;
    }
    currentState = DESKTOP; renderDesktop();
}

void runPong() {
    int p1y = 100, p2y = 100, ballX = 160, ballY = 120, dx = 2, dy = 2; // Hız optimize edildi
    tft.fillScreen(0x0000);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int j1y = analogRead(J1_Y); 
        int j2raw = analogRead(J2_Y); // MUHAMMED: J2 Eksen Fix
        int j2y = 4095 - j2raw;
        
        tft.fillRect(10, p1y, 8, 35, 0x0000); 
        tft.fillRect(302, p2y, 8, 35, 0x0000);
        
        p1y = map(j1y, 0, 4095, 0, 205);
        p2y = map(j2y, 0, 4095, 0, 205);
        
        tft.fillRect(10, p1y, 8, 35, 0xF800);
        tft.fillRect(302, p2y, 8, 35, 0x07E0);
        
        tft.fillCircle(ballX, ballY, 3, 0x0000);
        ballX += dx; ballY += dy;
        if(ballY <= 0 || ballY >= 240) dy = -dy;
        if(ballX <= 20 && ballY > p1y && ballY < p1y+35) dx = -dx;
        if(ballX >= 295 && ballY > p2y && ballY < p2y+35) dx = -dx;
        if(ballX < 0 || ballX > 320) { ballX = 160; ballY = 120; delay(400); }
        
        tft.fillCircle(ballX, ballY, 3, 0xFFFF);
        delay(30); 
    }
    currentState = DESKTOP; renderDesktop();
}

void setup() {
    WiFi.mode(WIFI_OFF);
    Serial.begin(115200);
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000); tft.setRotation(1); // Standart Yatay Mod
    touch.begin(); touch.setRotation(1);
    
    prefs.begin("sov_v325", false);
    whiteTheme = prefs.getBool("theme", false);
    
    renderDesktop();
    esp_task_wdt_init(30, true);
}

void loop() {
    esp_task_wdt_reset();

    // GÜÇ MENÜSÜ (GPIO 0 UZUN BASIŞ)
    if (digitalRead(JOY_SELECT) == LOW) {
        if (pressTimer == 0) pressTimer = millis();
        if (millis() - pressTimer > 2000 && !isLongPress) {
            isLongPress = true; currentState = POWER_MENU;
            tft.fillScreen(0); tft.setTextColor(0xF800); tft.setCursor(100,100); tft.print("GUC MENUSU");
            delay(1000); currentState = DESKTOP; renderDesktop();
        }
    } else { pressTimer = 0; isLongPress = false; }

    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int tx = getTX(p.x); int ty = getTY(p.y);

        if (currentState == DESKTOP) {
            if (tx < 50 && ty > 210) { currentState = START_MENU; renderStartMenu(); delay(300); }
        } 
        else if (currentState == START_MENU) {
            if (ty > 40 && ty < 70) { tft.fillScreen(whiteTheme ? 0xFFFF : 0x0000); tft.print("AYARLAR..."); delay(500); currentState=DESKTOP; renderDesktop(); }
            else if (ty > 70 && ty < 105) runPaintApp();
            else if (ty > 105 && ty < 140) { renderTestMenu(); currentState = TEST_MENU; delay(300); }
            else if (ty > 140 && ty < 175) { renderGameMenu(); currentState = GAME_MENU; delay(300); }
            else if (ty > 175) { 
                const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
                if (target) { esp_ota_set_boot_partition(target); ESP.restart(); }
            } else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == GAME_MENU) {
            if (tx > 141 && ty > 170) runPong();
            else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == TEST_MENU) {
            if (tx > 141 && ty > 80 && ty < 115) { // DOKUNMA TESTİ
                tft.fillScreen(0xFFFF); tft.setTextColor(0); tft.setCursor(10,10); tft.print("DOKUNMA TEST (SELECT: CIKIS)");
                while(digitalRead(JOY_SELECT)==HIGH) {
                    if(touch.touched()) { TS_Point tp = touch.getPoint(); tft.drawCircle(getTX(tp.x), getTY(tp.y), 10, 0x07FF); }
                    esp_task_wdt_reset();
                }
                currentState = DESKTOP; renderDesktop();
            }
            else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
    }
}
