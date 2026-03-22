/* **************************************************************************
 * KynexOs Sovereign Build v230.58 - The Ultimate Masterpiece
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: ALL IN ONE! (System Info, 3 Games, Paint, Tests, Power Menu)
 * Donanım: ESP32-S3 N16R8 (V325 Pinout - Absolute Calibration)
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

// DONANIM PIN HARITASI
#define TFT_SCK 12
#define TFT_MOSI 11
#define TFT_MISO 13
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 14
#define TFT_BL 1
#define TOUCH_CS 16
#define JOY_SELECT 0 // GPIO 0

// JOYSTICK ANALOG PINLERI
#define J1_X 5
#define J1_Y 4
#define J2_X 7
#define J2_Y 15

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS);
Preferences prefs;

// SISTEM DURUMLARI
enum State { DESKTOP, START_MENU, SETTINGS, SYS_INFO, PAINT, TEST_MENU, GAME_MENU, POWER_MENU };
State currentState = DESKTOP;
bool whiteTheme = false;
unsigned long pressTimer = 0;
bool isLongPress = false;
uint16_t paintColor = 0xF800;

// MUHAMMED: Mutlak Dokunmatik Kalibrasyonu
int getTX(int rawX) { return map(rawX, 3850, 250, 0, 320); } 
int getTY(int rawY) { return map(rawY, 3800, 240, 0, 240); }

void renderDesktop() {
    drawWin10HeroWallpaper(&tft, whiteTheme); 
    tft.fillRect(0, 215, 320, 25, whiteTheme ? 0xAD75 : 0x10A2); // Taskbar
    drawWin10SmallLogo(&tft, 5, 219, 18);
    tft.setTextColor(0xFFFF); tft.setCursor(180, 224); tft.print("SOVEREIGN ULTIMATE");
}

void renderStartMenu() {
    tft.fillRect(0, 15, 150, 200, whiteTheme ? 0xFFFF : 0x2104);
    tft.drawRect(0, 15, 150, 200, 0x07FF);
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setCursor(10, 30);  tft.print("> AYARLAR");
    tft.setCursor(10, 60);  tft.print("> SISTEM BILGISI");
    tft.setCursor(10, 90);  tft.print("> PAINT CIZIM");
    tft.setCursor(10, 120); tft.print("> DONANIM TESTLERI");
    tft.setCursor(10, 150); tft.print("> OYUNLAR");
    tft.setCursor(10, 180); tft.print("> RETRO-GO (OTA)");
}

// YENİ UYGULAMA: SİSTEM BİLGİSİ
void runSysInfo() {
    tft.fillScreen(whiteTheme ? 0xFFFF : 0x0000);
    tft.setTextColor(0x07FF); tft.setTextSize(2);
    tft.setCursor(10, 10); tft.print("SISTEM PANELI");
    tft.setTextSize(1); tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setCursor(10, 50); tft.printf("CPU HIZI: %d MHz", ESP.getCpuFreqMHz());
    tft.setCursor(10, 80); tft.printf("BOS RAM: %d KB", ESP.getFreeHeap()/1024);
    tft.setCursor(10, 110); tft.printf("BOS OPI PSRAM: %d KB", ESP.getFreePsram()/1024);
    tft.setCursor(10, 140); tft.printf("CHIP MODEL: ESP32-S3");
    tft.setCursor(10, 170); tft.printf("TEMA MODU: %s", whiteTheme ? "BEYAZ" : "KOYU");
    tft.setTextColor(0xF800); tft.setCursor(10, 210); tft.print("SELECT TUSU ILE CIKIS YAPIN");
    
    while(digitalRead(JOY_SELECT) == HIGH) { esp_task_wdt_reset(); delay(100); }
    currentState = DESKTOP; renderDesktop();
}

void runPaintApp() {
    tft.fillScreen(0xFFFF);
    tft.fillRect(280, 0, 40, 240, 0xC618);
    tft.fillRect(285, 10, 30, 30, 0xF800); // Kırmızı
    tft.fillRect(285, 50, 30, 30, 0x07E0); // Yeşil
    tft.fillRect(285, 90, 30, 30, 0x001F); // Mavi
    tft.fillRect(285, 130, 30, 30, 0xFFE0); // Sarı
    tft.fillRect(285, 170, 30, 30, 0x0000); // Silgi (Siyah)
    tft.fillRect(285, 210, 30, 25, 0xF81F); // ÇIKIŞ
    while(true) {
        esp_task_wdt_reset();
        if (touch.touched()) {
            TS_Point p = touch.getPoint();
            int tx = getTX(p.x); int ty = getTY(p.y);
            if (tx > 280) {
                if (ty > 210) break;
                else if (ty > 10 && ty < 40) paintColor = 0xF800;
                else if (ty > 50 && ty < 80) paintColor = 0x07E0;
                else if (ty > 90 && ty < 120) paintColor = 0x001F;
                else if (ty > 130 && ty < 160) paintColor = 0xFFE0;
                else if (ty > 170 && ty < 200) paintColor = 0xFFFF; // Silgi Rengi Beyaz
                delay(100);
            } else tft.fillCircle(tx, ty, 3, paintColor); // Çizim kalinlastirildi
        }
        if (digitalRead(JOY_SELECT) == LOW) break;
    }
    currentState = DESKTOP; renderDesktop();
}

// MUHAMMED: TAM SÜRÜM YILAN (SNAKE) OYUNU
void runSnake() {
    int x[50], y[50], len=5, dx=4, dy=0, ax=160, ay=120, score=0;
    for(int i=0;i<50;i++){x[i]=-10;y[i]=-10;}
    x[0]=160; y[0]=120;
    tft.fillScreen(0x0000);
    tft.fillRect(ax, ay, 4, 4, 0xF800); // İlk Elma
    tft.setCursor(10, 10); tft.setTextColor(0xFFFF); tft.print("SNAKE - SELECT ILE CIK");

    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int jx = analogRead(J1_X); int jy = analogRead(J1_Y);
        // Yön Kontrolü
        if(jx < 1000 && dx==0) {dx=-4; dy=0;}
        else if(jx > 3000 && dx==0) {dx=4; dy=0;}
        else if(jy < 1000 && dy==0) {dx=0; dy=-4;}
        else if(jy > 3000 && dy==0) {dx=0; dy=4;}

        tft.fillRect(x[len-1], y[len-1], 4, 4, 0x0000); // Kuyruğu Sil
        for(int i=len-1; i>0; i--) { x[i]=x[i-1]; y[i]=y[i-1]; } // Kaydır
        x[0]+=dx; y[0]+=dy; // Kafayı İlerlet

        // Elma Yeme
        if(abs(x[0]-ax)<4 && abs(y[0]-ay)<4) { 
            if(len<49) len++; score+=10;
            ax = random(5, 75)*4; ay = random(5, 55)*4;
            tft.fillRect(ax, ay, 4, 4, 0xF800); // Yeni Elma
        }
        
        tft.fillRect(x[0], y[0], 4, 4, 0x07E0); // Kafayı Çiz (Yeşil)
        
        // Çarpma Kontrolü (Duvarlar)
        if(x[0]<0 || x[0]>320 || y[0]<0 || y[0]>240) {
            tft.fillScreen(0xF800); tft.setCursor(120, 120); tft.setTextColor(0xFFFF); tft.printf("OYUN BITTI! SKOR: %d", score);
            delay(2000); break;
        }
        delay(40); // Hız
    }
    currentState = DESKTOP; renderDesktop();
}

void run3DCube() {
    tft.fillScreen(0x0000);
    float ax=0, ay=0;
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        ax += (analogRead(J1_X)-2048)*0.0001;
        ay += (analogRead(J1_Y)-2048)*0.0001;
        tft.fillScreen(0x0000);
        tft.drawRect(110+ax*10, 70+ay*10, 100, 100, 0x5DFF);
        tft.drawRect(130+ax*10, 90+ay*10, 100, 100, 0xFFFF);
        tft.drawLine(110+ax*10, 70+ay*10, 130+ax*10, 90+ay*10, 0x07E0);
        tft.drawLine(210+ax*10, 70+ay*10, 230+ax*10, 90+ay*10, 0x07E0);
        tft.drawLine(110+ax*10, 170+ay*10, 130+ax*10, 190+ay*10, 0x07E0);
        tft.drawLine(210+ax*10, 170+ay*10, 230+ax*10, 190+ay*10, 0x07E0);
        delay(20);
    }
    currentState = DESKTOP; renderDesktop();
}

void runPong() {
    int p1y=100, p2y=100, bx=160, by=120, bdx=3, bdy=3, s1=0, s2=0;
    tft.fillScreen(0);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        p1y = map(analogRead(J1_Y), 0, 4095, 0, 205);
        p2y = map(analogRead(J2_X), 0, 4095, 0, 205); // MUHAMMED: J2 FIX - Eksen Sağlandı
        tft.fillScreen(0);
        
        // Skor Tablosu
        tft.setCursor(130, 10); tft.setTextColor(0x03FF); tft.printf("%d - %d", s1, s2);
        
        tft.fillRect(10, p1y, 8, 35, 0xF800);
        tft.fillRect(302, p2y, 8, 35, 0x07E0);
        tft.fillCircle(bx, by, 3, 0xFFFF);
        bx+=bdx; by+=bdy;
        if(by<=0 || by>=240) bdy=-bdy;
        if(bx<=20 && by>p1y && by<p1y+35) bdx=-bdx;
        if(bx>=295 && by>p2y && by<p2y+35) bdx=-bdx;
        if(bx<0) { s2++; bx=160; by=120; delay(500); }
        if(bx>320) { s1++; bx=160; by=120; delay(500); }
        delay(20);
    }
    currentState = DESKTOP; renderDesktop();
}

void runJoyTest() {
    tft.fillScreen(0x0000);
    tft.setTextColor(0xFFFF); tft.setCursor(80, 10); tft.print("DUAL JOYSTICK TEST");
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int j1x = analogRead(J1_X); int j1y = analogRead(J1_Y);
        int j2x = analogRead(J2_X); int j2y = analogRead(J2_Y);
        tft.fillRect(20, 50, 130, 100, 0x1084); tft.setCursor(25, 60); tft.printf("SOL JOY\nX:%d Y:%d", j1x, j1y);
        tft.fillCircle(20 + map(j1x, 0, 4095, 5, 125), 50 + map(j1y, 0, 4095, 5, 95), 4, 0xF800);
        tft.fillRect(170, 50, 130, 100, 0x1084); tft.setCursor(175, 60); tft.printf("SAG JOY\nX:%d Y:%d", j2x, j2y);
        tft.fillCircle(170 + map(j2x, 0, 4095, 5, 125), 50 + map(j2y, 0, 4095, 5, 95), 4, 0x07E0);
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
    touch.begin(); touch.setRotation(1);
    prefs.begin("sov_v325", false);
    whiteTheme = prefs.getBool("theme", false);
    if(psramInit()) Serial.println("PSRAM READY");
    renderDesktop();
    esp_task_wdt_init(30, true);
}

void loop() {
    esp_task_wdt_reset();

    // POWER MENU FIX (GPIO 0 UZUN BASIŞ - 2 SN)
    if (digitalRead(JOY_SELECT) == LOW) {
        if (pressTimer == 0) pressTimer = millis();
        if (millis() - pressTimer > 2000 && !isLongPress) {
            isLongPress = true; currentState = POWER_MENU;
            tft.fillScreen(0x0000);
            tft.fillRect(60, 60, 200, 120, 0xF800);
            tft.setTextColor(0xFFFF); tft.setCursor(90, 80); tft.print("GUC SECENEKLERI");
            tft.fillRect(80, 100, 160, 30, 0x0000); tft.setCursor(120, 110); tft.print("KAPAT (UYKU)");
            tft.fillRect(80, 140, 160, 30, 0x03FF); tft.setCursor(110, 150); tft.print("YENIDEN BASLAT");
            delay(500);
        }
    } else { pressTimer = 0; isLongPress = false; }

    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int tx = getTX(p.x); int ty = getTY(p.y);

        if (currentState == DESKTOP) {
            if (tx < 50 && ty > 210) { currentState = START_MENU; renderStartMenu(); delay(300); }
        } 
        else if (currentState == START_MENU) {
            if (ty > 20 && ty < 50) { // AYARLAR
                currentState = SETTINGS; tft.fillScreen(whiteTheme ? 0xFFFF : 0); 
                tft.fillRect(50, 80, 220, 50, 0x07FF); tft.setTextColor(0xFFFF); tft.setCursor(100, 100); tft.print("TEMA DEGISTIR");
                tft.fillRect(50, 150, 220, 50, 0x07E0); tft.setCursor(110, 170); tft.print("KAYDET VE CIK");
                delay(300);
            }
            else if (ty > 50 && ty < 80) { currentState = SYS_INFO; runSysInfo(); }
            else if (ty > 80 && ty < 110) { currentState = PAINT; runPaintApp(); }
            else if (ty > 110 && ty < 140) { // TESTLER
                tft.fillRect(151, 110, 130, 70, 0x1084); tft.drawRect(151, 110, 130, 70, 0x07FF);
                tft.setCursor(160, 130); tft.print("1. DOKUNMA TEST"); tft.setCursor(160, 160); tft.print("2. JOYSTICK TEST");
                currentState = TEST_MENU; delay(300);
            }
            else if (ty > 140 && ty < 170) { // OYUNLAR
                tft.fillRect(151, 140, 130, 75, 0x1084); tft.drawRect(151, 140, 130, 75, 0xF81F);
                tft.setCursor(160, 150); tft.print("1. 3D KUBE"); tft.setCursor(160, 170); tft.print("2. YILAN"); tft.setCursor(160, 190); tft.print("3. PONG 2P");
                currentState = GAME_MENU; delay(300);
            }
            else if (ty > 170) { 
                const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo");
                if (target) { esp_ota_set_boot_partition(target); delay(500); ESP.restart(); }
            } else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == SETTINGS) {
            if (ty > 80 && ty < 130) { whiteTheme = !whiteTheme; prefs.putBool("theme", whiteTheme); delay(300); }
            if (ty > 150 && ty < 200) { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == GAME_MENU) {
            if (ty > 140 && ty < 165) run3DCube();
            else if (ty > 165 && ty < 185) runSnake();
            else if (ty > 185) runPong();
            else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == TEST_MENU) {
            if (ty > 110 && ty < 140) { 
                tft.fillScreen(0xFFFF); tft.setTextColor(0); tft.setCursor(10,10); tft.print("DOKUNMA TEST - SELECT:CIKIS");
                while(digitalRead(JOY_SELECT)==HIGH) { if(touch.touched()){ TS_Point tp = touch.getPoint(); tft.drawCircle(getTX(tp.x), getTY(tp.y), 10, 0x07FF); } esp_task_wdt_reset(); }
                currentState = DESKTOP; renderDesktop();
            }
            else if (ty > 140) runJoyTest();
            else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == POWER_MENU) {
            if (tx > 80 && tx < 240 && ty > 100 && ty < 130) { tft.fillScreen(0); tft.setCursor(100,120); tft.print("UYKU MODU..."); delay(1000); esp_deep_sleep_start(); }
            if (tx > 80 && tx < 240 && ty > 140 && ty < 170) { ESP.restart(); }
            if (tx < 60 || tx > 260 || ty < 60 || ty > 180) { currentState = DESKTOP; renderDesktop(); delay(300); } // Menü Dışı Tıklama İle Çıkış
        }
    }
}
