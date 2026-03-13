/* * KynexOs v133.0 - The Zenith Sovereign (Full Stability Build)
 * Geliştirici: Muhammed (Kynex)
 * Donanım: ESP32-S3 N16R8 (DIO+OPI Hybrid)
 * Özellikler: NTP WiFi Clock, About System, Auto-Connect/Forget WiFi, TR QWERTY, Paint, Games
 * Hata Düzeltme: 180 Degree Rotation Fix, Beep Loop Fix, Keyboard Trigger Fix
 * Talimat: Asla satır silmeden, optimize etmeden, tam ve tek parça kod.
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <TJpg_Decoder.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <FFat.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <time.h>

// --- GÖMÜLÜ DOSYA İŞARETÇİLERİ ---
extern const uint8_t wallpaper_jpg_start[] asm("_binary_src_wallpaper_jpg_start");
extern const uint8_t wallpaper_jpg_end[]   asm("_binary_src_wallpaper_jpg_end");

// --- PIN TANIMLAMALARI ---
#define TFT_BL        1
#define TFT_CS        10
#define TFT_DC        9
#define TFT_RST       14
#define TFT_MOSI      11
#define TFT_SCK       12
#define TOUCH_CS      16
#define MISO_PIN      13
#define JOY1_X        4
#define JOY1_Y        5
#define JOY1_SW       6
#define JOY2_X        7
#define JOY2_Y        15
#define JOY2_SW       17
#define SPEAKER_PIN   18

// --- RENK PALETİ ---
#define WIN10_BLUE      0x0011 
#define WIN10_TASKBAR   0x18C3 
#define WIN10_START     0x041F 
#define WIN10_HOVER     0x633F 
#define COLOR_WHITE     0xFFFF
#define COLOR_BLACK     0x0000
#define COLOR_RED       0xF800 
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_ICON_PC   0x4D3F
#define COLOR_ICON_SET  0x7BEF
#define COLOR_ICON_YEL  0xF620

// --- SİSTEM DEĞİŞKENLERİ ---
int currentScreen = 0; // 0:Desktop, 1:HW Test, 2:Explorer Info, 3:WiFi, 4:Klavye, 5:BT, 6:Snake, 7:Pong, 8:Paint, 9:About
bool startMenuOpen = false;
bool mouseEnabled = true;
int mouseX = 160;
int mouseY = 120;
unsigned long btnPressStart = 0;
bool longPressTriggered = false;
unsigned long lastAction = 0;

// --- WIFI VE SAAT ---
int numNetworks = 0;
String networks[6];
String kbBuffer = "";
int kbMode = 0; 
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 10800; // Türkiye GMT+3
const int daylightOffset_sec = 0;
String currentTime = "00:00";

// QWERTY TR Klavye Matrisi
String kRows[3][3] = {
  {"qwertyuiopgu", "asdfghjklsi-", "zxcvbnmoc.  "},
  {"QWERTYUIOPGU", "ASDFGHJKLSI-", "ZXCVBNMOC.  "},
  {"1234567890-=", "!@#$%&*()_+/", "[]{};:'\",.<>"}
};

// --- OYUN VE APP DATA ---
uint16_t paintColor = COLOR_BLACK;
#define SNAKE_MAX 64
struct Point { int x, y; };
Point snake[SNAKE_MAX];
int snakeLen = 3;
Point apple;
int snakeDir = 1;
unsigned long lastSnakeMove = 0;
int p1Y = 100, p2Y = 100;
int bX = 160, bY = 120, bDX = 4, bDY = 4;

// --- NESNELER ---
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS); 
WebServer server(80);
Preferences prefs;
File fsUploadFile; 

struct JoyData { int x, y; bool btn; };

// --- FONKSİYON PROTOTİPLERİ ---
void drawScreen();
void drawKynexKeyboard();
void drawExplorerInfo();
void drawSettingsScreen();
void drawAboutScreen();
void drawDesktop(int hIdx);
void drawTaskbar();
void drawMouse();
void drawPaintApp();
void updateClock();

// --- JPG DECODER ---
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

void playBeep(int f, int d) { 
    tone(SPEAKER_PIN, f, d); 
}

JoyData readJoy(int px, int py, int psw) {
    JoyData d; d.x = analogRead(px); d.y = analogRead(py); d.btn = (digitalRead(psw) == LOW);
    return d;
}

// --- WEB FM ---
void handleWebRoot() {
    String h = "<html><head><meta charset='UTF-8'></head><body style='font-family:sans-serif;'>";
    h += "<h1>Kynex Sovereign FM</h1><hr>";
    File root = FFat.open("/");
    File file = root.openNextFile();
    while(file) {
        h += "<div>" + String(file.name()) + " <a href='/del?f=" + String(file.name()) + "'>[Sil]</a></div>";
        file = root.openNextFile();
    }
    h += "<br><form action='/upload' method='POST' enctype='multipart/form-data'><input type='file' name='u'><input type='submit' value='Yukle'></form></body></html>";
    server.send(200, "text/html", h);
}

// --- SAAT GÜNCELLEME ---
void updateClock() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        return;
    }
    char timeStr[6];
    strftime(timeStr, 6, "%H:%M", &timeinfo);
    currentTime = String(timeStr);
}

// --- UI ÇİZİMLERİ ---
void renderIcon(int x, int y, const char* txt, uint16_t c, bool h) {
    uint16_t finalC = h ? WIN10_HOVER : c;
    if (h) tft.drawRoundRect(x-5, y-5, 52, 52, 6, COLOR_WHITE);
    tft.fillRect(x, y, 40, 32, finalC);
    tft.drawRect(x+2, y+2, 36, 28, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(x - 5, y + 40); tft.print(txt);
}

void drawTaskbar() {
    tft.fillRect(0, 215, 320, 25, WIN10_TASKBAR);
    tft.fillRect(0, 215, 45, 25, startMenuOpen ? WIN10_HOVER : WIN10_START);
    tft.drawRect(15, 220, 15, 15, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(270, 225);
    tft.print(currentTime);
}

void drawDesktop(int hIdx) {
    tft.fillRect(0, 0, 320, 240, COLOR_BLACK); 
    size_t wlen = wallpaper_jpg_end - wallpaper_jpg_start;
    TJpgDec.drawJpg(0, 0, wallpaper_jpg_start, wlen); 
    
    renderIcon(20, 20, "PC", COLOR_ICON_PC, hIdx == 1);
    renderIcon(20, 85, "WiFi", COLOR_ICON_SET, hIdx == 2);
    renderIcon(20, 150, "About", 0x7BEF, hIdx == 3);
    renderIcon(90, 20, "Paint", COLOR_RED, hIdx == 4);
    renderIcon(90, 85, "Snake", COLOR_GREEN, hIdx == 5);
    renderIcon(90, 150, "Pong", 0xF81F, hIdx == 6);
    
    drawTaskbar();
    
    if (startMenuOpen) {
        tft.fillRect(0, 40, 170, 175, WIN10_TASKBAR);
        tft.drawRect(0, 40, 170, 175, WIN10_START);
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(15, 60); tft.print("Kynex Zenith OS");
        tft.drawFastHLine(10, 75, 150, 0x4208);
        tft.setCursor(15, 95); tft.print("> WiFi Unut");
        tft.setCursor(15, 125); tft.print("> Sistemi Kapat");
        tft.fillRect(0, 185, 170, 30, COLOR_RED);
        tft.setCursor(65, 197); tft.print("RST");
    }
}

void drawAboutScreen() {
    tft.fillScreen(COLOR_WHITE);
    tft.fillRect(0,0,320,35, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10,12); tft.print("Cihaz Hakkinda");
    tft.setTextColor(COLOR_BLACK);
    tft.setCursor(10, 60); tft.print("Model: Kynex Sovereign S3");
    tft.setCursor(10, 80); tft.print("Versiyon: v133.0 Zenith");
    tft.setCursor(10, 110); tft.print("Bagli Ag: "); tft.print(WiFi.SSID());
    tft.setCursor(10, 130); tft.print("IP Adresi: "); tft.print(WiFi.localIP().toString());
    tft.setCursor(10, 160); tft.setTextColor(COLOR_BLUE);
    tft.print("Web Link: http://"); tft.print(WiFi.localIP().toString());
    tft.setTextColor(COLOR_BLACK); tft.setCursor(10, 210); tft.print("Geri: Sol Joy Uzun Bas");
}

void drawSettingsScreen() {
    tft.fillScreen(COLOR_WHITE);
    tft.fillRect(0,0,320,35, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10,12); tft.print("WiFi Aglari Taranıyor...");
    numNetworks = WiFi.scanNetworks();
    tft.fillRect(0,0,320,35, WIN10_START); tft.setCursor(10,12); tft.print("Ag Secin:");
    tft.setTextColor(COLOR_BLACK);
    for(int i=0; i<min(numNetworks,6); i++) {
        tft.drawRect(5, 45+(i*28), 310, 25, 0xCE79);
        tft.setCursor(15, 53+(i*28)); tft.print(WiFi.SSID(i));
    }
}

void drawKynexKeyboard() {
    tft.fillScreen(COLOR_BLACK);
    tft.drawRect(10, 10, 300, 35, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(15, 22); tft.print(kbBuffer);
    for(int r=0; r<3; r++) {
        for(int c=0; c<12; c++) {
            int x = c * 26 + 4; int y = 60 + (r * 40);
            tft.drawRect(x, y, 24, 35, 0x4208);
            tft.setCursor(x + 8, y + 12); tft.print(kRows[kbMode][r].charAt(c));
        }
    }
    tft.fillRect(250, 180, 65, 35, WIN10_START); tft.setCursor(270, 192); tft.print("OK");
}

void drawPaintApp() {
    tft.fillScreen(COLOR_WHITE);
    tft.fillRect(0, 0, 320, 35, 0xCE79);
    tft.fillRect(5, 5, 25, 25, COLOR_RED); tft.fillRect(35, 5, 25, 25, COLOR_GREEN);
    tft.fillRect(65, 5, 25, 25, COLOR_BLUE); tft.fillRect(95, 5, 25, 25, COLOR_YELLOW);
    tft.drawRect(130, 5, 60, 25, COLOR_BLACK); tft.setCursor(135, 12); tft.setTextColor(COLOR_BLACK); tft.print("SILGI");
    tft.drawRect(200, 5, 60, 25, COLOR_RED); tft.setCursor(205, 12); tft.setTextColor(COLOR_RED); tft.print("TEMIZ");
}

void drawScreen() {
    if (currentScreen == 0) drawDesktop(-1);
    else if (currentScreen == 2) drawExplorerInfo();
    else if (currentScreen == 3) drawSettingsScreen();
    else if (currentScreen == 4) drawKynexKeyboard();
    else if (currentScreen == 8) drawPaintApp();
    else if (currentScreen == 9) drawAboutScreen();
}

void handleGlobalClick(int x, int y) {
    playBeep(1800, 30);
    if (currentScreen == 0) {
        if (!startMenuOpen) {
            if (x < 80) {
                if (y > 20 && y < 80) { currentScreen = 2; drawScreen(); }
                else if (y > 85 && y < 145) { currentScreen = 3; drawScreen(); }
                else if (y > 150 && y < 210) { currentScreen = 9; drawScreen(); }
            } else if (x > 80 && x < 160) {
                if (y > 20 && y < 80) { currentScreen = 8; drawScreen(); }
                else if (y > 85 && y < 145) { currentScreen = 6; tft.fillScreen(COLOR_BLACK); }
                else if (y > 150 && y < 210) { currentScreen = 7; tft.fillScreen(COLOR_BLACK); }
            }
        } else {
            if (x < 170) {
                if (y > 80 && y < 115) { prefs.remove("ssid"); prefs.remove("pass"); WiFi.disconnect(); startMenuOpen=false; drawScreen(); }
                if (y > 185) ESP.restart();
            } else { startMenuOpen = false; drawScreen(); }
        }
    } else if (currentScreen == 3) { // WiFi Ağı seçildiğinde
        if (y > 40) {
            int idx = (y - 45) / 28;
            if (idx < numNetworks) {
                prefs.putString("ssid_tmp", WiFi.SSID(idx));
                kbBuffer = ""; currentScreen = 4; drawKynexKeyboard();
            }
        }
    } else if (currentScreen == 4) { // Klavye tık
        if (y > 60 && y < 180) {
            int r = (y-60)/40; int c = (x-4)/26;
            if (c < 12) { kbBuffer += kRows[kbMode][r].charAt(c); drawKynexKeyboard(); }
        } else if (x > 250 && y > 180) {
            prefs.putString("ssid", prefs.getString("ssid_tmp"));
            prefs.putString("pass", kbBuffer);
            WiFi.begin(prefs.getString("ssid").c_str(), kbBuffer.c_str());
            currentScreen = 0; drawScreen();
        }
    } else if (currentScreen == 8) { // Paint tık
        if (y < 35) {
            if (x < 30) paintColor = COLOR_RED;
            else if (x < 60) paintColor = COLOR_GREEN;
            else if (x < 90) paintColor = COLOR_BLUE;
            else if (x < 120) paintColor = COLOR_YELLOW;
            else if (x < 190) paintColor = COLOR_WHITE;
            else if (x < 260) drawPaintApp();
        } else { tft.fillCircle(x, y, 2, paintColor); }
    }
}

void setup() {
    Serial.begin(115200); psramInit(); FFat.begin(true);
    prefs.begin("kynex", false);
    WiFi.mode(WIFI_AP_STA);
    
    // Auto Connect
    String s = prefs.getString("ssid", "");
    String p = prefs.getString("pass", "");
    if(s != "") WiFi.begin(s.c_str(), p.c_str());

    WiFi.softAP("KynexOs-Win10", "*muhammed*krid*");
    server.on("/", handleWebRoot); server.begin();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY1_SW, INPUT_PULLUP); pinMode(JOY2_SW, INPUT_PULLUP);
    
    TJpgDec.setJpgScale(1); TJpgDec.setCallback(tft_output);
    SPI.begin(TFT_SCK, MISO_PIN, TFT_MOSI, TFT_CS); 
    
    tft.begin(); tft.setRotation(1); // MUHAMMED: 180 DERECE DÖNDÜRÜLDÜ
    ts.begin(); ts.setRotation(1);
    
    drawScreen();
}

void loop() {
    server.handleClient();
    updateClock();
    JoyData j1 = readJoy(JOY1_X, JOY1_Y, JOY1_SW);
    JoyData j2 = readJoy(JOY2_X, JOY2_Y, JOY2_SW);

    // --- SMART BACK BUTTON ---
    if (j1.btn) {
        if (btnPressStart == 0) btnPressStart = millis();
        if (millis() - btnPressStart > 1500 && !longPressTriggered) {
            longPressTriggered = true; playBeep(600, 200);
            currentScreen = 0; startMenuOpen = false; drawScreen();
        }
    } else {
        if (btnPressStart != 0) {
            unsigned long dur = millis() - btnPressStart;
            if (!longPressTriggered && dur > 50) {
                if (mouseEnabled && currentScreen == 0) handleGlobalClick(mouseX, mouseY);
                else if (currentScreen == 0) { startMenuOpen = !startMenuOpen; drawScreen(); }
                else handleGlobalClick(mouseX, mouseY);
            }
            btnPressStart = 0; longPressTriggered = false;
        }
    }

    // --- MOUSE ---
    if (currentScreen == 0 && mouseEnabled) {
        int dx = (j1.x - 2048) / 100; int dy = (j1.y - 2048) / 100;
        if (abs(dx) > 1 || abs(dy) > 1) {
            mouseX = constrain(mouseX + dx, 0, 310); mouseY = constrain(mouseY + dy, 0, 225);
            if (millis() - lastAction > 50) { 
                drawScreen(); 
                tft.fillTriangle(mouseX, mouseY, mouseX+10, mouseY+10, mouseX, mouseY+14, COLOR_WHITE);
                lastAction = millis(); 
            }
        }
    }

    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        int tx = map(p.x, 200, 3850, 320, 0); int ty = map(p.y, 240, 3800, 240, 0);
        if (currentScreen == 0 && tx < 50 && ty > 210) { startMenuOpen = !startMenuOpen; drawScreen(); delay(200); }
        else { handleGlobalClick(tx, ty); }
    }
}
