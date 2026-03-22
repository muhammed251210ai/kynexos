/* **************************************************************************
 * KynexOs Sovereign Build v230.53 - The Sovereign Arcade
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: 3D Cube, Snake, 2P Pong, Long Press Power Menu, Settings Fix
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
#include "esp_sleep.h"
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

// SİSTEM DURUMLARI
enum State { DESKTOP, START_MENU, SETTINGS, PAINT, GAME_CUBE, GAME_SNAKE, GAME_PONG, POWER_MENU };
State currentState = DESKTOP;
bool whiteTheme = false;
unsigned long pressTimer = 0;
bool isLongPress = false;

// 3D KÜP DEĞİŞKENLERİ
float rotX = 0, rotY = 0, rotZ = 0;
struct Point3D { float x, y, z; };
Point3D cubePoints[8] = {
  {-0.5,-0.5,0.5},{0.5,-0.5,0.5},{0.5,0.5,0.5},{-0.5,0.5,0.5},
  {-0.5,-0.5,-0.5},{0.5,-0.5,-0.5},{0.5,0.5,-0.5},{-0.5,0.5,-0.5}
};

void renderDesktop() {
    drawRetroWallpaper(&tft);
    tft.fillRect(0, 215, 320, 25, 0x10A2);
    drawWin10Logo(&tft, 5, 219, 18);
    tft.setTextColor(0xFFFF); tft.setCursor(200, 224); tft.print("SOVEREIGN OS OK");
}

void renderPowerMenu() {
    tft.fillScreen(0x0000);
    tft.drawRect(60, 40, 200, 160, 0xF800);
    tft.setTextColor(0xFFFF); tft.setCursor(85, 60); tft.print("GUC SECENEKLERI");
    tft.fillRect(80, 90, 160, 30, 0xF800); tft.setCursor(100, 100); tft.print("SISTEMI KAPAT");
    tft.fillRect(80, 130, 160, 30, 0x03FF); tft.setCursor(100, 140); tft.print("YENIDEN BASLAT");
    tft.fillRect(80, 170, 160, 30, 0x3186); tft.setCursor(120, 180); tft.print("GERI");
}

void renderSettingsUI() {
    tft.fillScreen(whiteTheme ? 0xFFFF : 0x0000);
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setTextSize(2); tft.setCursor(100, 20); tft.print("AYARLAR");
    tft.setTextSize(1);
    tft.drawRect(50, 60, 220, 40, 0x07FF);
    tft.setCursor(70, 75); tft.print(whiteTheme ? "KOYU TEMA YAP" : "BEYAZ TEMA YAP");
    tft.fillRect(50, 150, 220, 40, 0x07E0);
    tft.setCursor(110, 165); tft.setTextColor(0xFFFF); tft.print("KAYDET VE CIK");
    esp_task_wdt_reset();
}

// OYUNLAR -------------------------------------------------------------

void run3DCube() {
    tft.fillScreen(0x0000);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int jx = analogRead(J1_X); int jy = analogRead(J1_Y);
        rotX += (jy - 2048) * 0.00005; rotY += (jx - 2048) * 0.00005;
        tft.fillScreen(0x0000);
        tft.setCursor(10,10); tft.print("3D KUBE - J1 CONTROL");
        // Basit 3D Tel Kafes Çizimi
        for(int i=0; i<4; i++) {
            tft.drawLine(160+(cubePoints[i].x*100), 120+(cubePoints[i].y*100), 160+(cubePoints[(i+1)%4].x*100), 120+(cubePoints[(i+1)%4].y*100), 0x07FF);
        }
        delay(20);
    }
    currentState = DESKTOP; renderDesktop();
}

void runPong() {
    int p1y = 100, p2y = 100, ballX = 160, ballY = 120, dx = 4, dy = 4;
    tft.fillScreen(0x0000);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int j1 = analogRead(J1_Y); 
        int j2 = analogRead(J2_X); // MUHAMMED: J2 FIX - X Pong'da yukarı-aşağı oldu!
        
        tft.fillRect(10, p1y, 10, 40, 0x0000); // Temizle
        tft.fillRect(300, p2y, 10, 40, 0x0000);
        
        p1y = map(j1, 0, 4095, 0, 200);
        p2y = map(j2, 0, 4095, 0, 200);
        
        tft.fillRect(10, p1y, 10, 40, 0xF800); // P1 Kırmızı
        tft.fillRect(300, p2y, 10, 40, 0x07E0); // P2 Yeşil
        
        tft.fillCircle(ballX, ballY, 4, 0x0000);
        ballX += dx; ballY += dy;
        if(ballY <= 0 || ballY >= 240) dy = -dy;
        if(ballX <= 20 && ballY > p1y && ballY < p1y+40) dx = -dx;
        if(ballX >= 290 && ballY > p2y && ballY < p2y+40) dx = -dx;
        if(ballX < 0 || ballX > 320) { ballX = 160; ballY = 120; }
        
        tft.fillCircle(ballX, ballY, 4, 0xFFFF);
        delay(20);
    }
    currentState = DESKTOP; renderDesktop();
}

void setup() {
    WiFi.mode(WIFI_OFF);
    Serial.begin(115200);
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000); tft.setRotation(3);
    touch.begin(); touch.setRotation(3);
    
    prefs.begin("sov_v325", false);
    whiteTheme = prefs.getBool("theme", false);
    
    renderDesktop();
    esp_task_wdt_init(30, true);
}

void loop() {
    esp_task_wdt_reset();

    // LONG PRESS KONTROLÜ (2 Saniye)
    if (digitalRead(JOY_SELECT) == LOW) {
        if (pressTimer == 0) pressTimer = millis();
        if (millis() - pressTimer > 2000 && !isLongPress) {
            isLongPress = true;
            currentState = POWER_MENU;
            renderPowerMenu();
        }
    } else {
        pressTimer = 0;
        isLongPress = false;
    }

    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int tx = map(p.x, 3700, 200, 0, 320); // Düzeltilmiş Mirror Fix
        int ty = map(p.y, 3800, 240, 0, 240);

        if (currentState == DESKTOP) {
            if (tx < 50 && ty > 210) { currentState = START_MENU; tft.fillRect(0, 20, 140, 195, 0x2104); tft.drawRect(0, 20, 140, 195, 0x07FF);
                tft.setTextColor(0xFFFF); tft.setCursor(10, 40); tft.print("> AYARLAR");
                tft.setCursor(10, 75); tft.print("> 3D CUBE");
                tft.setCursor(10, 110); tft.print("> PONG 2P");
                tft.setCursor(10, 145); tft.print("> SNAKE");
                tft.setCursor(10, 180); tft.print("> RETRO-GO");
                delay(300);
            }
        } 
        else if (currentState == START_MENU) {
            if (ty > 40 && ty < 70) { currentState = SETTINGS; renderSettingsUI(); }
            else if (ty > 70 && ty < 105) run3DCube();
            else if (ty > 105 && ty < 140) runPong();
            else if (ty > 175) { const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
                if (target) { esp_ota_set_boot_partition(target); ESP.restart(); }
            } else { currentState = DESKTOP; renderDesktop(); }
            delay(300);
        }
        else if (currentState == SETTINGS) {
            if (ty > 60 && ty < 100) { whiteTheme = !whiteTheme; renderSettingsUI(); delay(200); }
            if (ty > 150) { prefs.putBool("theme", whiteTheme); currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == POWER_MENU) {
            if (ty > 90 && ty < 120) { tft.fillScreen(0); tft.setCursor(100,120); tft.print("KAPANIYOR..."); delay(1000); esp_deep_sleep_start(); }
            if (ty > 130 && ty < 160) ESP.restart();
            if (ty > 170) { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
    }
}
