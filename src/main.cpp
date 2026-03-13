/* * KynexOs v123.0 - The Kynex Sovereign OS (Final Build)
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Full Screen Image, TR Keyboard, WiFi/BT Manager, Web FM
 * Hata Düzeltme: drawKeyboard Name Fix, Rotation Correction, Linking Fix
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
#include <Preferences.h>
#include <BluetoothSerial.h>

#include "wallpaper.h" 

// --- FONKSİYON PROTOTİPLERİ ---
void drawKynexKeyboard();
void handleGlobalInteraction(int x, int y);
void drawDesktop(int h);
void playBeep(int f, int d);
void drawWiFiSettings();
void startWebFM();

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
unsigned long btnPressStart = 0;
bool longPressTriggered = false;
unsigned long lastAction = 0;

String kbBuffer = "";
int kbMode = 0; // 0:abc, 1:ABC, 2:123
const char* keys[3][4] = {
  {"q w e r t y u i o p g u", "a s d f g h j k l s i", "z x c v b n m o c .", "123 SPACE BACK OK"},
  {"Q W E R T Y U I O P G U", "A S D F G H J K L S I", "Z X C V B N M O C .", "123 SPACE BACK OK"},
  {"1 2 3 4 5 6 7 8 9 0 -", "! @ # $ % ^ & * ( ) _", "+ = [ ] { } ; : ' \"", "abc SPACE BACK OK"}
};

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS); 
WebServer server(80);
Preferences prefs;
BluetoothSerial SerialBT;

// --- JPG DECODER ---
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

// --- SİSTEM FONKSİYONLARI ---
void playBeep(int f, int d) { tone(SPEAKER_PIN, f, d); }

struct JoyData { int x, y; bool btn; };
JoyData readJoy(int px, int py, int psw) {
    JoyData d; d.x = analogRead(px); d.y = analogRead(py); d.btn = (digitalRead(psw) == LOW);
    return d;
}

// --- WEB TABANLI DOSYA YÖNETİCİSİ (PRO) ---
void handleWebRoot() {
    String h = "<html><head><meta charset='UTF-8'><title>Kynex Sovereign OS</title></head><body style='background:#f0f2f5; font-family:sans-serif;'>";
    h += "<div style='max-width:600px; margin:auto; background:white; padding:20px; border-radius:15px; box-shadow:0 10px 25px rgba(0,0,0,0.1);'>";
    h += "<h1 style='color:#0078d7;'>Kynex Explorer</h1><hr>";
    File root = FFat.open("/");
    File file = root.openNextFile();
    while(file) {
        h += "<div style='display:flex; justify-content:space-between; padding:10px; border-bottom:1px solid #eee;'>";
        h += "<span>" + String(file.name()) + "</span>";
        h += "<span><a href='/edit?f=" + String(file.name()) + "'>[Duzenle]</a> <a href='/del?f=" + String(file.name()) + "' style='color:red;'>[Sil]</a></span></div>";
        file = root.openNextFile();
    }
    h += "<br><form action='/upload' method='POST' enctype='multipart/form-data'><input type='file' name='u'><input type='submit' value='Yukle' style='background:#0078d7; color:white; border:0; padding:10px;'></form>";
    h += "<form action='/mkdir'><input name='d' placeholder='Klasor Adi'><input type='submit' value='Olustur'></form></div></body></html>";
    server.send(200, "text/html", h);
}

// --- UI ÇİZİMLERİ ---
void drawKynexKeyboard() {
    tft.fillScreen(COLOR_BLACK);
    tft.drawRect(5, 5, 310, 45, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(15, 20);
    tft.print(kbBuffer);
    for(int i=0; i<4; i++) {
        tft.setCursor(10, 70 + (i*45));
        tft.print(keys[kbMode][i]);
    }
}

void drawTaskbar() {
    tft.fillRect(0, 215, 320, 25, WIN10_TASKBAR);
    tft.fillRect(0, 215, 45, 25, startMenuOpen ? WIN10_HOVER : WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(270, 225);
    tft.print(WiFi.status() == WL_CONNECTED ? "BAĞLI" : "AP");
}

void drawDesktop(int h) {
    // Muhammed, tam ekran için temizlik ve basım
    tft.fillRect(0,0,320,240, WIN10_BLUE);
    TJpgDec.drawJpg(0, 0, win10_wallpaper_embedded, sizeof(win10_wallpaper_embedded)); 
    
    // Simgeler
    if(h==1) tft.drawRoundRect(25, 25, 55, 55, 8, COLOR_WHITE);
    tft.fillRect(30,30,45,35, h==1 ? WIN10_HOVER : 0x4D3F);
    tft.setCursor(20,75); tft.print("Bu Bilgisayar");
    
    if(h==3) tft.drawRoundRect(25, 145, 55, 55,
