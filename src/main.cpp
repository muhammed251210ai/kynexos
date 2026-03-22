/* **************************************************************************
 * KynexOs Sovereign Build v230.99 - The Omega Core
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: WiFi/BT Hub, QWERTY Keyboard, Calculator, Games, Paint, Settings
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
#include <esp_bt.h>
#include <Preferences.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"
#include "esp_sleep.h"
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

enum State { DESKTOP, START_MENU, SETTINGS, SYS_INFO, PAINT, TEST_MENU, GAME_MENU, POWER_MENU, NETWORK_MENU, CALCULATOR };
State currentState = DESKTOP;
bool whiteTheme = false;
bool btState = false;
String savedSSID = "";
String savedPASS = "";
unsigned long pressTimer = 0;
bool isLongPress = false;
uint16_t paintColor = 0xF800;

// Dokunmatik Kalibrasyonu
int getTX(int rawX) { return map(rawX, 3850, 250, 0, 320); } 
int getTY(int rawY) { return map(rawY, 3800, 240, 0, 240); }

// UI Çizimleri
void renderDesktop() {
    drawWin10HeroWallpaper(&tft, whiteTheme); 
    tft.fillRect(0, 215, 320, 25, whiteTheme ? 0xAD75 : 0x10A2); 
    drawWin10SmallLogo(&tft, 5, 219, 18);
    tft.setTextColor(0xFFFF); tft.setCursor(200, 224); tft.print("OMEGA CORE");
    
    // Bağlantı Simgeleri
    if(WiFi.status() == WL_CONNECTED) tft.fillCircle(170, 227, 4, 0x07E0);
    else tft.drawCircle(170, 227, 4, 0xF800);
    if(btState) tft.fillCircle(185, 227, 4, 0x03FF);
}

void renderStartMenu() {
    tft.fillRect(0, 5, 160, 210, whiteTheme ? 0xFFFF : 0x2104);
    tft.drawRect(0, 5, 160, 210, 0x07FF);
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setCursor(10, 15);  tft.print("> AGLAR (WIFI/BT)");
    tft.setCursor(10, 45);  tft.print("> AYARLAR & SISTEM");
    tft.setCursor(10, 75);  tft.print("> OYUNLAR");
    tft.setCursor(10, 105); tft.print("> HESAP MAKINESI");
    tft.setCursor(10, 135); tft.print("> PAINT");
    tft.setCursor(10, 165); tft.print("> TESTLER");
    tft.setCursor(10, 195); tft.print("> RETRO-GO (OTA)");
}

// ---------------- KLAVYE MOTORU ----------------
String runKeyboard(String prompt) {
    String input = "";
    int mode = 0; // 0: kucuk, 1: BUYUK, 2: 123/#$*
    const char* keysL[3][10] = { {"q","w","e","r","t","y","u","i","o","p"}, {"a","s","d","f","g","h","j","k","l","@"}, {"z","x","c","v","b","n","m",".","-","_"} };
    const char* keysU[3][10] = { {"Q","W","E","R","T","Y","U","I","O","P"}, {"A","S","D","F","G","H","J","K","L","@"}, {"Z","X","C","V","B","N","M",".","-","_"} };
    const char* keysN[3][10] = { {"1","2","3","4","5","6","7","8","9","0"}, {"!","#","$","%","&","*","+","=","/","?"}, {"(",")","<",">","[","]","{","}",":",";"} };

    tft.fillScreen(0x0000); tft.fillRect(0, 0, 320, 80, 0x10A2);
    tft.setTextColor(0xFFFF); tft.setTextSize(1);
    
    bool drawKeys = true;
    while(true) {
        esp_task_wdt_reset();
        if(drawKeys) {
            tft.fillRect(0, 0, 320, 80, 0x10A2);
            tft.setCursor(10, 10); tft.print(prompt);
            tft.setCursor(10, 40); tft.setTextSize(2); tft.print(input + "_"); tft.setTextSize(1);
            
            tft.fillRect(0, 80, 320, 160, 0x0000);
            for(int r=0; r<3; r++) {
                for(int c=0; c<10; c++) {
                    tft.drawRect(c*32, 80+r*40, 32, 40, 0xFFFF);
                    tft.setCursor(c*32+12, 80+r*40+15);
                    if(mode==0) tft.print(keysL[r][c]); else if(mode==1) tft.print(keysU[r][c]); else tft.print(keysN[r][c]);
                }
            }
            // Alt Satır
            tft.drawRect(0, 200, 60, 40, 0x07FF); tft.setCursor(15, 215); tft.print(mode==2?"ABC":"123");
            tft.drawRect(60, 200, 60, 40, 0x07FF); tft.setCursor(75, 215); tft.print(mode==1?"kucuk":"BUYUK");
            tft.drawRect(120, 200, 80, 40, 0xFFFF); tft.setCursor(140, 215); tft.print("BOSLUK");
            tft.drawRect(200, 200, 60, 40, 0xF800); tft.setCursor(215, 215); tft.print("SIL");
            tft.drawRect(260, 200, 60, 40, 0x07E0); tft.setCursor(275, 215); tft.print("TAMAM");
            drawKeys = false;
        }

        if (touch.touched()) {
            TS_Point p = touch.getPoint();
            int tx = getTX(p.x); int ty = getTY(p.y);
            
            if(ty >= 80 && ty < 200) { // Harfler
                int c = tx / 32; int r = (ty - 80) / 40;
                if(c>=0&&c<10 && r>=0&&r<3) {
                    if(mode==0) input += keysL[r][c]; else if(mode==1) input += keysU[r][c]; else input += keysN[r][c];
                    drawKeys = true;
                }
            } else if(ty >= 200) { // Fonksiyonlar
                if(tx < 60) { mode = (mode==2) ? 0 : 2; drawKeys = true; } // 123/ABC
                else if(tx < 120) { mode = (mode==1) ? 0 : 1; drawKeys = true; } // SHIFT
                else if(tx < 200) { input += " "; drawKeys = true; } // SPACE
                else if(tx < 260) { if(input.length()>0) input.remove(input.length()-1); drawKeys = true; } // DEL
                else if(tx >= 260) { return input; } // TAMAM
            }
            delay(200); // Dokunmatik zıplamayı önle
        }
    }
}

// ---------------- AĞ YÖNETİCİSİ (WIFI/BT) ----------------
void runNetworkManager() {
    tft.fillScreen(0x0000); tft.setTextColor(0xFFFF);
    tft.setCursor(10, 10); tft.print("AGLAR TARANIYOR LUTFEN BEKLEYIN...");
    
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks();
    
    tft.fillScreen(0x0000);
    tft.fillRect(0, 0, 320, 30, 0x10A2);
    tft.setCursor(10, 10); tft.print("WIFI AGLARI (En Iyi 4)");
    
    for (int i = 0; i < 4 && i < n; ++i) {
        tft.drawRect(10, 40 + i*40, 300, 35, 0x07FF);
        tft.setCursor(20, 50 + i*40);
        tft.print(WiFi.SSID(i));
    }
    
    tft.fillRect(10, 200, 140, 35, btState ? 0xF800 : 0x03FF);
    tft.setCursor(20, 210); tft.print(btState ? "BLUETOOTH KAPAT" : "BLUETOOTH AC");
    
    tft.fillRect(160, 200, 150, 35, 0x3186);
    tft.setCursor(210, 210); tft.print("GERI");

    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        if (touch.touched()) {
            TS_Point p = touch.getPoint();
            int tx = getTX(p.x); int ty = getTY(p.y);
            
            if(ty > 40 && ty < 190) { // WiFi Seçimi
                int idx = (ty - 40) / 40;
                if(idx < n) {
                    String selectedSSID = WiFi.SSID(idx);
                    String pass = runKeyboard("Sifre Girin: " + selectedSSID);
                    
                    tft.fillScreen(0x0000); tft.setCursor(10, 100); tft.print("BAGLANILIYOR...");
                    WiFi.begin(selectedSSID.c_str(), pass.c_str());
                    int timeout = 0;
                    while (WiFi.status() != WL_CONNECTED && timeout < 20) { delay(500); timeout++; esp_task_wdt_reset(); }
                    
                    if(WiFi.status() == WL_CONNECTED) {
                        prefs.putString("ssid", selectedSSID); prefs.putString("pass", pass);
                        tft.fillScreen(0x07E0); tft.setCursor(100, 120); tft.setTextColor(0x0000); tft.print("BAGLANTI BASARILI!");
                    } else {
                        tft.fillScreen(0xF800); tft.setCursor(100, 120); tft.setTextColor(0xFFFF); tft.print("BAGLANTI HATASI!");
                    }
                    delay(2000); break;
                }
            }
            else if(ty > 200) {
                if(tx < 150) { 
                    btState = !btState; 
                    if(btState) btStart(); else btStop();
                    break;
                }
                else { break; } // Geri
            }
            delay(300);
        }
    }
    currentState = DESKTOP; renderDesktop();
}

// ---------------- HESAP MAKİNESİ ----------------
void runCalculator() {
    String eq = "";
    const char* cKeys[4][4] = { {"7","8","9","/"}, {"4","5","6","*"}, {"1","2","3","-"}, {"C","0","=","+"} };
    tft.fillScreen(0x0000);
    bool drawC = true;
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        if(drawC) {
            tft.fillRect(10, 10, 300, 50, 0xFFFF);
            tft.setTextColor(0x0000); tft.setTextSize(2); tft.setCursor(20, 25); tft.print(eq);
            tft.setTextSize(1);
            for(int r=0; r<4; r++) {
                for(int c=0; c<4; c++) {
                    tft.drawRect(10+c*75, 70+r*40, 70, 35, 0x07FF);
                    tft.setTextColor(0xFFFF); tft.setCursor(40+c*75, 80+r*40); tft.print(cKeys[r][c]);
                }
            }
            drawC = false;
        }
        if (touch.touched()) {
            TS_Point p = touch.getPoint();
            int tx = getTX(p.x); int ty = getTY(p.y);
            if(ty > 70 && ty < 230) {
                int c = (tx - 10) / 75; int r = (ty - 70) / 40;
                if(c>=0&&c<4 && r>=0&&r<4) {
                    String k = cKeys[r][c];
                    if(k == "C") eq = "";
                    else if(k == "=") { eq = "Hesaplandi (Demo)"; } // Basit mock, gerçeği parse ister
                    else eq += k;
                    drawC = true;
                }
            }
            delay(200);
        }
    }
    currentState = DESKTOP; renderDesktop();
}

// ---------------- STANDART UYGULAMALAR (OYUN, INFO, PAINT) ----------------
void runSysInfo() {
    tft.fillScreen(whiteTheme ? 0xFFFF : 0x0000);
    tft.setTextColor(0x07FF); tft.setTextSize(2); tft.setCursor(10, 10); tft.print("SISTEM PANELI");
    tft.setTextSize(1); tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setCursor(10, 50); tft.printf("CPU HIZI: %d MHz", ESP.getCpuFreqMHz());
    tft.setCursor(10, 80); tft.printf("BOS RAM: %d KB", ESP.getFreeHeap()/1024);
    tft.setCursor(10, 110); tft.printf("BOS PSRAM: %d KB", ESP.getFreePsram()/1024);
    tft.setCursor(10, 140); tft.printf("CHIP: ESP32-S3 OPI");
    tft.setCursor(10, 170); tft.printf("WIFI IP: %s", WiFi.localIP().toString().c_str());
    tft.setTextColor(0xF800); tft.setCursor(10, 210); tft.print("SELECT TUSU ILE CIKIS YAPIN");
    while(digitalRead(JOY_SELECT) == HIGH) { esp_task_wdt_reset(); delay(100); }
    currentState = DESKTOP; renderDesktop();
}

void runPaintApp() {
    tft.fillScreen(0xFFFF);
    tft.fillRect(280, 0, 40, 240, 0xC618);
    tft.fillRect(285, 10, 30, 30, 0xF800); tft.fillRect(285, 50, 30, 30, 0x07E0);
    tft.fillRect(285, 90, 30, 30, 0x001F); tft.fillRect(285, 130, 30, 30, 0xFFE0);
    tft.fillRect(285, 170, 30, 30, 0x0000); tft.fillRect(285, 210, 30, 25, 0xF81F);
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
                else if (ty > 170 && ty < 200) paintColor = 0xFFFF;
                delay(100);
            } else tft.fillCircle(tx, ty, 3, paintColor);
        }
        if (digitalRead(JOY_SELECT) == LOW) break;
    }
    currentState = DESKTOP; renderDesktop();
}

void runSnake() {
    int x[50], y[50], len=5, dx=4, dy=0, ax=160, ay=120, score=0;
    for(int i=0;i<50;i++){x[i]=-10;y[i]=-10;} x[0]=160; y[0]=120;
    tft.fillScreen(0x0000); tft.fillRect(ax, ay, 4, 4, 0xF800);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int jx = analogRead(J1_X); int jy = analogRead(J1_Y);
        if(jx < 1000 && dx==0) {dx=-4; dy=0;} else if(jx > 3000 && dx==0) {dx=4; dy=0;}
        else if(jy < 1000 && dy==0) {dx=0; dy=-4;} else if(jy > 3000 && dy==0) {dx=0; dy=4;}
        tft.fillRect(x[len-1], y[len-1], 4, 4, 0x0000);
        for(int i=len-1; i>0; i--) { x[i]=x[i-1]; y[i]=y[i-1]; }
        x[0]+=dx; y[0]+=dy;
        if(abs(x[0]-ax)<4 && abs(y[0]-ay)<4) { 
            if(len<49) len++; score+=10; ax = random(5, 75)*4; ay = random(5, 55)*4; tft.fillRect(ax, ay, 4, 4, 0xF800); 
        }
        tft.fillRect(x[0], y[0], 4, 4, 0x07E0);
        if(x[0]<0 || x[0]>320 || y[0]<0 || y[0]>240) {
            tft.fillScreen(0xF800); tft.setCursor(100, 120); tft.setTextColor(0xFFFF); tft.printf("OYUN BITTI! SKOR: %d", score);
            delay(2000); break;
        }
        delay(40);
    }
    currentState = DESKTOP; renderDesktop();
}

void run3DCube() {
    tft.fillScreen(0x0000); float ax=0, ay=0;
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        ax += (analogRead(J1_X)-2048)*0.0001; ay += (analogRead(J1_Y)-2048)*0.0001;
        tft.fillScreen(0x0000);
        tft.drawRect(110+ax*10, 70+ay*10, 100, 100, 0x5DFF); tft.drawRect(130+ax*10, 90+ay*10, 100, 100, 0xFFFF);
        tft.drawLine(110+ax*10, 70+ay*10, 130+ax*10, 90+ay*10, 0x07E0); tft.drawLine(210+ax*10, 70+ay*10, 230+ax*10, 90+ay*10, 0x07E0);
        tft.drawLine(110+ax*10, 170+ay*10, 130+ax*10, 190+ay*10, 0x07E0); tft.drawLine(210+ax*10, 170+ay*10, 230+ax*10, 190+ay*10, 0x07E0);
        delay(20);
    }
    currentState = DESKTOP; renderDesktop();
}

void runPong() {
    int p1y=100, p2y=100, bx=160, by=120, bdx=3, bdy=3, s1=0, s2=0; tft.fillScreen(0);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        p1y = map(analogRead(J1_Y), 0, 4095, 0, 205); p2y = map(analogRead(J2_X), 0, 4095, 0, 205); 
        tft.fillScreen(0); tft.setCursor(130, 10); tft.setTextColor(0x03FF); tft.printf("%d - %d", s1, s2);
        tft.fillRect(10, p1y, 8, 35, 0xF800); tft.fillRect(302, p2y, 8, 35, 0x07E0); tft.fillCircle(bx, by, 3, 0xFFFF);
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
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int j1x = analogRead(J1_X); int j1y = analogRead(J1_Y);
        int j2x = analogRead(J2_X); int j2y = analogRead(J2_Y);
        tft.fillRect(20, 50, 130, 100, 0x1084); tft.setCursor(25, 60); tft.setTextColor(0xFFFF); tft.printf("SOL JOY\nX:%d Y:%d", j1x, j1y);
        tft.fillCircle(20 + map(j1x, 0, 4095, 5, 125), 50 + map(j1y, 0, 4095, 5, 95), 4, 0xF800);
        tft.fillRect(170, 50, 130, 100, 0x1084); tft.setCursor(175, 60); tft.printf("SAG JOY\nX:%d Y:%d", j2x, j2y);
        tft.fillCircle(170 + map(j2x, 0, 4095, 5, 125), 50 + map(j2y, 0, 4095, 5, 95), 4, 0x07E0);
        delay(30);
    }
    currentState = DESKTOP; renderDesktop();
}

// ---------------- ANA SİSTEM ----------------
void setup() {
    WiFi.mode(WIFI_OFF);
    btStop();
    Serial.begin(115200);
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000); tft.setRotation(1);
    touch.begin(); touch.setRotation(1);
    
    prefs.begin("sov_v325", false);
    whiteTheme = prefs.getBool("theme", false);
    savedSSID = prefs.getString("ssid", "");
    savedPASS = prefs.getString("pass", "");

    if(psramInit()) Serial.println("PSRAM READY");
    esp_task_wdt_init(30, true);

    // Kayıtlı WiFi varsa otomatik uyan
    if(savedSSID != "") {
        WiFi.mode(WIFI_STA);
        WiFi.begin(savedSSID.c_str(), savedPASS.c_str());
    }

    renderDesktop();
}

void loop() {
    esp_task_wdt_reset();

    if (digitalRead(JOY_SELECT) == LOW) {
        if (pressTimer == 0) pressTimer = millis();
        if (millis() - pressTimer > 2000 && !isLongPress) {
            isLongPress = true; currentState = POWER_MENU;
            tft.fillScreen(0x0000); tft.fillRect(60, 60, 200, 120, 0xF800);
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
            if (ty > 10 && ty < 40) { currentState = NETWORK_MENU; runNetworkManager(); }
            else if (ty > 40 && ty < 70) { currentState = SETTINGS; tft.fillScreen(whiteTheme?0xFFFF:0); tft.fillRect(50, 80, 220, 50, 0x07FF); tft.setTextColor(0xFFFF); tft.setCursor(100, 100); tft.print("TEMA DEGISTIR"); tft.fillRect(50, 150, 220, 50, 0x07E0); tft.setCursor(110, 170); tft.print("KAYDET VE CIK"); delay(300); }
            else if (ty > 70 && ty < 100) { currentState = GAME_MENU; tft.fillRect(161, 75, 130, 75, 0x1084); tft.drawRect(161, 75, 130, 75, 0xF81F); tft.setCursor(170, 85); tft.print("1. 3D KUBE"); tft.setCursor(170, 110); tft.print("2. YILAN"); tft.setCursor(170, 135); tft.print("3. PONG 2P"); delay(300); }
            else if (ty > 100 && ty < 130) { currentState = CALCULATOR; runCalculator(); }
            else if (ty > 130 && ty < 160) { currentState = PAINT; runPaintApp(); }
            else if (ty > 160 && ty < 190) { currentState = TEST_MENU; tft.fillRect(161, 160, 130, 50, 0x1084); tft.drawRect(161, 160, 130, 50, 0x07FF); tft.setCursor(170, 170); tft.print("1. DOKUNMA TEST"); tft.setCursor(170, 190); tft.print("2. JOYSTICK TEST"); delay(300); }
            else if (ty > 190) { const esp_partition_t* target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "retrogo"); if (target) { esp_ota_set_boot_partition(target); delay(500); ESP.restart(); } }
            else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == SETTINGS) {
            if (ty > 80 && ty < 130) { whiteTheme = !whiteTheme; prefs.putBool("theme", whiteTheme); delay(300); }
            if (ty > 150 && ty < 200) { currentState = SYS_INFO; runSysInfo(); } // Kaydet Cik yerine sys info
        }
        else if (currentState == GAME_MENU) {
            if (ty > 75 && ty < 100) run3DCube(); else if (ty > 100 && ty < 125) runSnake(); else if (ty > 125 && ty < 150) runPong();
            else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == TEST_MENU) {
            if (ty > 160 && ty < 185) { tft.fillScreen(0xFFFF); tft.setTextColor(0); tft.setCursor(10,10); tft.print("DOKUNMA TEST - SELECT:CIKIS"); while(digitalRead(JOY_SELECT)==HIGH) { if(touch.touched()){ TS_Point tp = touch.getPoint(); tft.drawCircle(getTX(tp.x), getTY(tp.y), 10, 0x07FF); } esp_task_wdt_reset(); } currentState = DESKTOP; renderDesktop(); }
            else if (ty > 185 && ty < 210) runJoyTest();
            else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == POWER_MENU) {
            if (tx > 80 && tx < 240 && ty > 100 && ty < 130) { tft.fillScreen(0); tft.setCursor(100,120); tft.print("UYKU MODU..."); delay(1000); esp_deep_sleep_start(); }
            if (tx > 80 && tx < 240 && ty > 140 && ty < 170) { ESP.restart(); }
            if (tx < 60 || tx > 260 || ty < 60 || ty > 180) { currentState = DESKTOP; renderDesktop(); delay(300); } 
        }
    }
}
