/* **************************************************************************
 * KynexOs Sovereign Build v230.50 - The Precision Core
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Dual Analog Calibration, Touch Visualizer, GPIO 0 Select
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

// DONANIM HARİTASI
#define TFT_SCK    12
#define TFT_MOSI   11
#define TFT_MISO   13
#define TFT_CS     10
#define TFT_DC     9
#define TFT_RST    14
#define TFT_BL     1
#define TOUCH_CS   16
#define JOY_SELECT 0  // MUHAMMED: Select tusu artik GPIO 0

// JOYSTICK ANALOG PİNLERİ (V325 ADC MAP)
#define J1_X 5  
#define J1_Y 4  
#define J2_X 7  
#define J2_Y 15 

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS);

enum SystemState { DESKTOP, START_MENU, JOY_TEST };
SystemState currentState = DESKTOP;

// Dokunmatik Efekti İçin Son Koordinatlar
int lastTX = -1, lastTY = -1;

void renderDesktop() {
    drawRetroWallpaper(&tft);
    tft.fillRect(0, 215, 320, 25, 0x10A2); 
    drawWin10Logo(&tft, 5, 219, 18);       
    tft.setTextColor(0xFFFF); tft.setTextSize(1);
    tft.setCursor(220, 224); tft.print("PRECISION CORE");
}

void renderStartMenu() {
    tft.fillRect(0, 100, 140, 115, 0x2104);
    tft.drawRect(0, 100, 140, 115, RETRO_CYAN);
    tft.setTextColor(0xFFFF); tft.setTextSize(1);
    tft.setCursor(10, 120); tft.print("> AYARLAR");
    tft.setCursor(10, 155); tft.print("> JOYSTICK TEST");
    tft.setCursor(10, 190); tft.print("> RETRO-GO");
}

void runJoystickTest() {
    tft.fillScreen(0x0000);
    tft.setTextColor(0xFFFF);
    tft.setCursor(80, 10); tft.print("HASSAS JOYSTICK TESTI");

    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        
        // MUHAMMED: J1 SAĞ-SOL TERS ÇEVİRİLDİ (4095 - X)
        int v1x = 4095 - analogRead(J1_X); 
        int v1y = analogRead(J1_Y);

        // MUHAMMED: J2 DİAGONAL FIX (Sağ yukarı, aşağı sağ vb. karmaşasını çözen haritalama)
        int raw2x = analogRead(J2_X);
        int raw2y = analogRead(J2_Y);
        // Senin tarifine göre eksenleri normalize ediyoruz:
        int v2x = map(raw2x, 0, 4095, 4095, 0); 
        int v2y = map(raw2y, 0, 4095, 4095, 0);

        // Görsel Panel 1
        tft.fillRect(20, 50, 130, 100, 0x1084);
        tft.drawRect(20, 50, 130, 100, 0xFFFF);
        tft.setCursor(25, 60); tft.printf("SOL JOY\nX:%d Y:%d", v1x, v1y);
        tft.fillCircle(20 + map(v1x, 0, 4095, 5, 125), 50 + map(v1y, 0, 4095, 5, 95), 4, 0xF800);

        // Görsel Panel 2 (Remapped)
        tft.fillRect(170, 50, 130, 100, 0x1084);
        tft.drawRect(170, 50, 130, 100, 0xFFFF);
        tft.setCursor(175, 60); tft.printf("SAG JOY\nX:%d Y:%d", v2x, v2y);
        tft.fillCircle(170 + map(v2x, 0, 4095, 5, 125), 50 + map(v2y, 0, 4095, 5, 95), 4, 0x07E0);
        
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
    tft.begin(20000000); tft.setRotation(1);
    
    tft.sendCommand(0x36, (const uint8_t*)"\x28", 1);
    tft.sendCommand(0xB1, (const uint8_t*)"\x00\x1B", 2);
    tft.sendCommand(0xB6, (const uint8_t*)"\x08\x82\x27", 3);

    touch.begin(); touch.setRotation(1);
    renderDesktop();
    esp_task_wdt_init(30, true);
    if (psramInit()) Serial.println("OPI PSRAM Ready");
}

void loop() {
    esp_task_wdt_reset();

    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int tx = map(p.x, 200, 3700, 0, 320);
        int ty = map(p.y, 240, 3800, 0, 240);

        // MUHAMMED: Dokunulan yere geçici daire çiz
        tft.drawCircle(tx, ty, 10, 0xFFFF); 
        tft.drawCircle(tx, ty, 11, 0x03FF);

        if (currentState == DESKTOP) {
            if (tx < 50 && ty > 210) { // WIN Butonu
                currentState = START_MENU;
                renderStartMenu();
                delay(300);
            }
            // Masaüstü simgesine tıklandı mı?
            if (tx > 20 && tx < 65 && ty > 20 && ty < 65) {
                tft.drawRoundRect(18, 18, 49, 49, 3, 0x07FF); // Basılma efekti
                delay(100);
                Serial.println("[ACTION] Retro-Go Launching...");
            }
        } 
        else if (currentState == START_MENU) {
            if (tx < 140 && ty > 145 && ty < 180) { // Joystick Test
                currentState = JOY_TEST;
                runJoystickTest();
            } else if (tx < 140 && ty > 180) { // Kapat/Retro-Go
                const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
                if (target) { esp_ota_set_boot_partition(target); ESP.restart(); }
            } else {
                currentState = DESKTOP; renderDesktop();
                delay(300);
            }
        }
    }

    // SELECT (GPIO 0) ILE RETRO-GO (Masaüstündeyken)
    if (currentState == DESKTOP && digitalRead(JOY_SELECT) == LOW) {
        const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
        if (target) { esp_ota_set_boot_partition(target); delay(1000); ESP.restart(); }
    }
}
