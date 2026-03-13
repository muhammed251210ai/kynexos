/* * KynexOs v119.0 - The Kynex Matrix (Total Fix Edition)
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Long Press logic fixed, Display Orientation Fixed, Turbo Cloud
 * Talimat: Asla satır silme, optimize etme, sadeleştirme yapma. 
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

#include "wallpaper.h" 

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

// --- SİSTEM DEĞİŞKENLERİ ---
int currentScreen = 0; 
bool startMenuOpen = false;
bool mouseEnabled = true;
float mouseX = 160, mouseY = 120;
int lastHoverIdx = -1;
bool longPressTriggered = false;

unsigned long btnPressStartTime = 0;
bool lastBtnState = HIGH;
unsigned long lastActionTime = 0;

struct JoystickData {
    int x;
    int y;
    bool btn;
};

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS); 
WebServer server(80);

// --- JPG DECODER ---
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

// --- SİSTEM FONKSİYONLARI ---
void playFeedback(int freq, int dur) {
    tone(SPEAKER_PIN, freq, dur);
}

JoystickData readJoystick(int pinX, int pinY, int pinSW) {
    JoystickData data;
    data.x = analogRead(pinX);
    data.y = analogRead(pinY);
    data.btn = (digitalRead(pinSW) == LOW);
    return data;
}

void drawWindowsLogo(int x, int y, int size, uint16_t color) {
    int s = size / 2 - 1;
    tft.fillRect(x, y, s, s, color);         
    tft.fillRect(x + s + 2, y, s, s, color); 
    tft.fillRect(x, y + s + 2, s, s, color); 
    tft.fillRect(x + s + 2, y + s + 2, s, s, color); 
}

// --- UI ÇİZİM ---
void renderIcon(int x, int y, const char* label, uint16_t color, bool hover) {
    uint16_t finalColor = hover ? WIN10_HOVER : color;
    if (hover) { tft.drawRoundRect(x-5, y-5, 52, 52, 5, COLOR_WHITE); }
    tft.fillRect(x, y, 40, 30, finalColor);
    tft.drawRect(x+3, y+3, 34, 24, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setTextSize(1);
    tft.setCursor(x - 5, y + 38); tft.print(label);
}

void drawTaskbar() {
    tft.fillRect(0, 215, 320, 25, WIN10_TASKBAR);
    tft.fillRect(0, 215, 45, 25, startMenuOpen ? WIN10_HOVER : WIN10_START);
    drawWindowsLogo(16, 221, 12, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(270, 225); tft.print("21:30");
}

void drawMouse() {
    if (!mouseEnabled) return;
    tft.fillTriangle(mouseX, mouseY, mouseX+12, mouseY+12, mouseX, mouseY+16, COLOR_WHITE);
    tft.drawTriangle(mouseX, mouseY, mouseX+12, mouseY+12, mouseX, mouseY+16, COLOR_BLACK);
}

void drawDesktop(int hoverIdx) {
    TJpgDec.drawJpg(0, 0, win10_wallpaper_embedded, sizeof(win10_wallpaper_embedded)); 
    renderIcon(20, 20, "PC", 0x4D3F, hoverIdx == 1);
    renderIcon(20, 85, "Cop", 0x7BEF, hoverIdx == 2);
    renderIcon(20, 150, "Test", 0xF620, hoverIdx == 3);
    drawTaskbar();
    if (startMenuOpen) {
        tft.fillRect(0, 50, 150, 165, WIN10_TASKBAR);
        tft.drawRect(0, 50, 150, 165, WIN10_START);
        tft.setTextColor(COLOR_WHITE); tft.setCursor(15, 65); tft.print("Kynex Muhammed");
        tft.fillRect(0, 185, 150, 30, COLOR_RED); tft.setCursor(55, 197); tft.print("KAPAT");
    }
}

void openFileExplorer() {
    tft.fillScreen(COLOR_WHITE); tft.fillRect(0, 0, 320, 30, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10, 10); tft.print("Kynex C:/");
    tft.setTextColor(COLOR_BLACK);
    File root = FFat.open("/"); File file = root.openNextFile(); int count = 0;
    while(file) { tft.setCursor(10, 45+(count*15)); tft.printf("%s (%u KB)", file.name(), file.size()/1024); file=root.openNextFile(); count++; }
}

void handleInteraction(int x, int y) {
    playFeedback(1500, 20);
    if (currentScreen == 0 && !startMenuOpen) {
        if (x < 85) {
            if (y > 20 && y < 80) { currentScreen = 2; openFileExplorer(); }
            else if (y > 140 && y < 200) { currentScreen = 1; tft.fillScreen(COLOR_BLACK); }
        }
    }
}

void setup() {
    Serial.begin(115200); psramInit(); FFat.begin(true);
    WiFi.softAP("Kynex-Win10", "12345678");
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY1_SW, INPUT_PULLUP);
    TJpgDec.setJpgScale(1); TJpgDec.setCallback(tft_output);
    SPI.begin(TFT_SCK, MISO_PIN, TFT_MOSI, TFT_CS); 
    tft.begin(); 
    tft.setRotation(3); // KRİTİK: Fotoğraftaki yön hatası buradan düzelir
    ts.begin(); ts.setRotation(3);
    playFeedback(1000, 200); drawDesktop(-1);
}

void loop() {
    server.handleClient();
    JoystickData j1 = readJoystick(JOY1_X, JOY1_Y, JOY1_SW);

    // --- GELİŞMİŞ BUTON MANTIĞI ---
    if (j1.btn == LOW) { // Butona basılıyor
        if (btnPressStartTime == 0) btnPressStartTime = millis();
        if (millis() - btnPressStartTime > 2000 && !longPressTriggered) { // 2 Sn Geçti
            mouseEnabled = !mouseEnabled;
            longPressTriggered = true;
            playFeedback(600, 300);
            currentScreen = 0; startMenuOpen = false; drawDesktop(-1);
        }
    } else { // Buton bırakıldı
        if (btnPressStartTime != 0) {
            unsigned long duration = millis() - btnPressStartTime;
            if (duration < 2000) { // Kısa Basış
                if (mouseEnabled) { handleInteraction(mouseX, mouseY); drawDesktop(-1); }
                else { startMenuOpen = !startMenuOpen; drawDesktop(-1); playFeedback(1200, 50); }
            }
            btnPressStartTime = 0; longPressTriggered = false;
        }
    }

    // --- MASAÜSTÜ VE FARE ---
    if (currentScreen == 0) {
        if (mouseEnabled) {
            int dx = (j1.x - 2048) / 100; int dy = (j1.y - 2048) / 100;
            if (abs(dx) > 1 || abs(dy) > 1) {
                mouseX = constrain(mouseX + dx, 0, 310); mouseY = constrain(mouseY + dy, 0, 210);
                int hover = -1;
                if (mouseX < 85) {
                    if (mouseY < 80) hover = 1; 
                    else if (mouseY > 140 && mouseY < 200) hover = 3;
                }
                if (millis() - lastActionTime > 50) { drawDesktop(hover); drawMouse(); lastActionTime = millis(); }
            }
        }
        if (ts.touched()) {
            TS_Point p = ts.getPoint();
            int tx = map(p.x, 200, 3850, 320, 0); int ty = map(p.y, 240, 3800, 240, 0);
            if (tx < 50 && ty > 200) { startMenuOpen = !startMenuOpen; drawDesktop(-1); delay(200); }
            else { handleInteraction(tx, ty); }
        }
    }
}
