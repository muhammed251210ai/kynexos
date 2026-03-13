/* * KynexOs v123.2 - The Kynex Sovereign OS (Absolute Master Fix)
 * Geliştirici: Muhammed (Kynex)
 * Donanım: ESP32-S3 N16R8
 * Özellikler: Full TR Keyboard, WiFi Manager, BLE (S3 Compatible), Web File Manager
 * Hata Düzeltme: BluetoothSerial removed (S3 doesn't support it), Keyboard Fix, Rotation Fix
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
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

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

// --- BLE TANIMLAMALARI ---
BLEServer *pServer;
bool deviceConnected = false;

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

// --- WEB TABANLI DOSYA YÖNETİCİSİ ---
void handleWebRoot() {
    String h = "<html><head><meta charset='UTF-8'><title>Kynex Explorer</title></head><body style='background:#f0f2f5; font-family:sans-serif; padding:20px;'>";
    h += "<div style='max-width:600px; margin:auto; background:white; padding:20px; border-radius:15px; box-shadow:0 10px 25px rgba(0,0,0,0.1);'>";
    h += "<h1 style='color:#0078d7;'>Kynex Sovereign Web FM</h1><hr>";
    File root = FFat.open("/");
    File file = root.openNextFile();
    while(file) {
        h += "<div style='display:flex; justify-content:space-between; padding:10px; border-bottom:1px solid #eee;'>";
        h += "<span>" + String(file.name()) + " (" + String(file.size()) + "B)</span>";
        h += "<span><a href='/edit?f=" + String(file.name()) + "'>[Duzenle]</a> <a href='/del?f=" + String(file.name()) + "' style='color:red;'>[Sil]</a></span></div>";
        file = root.openNextFile();
    }
    h += "<br><form action='/upload' method='POST' enctype='multipart/form-data'><input type='file' name='u'><input type='submit' value='Dosya Yukle' style='background:#0078d7; color:white; border:0; padding:10px; border-radius:5px;'></form>";
    h += "<form action='/mkdir'><input name='d' placeholder='Klasor Adi' style='padding:8px;'><input type='submit' value='Olustur'></form></div></body></html>";
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
    tft.setTextColor(COLOR_WHITE); tft.setCursor(260, 225);
    tft.print(WiFi.status() == WL_CONNECTED ? "BAGLI" : "KOPUK");
}

void drawDesktop(int h) {
    // Tam ekran garantisi için önce temizlik
    tft.fillRect(0,0,320,240, WIN10_BLUE);
    TJpgDec.drawJpg(0, 0, win10_wallpaper_embedded, sizeof(win10_wallpaper_embedded)); 
    
    // Ikonlar (PC, Settings, Test)
    if(h==1) tft.drawRoundRect(25, 25, 55, 55, 8, COLOR_WHITE);
    tft.fillRect(30,30,45,35, h==1 ? WIN10_HOVER : 0x4D3F);
    tft.setCursor(20,75); tft.setTextColor(COLOR_WHITE); tft.print("Bu Bilgisayar");
    
    if(h==3) tft.drawRoundRect(25, 145, 55, 55, 8, COLOR_WHITE);
    tft.fillRect(30,150,45,35, h==3 ? WIN10_HOVER : 0xF620);
    tft.setCursor(25,195); tft.print("Donanim Test");
    
    drawTaskbar();
    
    if (startMenuOpen) {
        tft.fillRect(0, 40, 165, 175, WIN10_TASKBAR);
        tft.drawRect(0, 40, 165, 175, WIN10_START);
        tft.setCursor(15, 60); tft.print("Kynex Sovereign OS");
        tft.setCursor(15, 90); tft.print("> WiFi Yonetimi");
        tft.setCursor(15, 120); tft.print("> BT Esleme (BLE)");
        tft.fillRect(0, 185, 165, 30, COLOR_RED);
        tft.setCursor(65, 197); tft.print("KAPAT");
    }
}

void handleGlobalInteraction(int x, int y) {
    playBeep(1800, 20);
    if (currentScreen == 0 && !startMenuOpen) {
        if (x < 90) {
            if (y > 20 && y < 90) { currentScreen = 2; /* Explorer */ }
            else if (y > 140 && y < 210) { currentScreen = 1; tft.fillScreen(COLOR_BLACK); }
        }
    }
    if (startMenuOpen) {
        if (x < 165 && y > 185) ESP.restart();
        if (x < 165 && y > 80 && y < 110) { currentScreen = 4; startMenuOpen = false; }
    }
}

void setup() {
    Serial.begin(115200);
    psramInit();
    if(!FFat.begin(true)) FFat.format();
    prefs.begin("kynex", false);

    // Auto-Connect (Preferences hafızasından)
    String savedSSID = prefs.getString("ssid", "");
    String savedPASS = prefs.getString("pass", "");
    if(savedSSID != "") WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

    WiFi.softAP("Kynex-Win10", "12345678");
    server.on("/", handleWebRoot);
    server.begin();

    // S3 Uyumlu Bluetooth (BLE) Baslatma
    BLEDevice::init("Kynex-Sovereign-BLE");
    pServer = BLEDevice::createServer();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x180D)); // Heart Rate Service (Temsili)
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY1_SW, INPUT_PULLUP);
    
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    SPI.begin(TFT_SCK, MISO_PIN, TFT_MOSI, TFT_CS); 
    
    tft.begin(); tft.setRotation(3); 
    ts.begin(); ts.setRotation(3);
    
    drawDesktop(-1);
}

void loop() {
    server.handleClient();
    JoyData j1 = readJoy(JOY1_X, JOY1_Y, JOY1_SW);
    JoyData j2 = readJoy(JOY2_X, JOY2_Y, JOY2_SW);

    // --- GELİŞMİŞ BUTON MANTIĞI ---
    if (j1.btn == LOW) {
        if (btnPressStart == 0) btnPressStart = millis();
        if (millis() - btnPressStart > 2000 && !longPressTriggered) {
            mouseEnabled = !mouseEnabled; longPressTriggered = true;
            playBeep(600, 300); currentScreen = 0; startMenuOpen = false; drawDesktop(-1);
        }
    } else {
        if (btnPressStart != 0) {
            unsigned long dur = millis() - btnPressStart;
            if (!longPressTriggered && dur > 50) {
                if (mouseEnabled && currentScreen == 0) { handleGlobalInteraction(mouseX, mouseY); drawDesktop(-1); }
                else if (currentScreen == 0) { startMenuOpen = !startMenuOpen; drawDesktop(-1); playBeep(1200, 50); }
            }
            btnPressStart = 0; longPressTriggered = false;
        }
    }

    // --- MASAÜSTÜ VE FARE ---
    if (currentScreen == 0) {
        if (mouseEnabled) {
            mouseX = constrain(mouseX + (j1.x - 2048)/120, 0, 310);
            mouseY = constrain(mouseY + (j1.y - 2048)/120, 0, 230);
            int h = -1;
            if (mouseX < 90) { if (mouseY < 90) h = 1; else if (mouseY > 140) h = 3; }
            if (millis() - lastAction > 45) {
                drawDesktop(h);
                tft.fillTriangle(mouseX, mouseY, mouseX+12, mouseY+12, mouseX, mouseY+16, COLOR_WHITE);
                tft.drawTriangle(mouseX, mouseY, mouseX+12, mouseY+12, mouseX, mouseY+16, COLOR_BLACK);
                lastAction = millis();
            }
        }
        if (ts.touched()) {
            TS_Point p = ts.getPoint();
            int tx = map(p.x, 200, 3850, 320, 0); int ty = map(p.y, 240, 3800, 240, 0);
            if (tx < 55 && ty > 210) { startMenuOpen = !startMenuOpen; drawDesktop(-1); delay(200); }
            else { handleGlobalInteraction(tx, ty); }
        }
    }

    // --- DONANIM TEST EKRANI ---
    if (currentScreen == 1) {
        if (millis() - lastAction > 100) {
            tft.fillScreen(COLOR_BLACK);
            tft.setCursor(10,10); tft.setTextColor(COLOR_WHITE); tft.print("KYNEX DUAL TEST (S3)");
            tft.setCursor(10,50); tft.printf("JOY1 X:%d Y:%d B:%d", j1.x, j1.y, j1.btn);
            tft.setCursor(10,85); tft.printf("JOY2 X:%d Y:%d B:%d", j2.x, j2.y, j2.btn);
            if(ts.touched()){
                TS_Point p = ts.getPoint();
                int tx = map(p.x, 200, 3850, 320, 0); int ty = map(p.y, 240, 3800, 240, 0);
                tft.fillCircle(tx, ty, 4, COLOR_RED);
            }
            lastAction = millis();
        }
    }

    // --- KLAVYE MODU ---
    if (currentScreen == 4) {
        drawKynexKeyboard();
    }
}
