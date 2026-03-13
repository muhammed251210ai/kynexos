/* * KynexOs v129.0 - The Master Core (Infinite Loop & Radio Crash Fix)
 * Geliştirici: Muhammed (Kynex)
 * Donanım: ESP32-S3 N16R8 (DIO+OPI)
 * Özellikler: Dual WiFi Mode (AP+STA), Precise Hitboxes, Tech-Grid BG, BLE Manager, FFat
 * Hata Düzeltme: WiFi Password Rejection (Radio Loop Fix), Start Menu Sub-tabs Unresponsive Fix
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
#define COLOR_ICON_PC   0x4D3F
#define COLOR_ICON_SET  0x7BEF
#define COLOR_ICON_YEL  0xF620

// --- SİSTEM DEĞİŞKENLERİ ---
int currentScreen = 0; // 0: Masaüstü, 1: HW Test, 2: Web FM, 3: WiFi Scanner, 4: Klavye, 5: BT Scanner
bool startMenuOpen = false;
bool mouseEnabled = true;
float mouseX = 160, mouseY = 120;

unsigned long btnPressStart = 0;
bool longPressTriggered = false;
unsigned long lastAction = 0;

// --- WIFI VE KLAVYE DEĞİŞKENLERİ ---
int numNetworks = 0;
String networks[6];
String kbBuffer = "";
int kbMode = 0; 

// Hatasiz TR Klavye Matrisi
String kRows[3][3] = {
  {"qwertyuiopgu", "asdfghjklsi-", "zxcvbnmoc.  "},
  {"QWERTYUIOPGU", "ASDFGHJKLSI-", "ZXCVBNMOC.  "},
  {"1234567890-=", "!@#$%&*()_+/", "[]{};:'\",.<>"}
};

// --- NESNELER ---
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS); 
WebServer server(80);
Preferences prefs;
File fsUploadFile; 

// --- FONKSİYON PROTOTİPLERİ ---
void drawScreen();
void drawKynexKeyboard();
void drawExplorerInfo();
void drawSettingsScreen();
void drawBTScreen();
void drawDesktop(int hoverIdx);
void drawTaskbar();

// --- JPG DECODER ---
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

// --- DONANIM KONTROL ---
void playBeep(int f, int d) { tone(SPEAKER_PIN, f, d); }

struct JoyData { int x, y; bool btn; };
JoyData readJoy(int px, int py, int psw) {
    JoyData d; 
    d.x = analogRead(px); d.y = analogRead(py); d.btn = (digitalRead(psw) == LOW);
    return d;
}

// --- WEB DOSYA YÖNETİCİSİ (FFAT) ---
void handleWebRoot() {
    String h = "<html><head><meta charset='UTF-8'><title>Kynex OS</title></head>";
    h += "<body style='background:#f0f2f5; font-family:sans-serif; padding:20px;'>";
    h += "<div style='max-width:600px; margin:auto; background:white; padding:20px; border-radius:15px; box-shadow:0 10px 25px rgba(0,0,0,0.1);'>";
    h += "<h1 style='color:#0078d7; text-align:center;'>Kynex Explorer</h1><hr>";
    
    File root = FFat.open("/");
    File file = root.openNextFile();
    if(!file) h += "<p style='text-align:center; color:#888;'>Hic dosya bulunamadi.</p>";
    
    while(file) {
        h += "<div style='display:flex; justify-content:space-between; padding:10px; border-bottom:1px solid #eee;'>";
        h += "<span>📁 <b>" + String(file.name()) + "</b> <small>(" + String(file.size()) + " B)</small></span>";
        h += "<span><a href='/del?f=" + String(file.name()) + "' style='text-decoration:none; color:red;'>[Sil]</a></span></div>";
        file = root.openNextFile();
    }
    h += "<br><hr><div style='display:flex; justify-content:space-between; flex-wrap:wrap; gap:10px;'>";
    h += "<form action='/upload' method='POST' enctype='multipart/form-data'><input type='file' name='u'> <input type='submit' value='Yukle' style='background:#0078d7; color:white; border:0; padding:8px 15px; border-radius:5px;'></form>";
    h += "<form action='/mkdir'><input name='d' placeholder='Klasor Adi' style='padding:8px;'> <input type='submit' value='Olustur' style='background:#28a745; color:white; border:0; padding:8px 15px; border-radius:5px;'></form>";
    h += "</div></div></body></html>";
    server.send(200, "text/html", h);
}

void handleFileDelete() {
    if(server.hasArg("f")) { FFat.remove("/" + server.arg("f")); }
    server.sendHeader("Location", "/"); server.send(303);
}

void handleMkdir() {
    if(server.hasArg("d")) { FFat.mkdir("/" + server.arg("d")); }
    server.sendHeader("Location", "/"); server.send(303);
}

void handleFileUpload() {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
        String filename = upload.filename;
        if(!filename.startsWith("/")) filename = "/" + filename;
        fsUploadFile = FFat.open(filename, FILE_WRITE);
    } else if(upload.status == UPLOAD_FILE_WRITE){
        if(fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
    } else if(upload.status == UPLOAD_FILE_END){
        if(fsUploadFile) fsUploadFile.close();
    }
}

// --- UI ÇİZİMLERİ ---
void drawWindowsLogo(int x, int y, int s, uint16_t c) {
    int sz = s/2 - 1;
    tft.fillRect(x, y, sz, sz, c); tft.fillRect(x+sz+2, y, sz, sz, c);
    tft.fillRect(x, y+sz+2, sz, sz, c); tft.fillRect(x+sz+2, y+sz+2, sz, sz, c);
}

void renderIcon(int x, int y, const char* txt, uint16_t c, bool h) {
    uint16_t finalC = h ? WIN10_HOVER : c;
    if (h) tft.drawRoundRect(x-5, y-5, 52, 52, 6, COLOR_WHITE);
    tft.fillRect(x, y, 40, 32, finalC);
    tft.drawRect(x+2, y+2, 36, 28, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setTextSize(1);
    tft.setCursor(x - 10, y + 40); tft.print(txt);
}

void drawTaskbar() {
    tft.fillRect(0, 215, 320, 25, WIN10_TASKBAR);
    tft.fillRect(0, 215, 45, 25, startMenuOpen ? WIN10_HOVER : WIN10_START);
    drawWindowsLogo(16, 221, 12, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(240, 225);
    tft.print(WiFi.status() == WL_CONNECTED ? "WIFI: BAGLI" : "WIFI: KOPUK");
}

void drawDesktop(int hoverIdx) {
    tft.fillRect(0, 0, 320, 240, COLOR_BLACK); 
    // Siber Izgara (Resmin yarida kesilme ihtimaline karsi profesyonel gorunum)
    for(int i=0; i<320; i+=20) tft.drawFastVLine(i, 0, 240, 0x18C3);
    for(int i=0; i<240; i+=20) tft.drawFastHLine(0, i, 320, 0x18C3);
    
    if (FFat.exists("/wallpaper.jpg")) {
        TJpgDec.drawFsJpg(0, 0, "/wallpaper.jpg");
    } else {
        size_t wallpaper_len = wallpaper_jpg_end - wallpaper_jpg_start;
        TJpgDec.drawJpg(0, 0, wallpaper_jpg_start, wallpaper_len); 
    }
    
    renderIcon(30, 20, "Bu Bilgisayar", COLOR_ICON_PC, hoverIdx == 1);
    renderIcon(30, 85, "Aga Baglan", COLOR_ICON_SET, hoverIdx == 2);
    renderIcon(30, 150, "Donanim Test", COLOR_ICON_YEL, hoverIdx == 3);
    
    drawTaskbar();
    
    if (startMenuOpen) {
        tft.fillRect(0, 40, 170, 175, WIN10_TASKBAR);
        tft.drawRect(0, 40, 170, 175, WIN10_START);
        tft.setCursor(15, 60); tft.setTextColor(COLOR_WHITE); tft.print("Kynex Sovereign OS");
        tft.drawFastHLine(10, 75, 150, 0x4208);
        tft.setCursor(15, 95); tft.print("> WiFi Tara & Baglan");
        tft.setCursor(15, 135); tft.print("> BT Cihazlari (BLE)");
        tft.fillRect(0, 185, 170, 30, COLOR_RED);
        tft.setCursor(65, 197); tft.print("KAPAT");
    }
}

void drawMouse() {
    if (!mouseEnabled) return;
    tft.fillTriangle(mouseX, mouseY, mouseX+12, mouseY+12, mouseX, mouseY+16, COLOR_WHITE);
    tft.drawTriangle(mouseX, mouseY, mouseX+12, mouseY+12, mouseX, mouseY+16, COLOR_BLACK);
}

// --- ALT EKRANLAR ---
void drawExplorerInfo() {
    tft.fillScreen(COLOR_WHITE);
    tft.fillRect(0, 0, 320, 35, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10, 12); tft.print("Kynex Web Dosya Yoneticisi");
    tft.setTextColor(COLOR_BLACK);
    tft.setCursor(10, 60); tft.print("Tarayicidan su adrese girin:");
    tft.setTextColor(COLOR_RED); tft.setTextSize(2);
    tft.setCursor(10, 100); tft.print(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "192.168.4.1");
    tft.setTextSize(1); tft.setTextColor(COLOR_BLACK);
    tft.setCursor(10, 200); tft.print("Cikis Icin UZUN BAS");
}

void drawBTScreen() {
    tft.fillScreen(COLOR_WHITE);
    tft.fillRect(0, 0, 320, 35, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10, 12); tft.print("Kynex BLE Yoneticisi");
    tft.setTextColor(COLOR_BLACK);
    tft.setCursor(10, 60); tft.print("S3 Bluetooth Low Energy Aktif.");
    tft.setCursor(10, 90); tft.print("Gorunur Ad: Kynex-Sovereign-BLE");
    tft.setCursor(10, 120); tft.print("Telefondan taratip baglanabilirsiniz.");
    tft.setCursor(10, 220); tft.print("Cikis Icin UZUN BAS");
}

void drawSettingsScreen() {
    tft.fillScreen(COLOR_WHITE);
    tft.fillRect(0, 0, 320, 35, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10, 12); tft.print("WiFi Aglari Taraniyor...");
    
    // HATA BURADA ÇÖZÜLDÜ: Taramayı ekrana yazmadan hemen önce BİR KEZ yapar
    numNetworks = WiFi.scanNetworks();
    
    tft.fillRect(0, 0, 320, 35, WIN10_START);
    tft.setCursor(10, 12); tft.print("Baglanmak Icin Bir Aga Tikla:");

    tft.setTextColor(COLOR_BLACK);
    for(int i=0; i<6; i++) {
        tft.drawFastHLine(0, 40 + (i*30), 320, 0xCE79);
        tft.setCursor(10, 48 + (i*30));
        if(i < numNetworks) {
            networks[i] = WiFi.SSID(i);
            tft.print(networks[i]); tft.print(" ("); tft.print(WiFi.RSSI(i)); tft.print(" dBm)");
        }
    }
}

void drawKynexKeyboard() {
    tft.fillScreen(COLOR_BLACK);
    tft.fillRect(0, 0, 320, 50, 0x2104);
    tft.drawRect(10, 10, 300, 30, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(15, 20); tft.print(kbBuffer);
    
    for(int r=0; r<3; r++) {
        for(int c=0; c<12; c++) {
            int x = c * 26 + 4; 
            int y = 60 + (r * 40);
            tft.drawRect(x, y, 24, 35, 0x4208);
            tft.setCursor(x + 8, y + 12);
            tft.print(kRows[kbMode][r].charAt(c));
        }
    }
    
    int y = 180;
    tft.drawRect(4, y, 40, 35, 0x4208); tft.setCursor(12, y+12); tft.print(kbMode==2 ? "ABC" : "123");
    tft.drawRect(48, y, 40, 35, 0x4208); tft.setCursor(60, y+12); tft.print("SH");
    tft.drawRect(92, y, 100, 35, 0x4208); tft.setCursor(125, y+12); tft.print("SPACE");
    tft.drawRect(196, y, 50, 35, 0x4208); tft.setCursor(210, y+12); tft.print("DEL");
    tft.fillRect(250, y, 66, 35, WIN10_START); tft.drawRect(250, y, 66, 35, COLOR_WHITE); 
    tft.setCursor(275, y+12); tft.print("OK");
}

void drawHardwareTest(JoyData j1, JoyData j2) {
    tft.fillScreen(COLOR_BLACK);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(10,10); tft.print("KYNEX DUAL TEST (Cikis: Uzun Bas)");
    tft.setCursor(10,50); tft.printf("JOY1 X:%04d Y:%04d B:%d", j1.x, j1.y, j1.btn);
    tft.setCursor(10,85); tft.printf("JOY2 X:%04d Y:%04d B:%d", j2.x, j2.y, j2.btn);
}

void drawScreen() {
    if (currentScreen == 0) drawDesktop(-1);
    else if (currentScreen == 2) drawExplorerInfo();
    else if (currentScreen == 3) drawSettingsScreen();
    else if (currentScreen == 4) drawKynexKeyboard();
    else if (currentScreen == 5) drawBTScreen();
}

// --- TIKLAMA YÖNETİCİSİ (KUSURSUZ HITBOX) ---
void handleGlobalClick(int x, int y) {
    playBeep(1800, 20);
    
    // GÖREV ÇUBUĞU (Başlat Menüsü Butonu)
    if (currentScreen == 0 && y > 210 && x < 50) {
        startMenuOpen = !startMenuOpen;
        drawScreen();
        return;
    }
    
    if (currentScreen == 0) {
        if (!startMenuOpen) {
            if (x < 100) {
                if (y > 20 && y < 80) { currentScreen = 2; drawScreen(); }
                else if (y > 80 && y < 145) { 
                    currentScreen = 3; 
                    tft.fillScreen(COLOR_BLACK); tft.setCursor(10,100); tft.setTextColor(COLOR_WHITE); tft.print("Ağlar Taranıyor Lütfen Bekleyin...");
                    drawScreen(); 
                }
                else if (y > 145 && y < 210) { currentScreen = 1; tft.fillScreen(COLOR_BLACK); }
            }
        } else {
            // BAŞLAT MENÜSÜ AÇIKKEN TIKLAMALAR (HATA BURADA ÇÖZÜLDÜ)
            if (x < 170 && y > 40 && y < 215) {
                if (y > 75 && y < 115) { 
                    currentScreen = 3; startMenuOpen = false; 
                    tft.fillScreen(COLOR_BLACK); tft.setCursor(10,100); tft.setTextColor(COLOR_WHITE); tft.print("Ağlar Taranıyor Lütfen Bekleyin...");
                    drawScreen(); 
                } 
                else if (y > 115 && y < 155) { currentScreen = 5; startMenuOpen = false; drawScreen(); } 
                else if (y > 185 && y < 215) { ESP.restart(); } 
            } else {
                startMenuOpen = false; drawScreen(); 
            }
        }
    }
    else if (currentScreen == 3) {
        if (y >= 40 && y < 220) {
            int idx = (y - 40) / 30;
            if (idx < numNetworks) {
                prefs.putString("ssid", networks[idx]); 
                kbBuffer = ""; currentScreen = 4; drawScreen();
            }
        }
    }
    else if (currentScreen == 4) { 
        if (y >= 60 && y < 180) { // Harfler
            int r = (y - 60) / 40; 
            int c = (x - 4) / 26;
            if (c >= 0 && c < 12) { kbBuffer += kRows[kbMode][r].charAt(c); drawScreen(); }
        } 
        else if (y >= 180 && y < 220) { // Fonksiyonlar
            if (x >= 4 && x < 44) { kbMode = (kbMode == 2) ? 0 : 2; drawScreen(); } 
            else if (x >= 48 && x < 88) { kbMode = (kbMode == 0) ? 1 : 0; drawScreen(); } 
            else if (x >= 92 && x < 192) { kbBuffer += " "; drawScreen(); } 
            else if (x >= 196 && x < 246) { if(kbBuffer.length()>0) kbBuffer.remove(kbBuffer.length()-1); drawScreen(); } 
            else if (x >= 250) { 
                prefs.putString("pass", kbBuffer);
                WiFi.begin(prefs.getString("ssid", "").c_str(), kbBuffer.c_str());
                currentScreen = 0; drawScreen();
            }
        }
    }
}

// --- KURULUM ---
void setup() {
    Serial.begin(115200);
    psramInit();
    if(!FFat.begin(true)) FFat.format();
    prefs.begin("kynex", false);

    // HATA ÇÖZÜMÜ: AP VE STA MODU ÇAKIŞMASI ENGELLENDİ
    WiFi.mode(WIFI_AP_STA);

    String savedSSID = prefs.getString("ssid", "");
    String savedPASS = prefs.getString("pass", "");
    if(savedSSID != "") WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

    WiFi.softAP("Kynex-Win10", "*muhammed*krid*");
    
    server.on("/", handleWebRoot);
    server.on("/del", handleFileDelete);
    server.on("/mkdir", handleMkdir);
    server.on("/upload", HTTP_POST, [](){ server.sendHeader("Location", "/"); server.send(303); }, handleFileUpload);
    server.begin();

    BLEDevice::init("Kynex-Sovereign-BLE");
    BLEServer *pServer = BLEDevice::createServer();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x180D));
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY1_SW, INPUT_PULLUP);
    
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(tft_output);
    SPI.begin(TFT_SCK, MISO_PIN, TFT_MOSI, TFT_CS); 
    
    tft.begin(); 
    tft.setRotation(3); 
    ts.begin(); 
    ts.setRotation(3);
    
    playBeep(1000, 200);
    drawScreen();
}

// --- ANA DÖNGÜ (HATA BURADA ÇÖZÜLDÜ) ---
void loop() {
    server.handleClient();
    JoyData j1 = readJoy(JOY1_X, JOY1_Y, JOY1_SW);
    JoyData j2 = readJoy(JOY2_X, JOY2_Y, JOY2_SW);

    // GERİ TUŞU YÖNETİMİ
    if (j1.btn == LOW) {
        if (btnPressStart == 0) btnPressStart = millis();
        if (millis() - btnPressStart > 1500 && !longPressTriggered) {
            longPressTriggered = true;
            playBeep(600, 300); 
            if (currentScreen != 0) { currentScreen = 0; startMenuOpen = false; drawScreen(); } 
            else if (startMenuOpen) { startMenuOpen = false; drawScreen(); }
            else { mouseEnabled = !mouseEnabled; drawScreen(); }
        }
    } else {
        if (btnPressStart != 0) {
            unsigned long dur = millis() - btnPressStart;
            if (!longPressTriggered && dur > 50) { 
                if (mouseEnabled && currentScreen == 0) { handleGlobalClick(mouseX, mouseY); }
                else if (currentScreen == 0) { startMenuOpen = !startMenuOpen; drawScreen(); playBeep(1200, 50); }
            }
            btnPressStart = 0; longPressTriggered = false;
        }
    }

    // EKRAN İÇİ FARE YÖNETİMİ (Sadece masaüstü ve test ekranında ekran yenilenir)
    if (mouseEnabled) {
        int dx = (j1.x - 2048) / 100;
        int dy = (j1.y - 2048) / 100;
        if (abs(dx) > 1 || abs(dy) > 1) {
            mouseX = constrain(mouseX + dx, 0, 310);
            mouseY = constrain(mouseY + dy, 0, 225);
            
            if (millis() - lastAction > 40) {
                if (currentScreen == 0) {
                    int hIdx = -1;
                    if (!startMenuOpen && mouseX < 100) {
                        if (mouseY > 10 && mouseY < 80) hIdx = 1;
                        else if (mouseY > 80 && mouseY < 145) hIdx = 2;
                        else if (mouseY > 145 && mouseY < 220) hIdx = 3;
                    }
                    drawDesktop(hIdx);
                    drawMouse();
                } else if (currentScreen == 1) {
                    // HW Test ekrani
                }
                // Diger ekranlarda fare hareket etse de ekrani basa sarmasin (Wifi taramasi kesilmesin)
                lastAction = millis();
            }
        }
    }

    // DOKUNMATİK EKRAN
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        int tx = map(p.x, 200, 3850, 320, 0); 
        int ty = map(p.y, 240, 3800, 240, 0);
        handleGlobalClick(tx, ty); 
        delay(200); 
    }
    
    // DONANIM TESTİ YENİLEME EKRANI
    if (currentScreen == 1) {
        if (millis() - lastAction > 100) { 
            drawHardwareTest(j1, j2); 
            if(mouseEnabled) drawMouse();
            lastAction = millis(); 
        }
    }
}
