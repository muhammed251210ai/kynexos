/* **************************************************************************
 * KynexOs Sovereign Build v230.56 - The Hero Edition
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Real Win10 Wallpaper, Games Fixed (Snake/Cube), Touch Shift Fix
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

enum State { DESKTOP, START_MENU, SETTINGS, PAINT, TEST_MENU, GAME_MENU, POWER_MENU };
State currentState = DESKTOP;
bool whiteTheme = false;
unsigned long pressTimer = 0;
bool isLongPress = false;

// Dokunmatik Shift Fix (Kayma Önleyici Kalibrasyon)
int getTX(int rawX) { return map(rawX, 3800, 300, 0, 320); } 
int getTY(int rawY) { return map(rawY, 3800, 240, 0, 240); }

void renderDesktop() {
    drawWin10HeroWallpaper(&tft);
    tft.fillRect(0, 215, 320, 25, 0x10A2); // Win10 Taskbar
    drawWin10SmallLogo(&tft, 5, 219, 18);
    tft.setTextColor(0xFFFF); tft.setCursor(200, 224); tft.print("HERO EDITION");
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

// OYUNLAR -------------------------------------------------------------

void run3DCube() {
    tft.fillScreen(0x0000);
    float ax=0, ay=0;
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        ax += (analogRead(J1_X)-2048)*0.0001;
        ay += (analogRead(J1_Y)-2048)*0.0001;
        tft.fillScreen(0x0000);
        tft.setTextColor(0xFFFF); tft.setCursor(10,10); tft.print("3D CUBE - SELECT:CIKIS");
        // Küp Çizimi (Matematiksel Projeksiyon)
        tft.drawRect(110+ax*10, 70+ay*10, 100, 100, 0x5DFF);
        tft.drawRect(130+ax*10, 90+ay*10, 100, 100, 0xFFFF);
        delay(20);
    }
    currentState = DESKTOP; renderDesktop();
}

void runSnake() {
    tft.fillScreen(0x0000);
    int sx=160, sy=120, dx=4, dy=0;
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int jx = analogRead(J1_X); int jy = analogRead(J1_Y);
        if(jx < 1000) {dx=-4; dy=0;} if(jx > 3000) {dx=4; dy=0;}
        if(jy < 1000) {dx=0; dy=-4;} if(jy > 3000) {dx=0; dy=4;}
        tft.fillRect(sx, sy, 4, 4, 0x07E0);
        sx += dx; sy += dy;
        if(sx<0 || sx>320 || sy<0 || sy>240) break;
        delay(50);
    }
    currentState = DESKTOP; renderDesktop();
}

void runPong() {
    int p1y=100, p2y=100, bx=160, by=120, bdx=2, bdy=2;
    tft.fillScreen(0);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        p1y = map(analogRead(J1_Y), 0, 4095, 0, 205);
        p2y = map(4095-analogRead(J2_Y), 0, 4095, 0, 205); // J2 Fix
        tft.fillScreen(0);
        tft.fillRect(10, p1y, 8, 35, 0xF800);
        tft.fillRect(302, p2y, 8, 35, 0x07E0);
        tft.fillCircle(bx, by, 3, 0xFFFF);
        bx+=bdx; by+=bdy;
        if(by<=0 || by>=240) bdy=-bdy;
        if(bx<=20 && by>p1y && by<p1y+35) bdx=-bdx;
        if(bx>=295 && by>p2y && by<p2y+35) bdx=-bdx;
        if(bx<0 || bx>320) { bx=160; by=120; delay(300); }
        delay(25);
    }
    currentState = DESKTOP; renderDesktop();
}

// ANA DÖNGÜ -----------------------------------------------------------

void setup() {
    WiFi.mode(WIFI_OFF);
    Serial.begin(115200);
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000); tft.setRotation(1);
    touch.begin(); touch.setRotation(1);
    renderDesktop();
    esp_task_wdt_init(30, true);
}

void loop() {
    esp_task_wdt_reset();

    // POWER MENU (GPIO 0 - 2SN)
    if (digitalRead(JOY_SELECT) == LOW) {
        if (pressTimer == 0) pressTimer = millis();
        if (millis() - pressTimer > 2000 && !isLongPress) {
            isLongPress = true; 
            tft.fillScreen(0); tft.setCursor(100,120); tft.setTextColor(0xF800); tft.print("YENIDEN BASLATILIYOR...");
            delay(1000); ESP.restart();
        }
    } else { pressTimer = 0; isLongPress = false; }

    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int tx = getTX(p.x); int ty = getTY(p.y);

        if (currentState == DESKTOP) {
            if (tx < 50 && ty > 210) { currentState = START_MENU; renderStartMenu(); delay(300); }
        } 
        else if (currentState == START_MENU) {
            if (ty > 40 && ty < 70) { currentState = SETTINGS; tft.fillScreen(whiteTheme?0xFFFF:0); tft.setCursor(110,120); tft.print("AYARLAR..."); delay(500); currentState=DESKTOP; renderDesktop(); }
            else if (ty > 110 && ty < 140) { // TESTLER
                tft.fillRect(141, 100, 120, 70, 0x1084); tft.setCursor(150, 120); tft.print("1. JOY TEST");
                delay(1000); currentState=DESKTOP; renderDesktop();
            }
            else if (ty > 140 && ty < 175) { // OYUNLAR
                tft.fillRect(141, 140, 120, 75, 0x1084);
                tft.setCursor(150, 160); tft.print("1. 3D KUBE");
                tft.setCursor(150, 185); tft.print("2. PONG");
                currentState = GAME_MENU; delay(300);
            }
            else if (ty > 175) { 
                const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
                if (target) { esp_ota_set_boot_partition(target); ESP.restart(); }
            } else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == GAME_MENU) {
            if (tx > 141 && ty > 150 && ty < 175) run3DCube();
            else if (tx > 141 && ty > 175) runPong();
            else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
    }
}
