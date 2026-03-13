/* * KynexOs v136.0 - The Zenith Core (Universal Click & Anti-Spam)
 * Geliştirici: Muhammed (Kynex)
 * Donanım: ESP32-S3 N16R8 (DIO+OPI Hybrid)
 * Özellikler: Universal Joystick Click, Touch Anti-Spam, NTP Clock, TR QWERTY, Games, Web FM
 * Hata Düzeltme: WiFi Keyboard Trigger Fixed, Rotation(3) Fixed, Startup Sound Restored
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
int currentScreen = 0; // 0:Desktop, 1:HW Test, 2:Explorer Info, 3:WiFi, 4:Klavye, 6:Snake, 7:Pong, 8:Paint, 9:About
bool startMenuOpen = false;
bool mouseEnabled = true;
int mouseX = 160;
int mouseY = 120;

unsigned long btnPressStart = 0;
bool longPressTriggered = false;
unsigned long lastAction = 0;
unsigned long lastTouchTime = 0; // Anti-Spam Dokunmatik icin eklendi

// --- WIFI VE SAAT ---
int numNetworks = 0;
String networks[6];
String kbBuffer = "";
int kbMode = 0; 
const char* ntpServer = "pool.ntp.org";
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
int bX = 160, bY = 120, bDX = 5, bDY = 5;

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
void handleGlobalClick(int x, int y);
void spawnApple();
void drawSnakeGame();
void updateSnake(JoyData j);
void drawPongGame();
void updatePong(JoyData j1, JoyData j2);
void drawHardwareTest(JoyData j1, JoyData j2);

// --- JPG DECODER ---
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

// AÇILIŞ VE TIKLAMA SESİ (GÜVENLİ)
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
    h += "<h1>Kynex Sovereign Web FM</h1><hr>";
    File root = FFat.open("/");
    File file = root.openNextFile();
    while(file) {
        h += "<div>" + String(file.name()) + " <a href='/del?f=" + String(file.name()) + "'>[Sil]</a></div>";
        file = root.openNextFile();
    }
    h += "<br><form action='/upload' method='POST' enctype='multipart/form-data'><input type='file' name='u'><input type='submit' value='Yukle'></form></body></html>";
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

// --- SAAT GÜNCELLEME ---
void updateClock() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){ return; }
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
    tft.setTextColor(COLOR_WHITE); tft.setTextSize(1);
    tft.setCursor(x - 5, y + 40); tft.print(txt);
}

void drawTaskbar() {
    tft.fillRect(0, 215, 320, 25, WIN10_TASKBAR);
    tft.fillRect(0, 215, 45, 25, startMenuOpen ? WIN10_HOVER : WIN10_START);
    // Basit Logo
    tft.drawRect(15, 220, 15, 15, COLOR_WHITE);
    tft.fillRect(15, 220, 6, 6, COLOR_WHITE); tft.fillRect(24, 220, 6, 6, COLOR_WHITE);
    tft.fillRect(15, 229, 6, 6, COLOR_WHITE); tft.fillRect(24, 229, 6, 6, COLOR_WHITE);
    
    tft.setTextColor(COLOR_WHITE); tft.setCursor(270, 225);
    tft.print(currentTime);
}

void drawDesktop(int hIdx) {
    tft.fillRect(0, 0, 320, 240, COLOR_BLACK); 
    size_t wlen = wallpaper_jpg_end - wallpaper_jpg_start;
    TJpgDec.drawJpg(0, 0, wallpaper_jpg_start, wlen); 
    
    renderIcon(20, 20, "Files", COLOR_ICON_PC, hIdx == 1);
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
        tft.setCursor(15, 60); tft.print("Kynex Zenith S3");
        tft.drawFastHLine(10, 75, 150, 0x4208);
        tft.setCursor(15, 95); tft.print("> WiFi Unut");
        tft.setCursor(15, 125); tft.print("> Sistemi Kapat");
        tft.fillRect(0, 185, 170, 30, COLOR_RED);
        tft.setCursor(65, 197); tft.print("RST");
    }
}

void drawExplorerInfo() {
    tft.fillScreen(COLOR_WHITE);
    tft.fillRect(0, 0, 320, 35, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10, 12); tft.print("Kynex Web File Manager");
    tft.setTextColor(COLOR_BLACK);
    tft.setCursor(10, 60); tft.print("Yerel Ag IP:");
    tft.setTextColor(COLOR_BLUE); tft.setCursor(10, 80); tft.print("http://"); tft.print(WiFi.localIP().toString());
    tft.setTextColor(COLOR_BLACK); tft.setCursor(10, 110); tft.print("Hotspot Modu (Sifre: *muhammed*krid*):");
    tft.setTextColor(COLOR_RED); tft.setCursor(10, 130); tft.print("http://192.168.4.1");
    tft.setTextColor(COLOR_BLACK); tft.setCursor(10, 210); tft.print("Geri: Sol Joy Uzun Bas");
}

void drawAboutScreen() {
    tft.fillScreen(COLOR_WHITE);
    tft.fillRect(0,0,320,35, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10,12); tft.print("Sistem Bilgileri");
    tft.setTextColor(COLOR_BLACK);
    tft.setCursor(10, 60); tft.print("Cihaz: Kynex Sovereign S3");
    tft.setCursor(10, 80); tft.print("Versiyon: v136.0 Zenith");
    tft.setCursor(10, 110); tft.print("WiFi Ag: "); tft.print(WiFi.SSID());
    
    // LINKLER EKLENDİ
    tft.setCursor(10, 140); tft.setTextColor(COLOR_BLUE);
    tft.print("FM Link: http://"); tft.print(WiFi.localIP().toString());
    tft.setCursor(10, 160); tft.setTextColor(COLOR_RED);
    tft.print("AP Link: http://192.168.4.1");
    
    tft.setTextColor(COLOR_BLACK); tft.setCursor(10, 210); tft.print("Geri: Sol Joy Uzun Bas");
}

void drawSettingsScreen() {
    tft.fillScreen(COLOR_WHITE);
    tft.fillRect(0,0,320,35, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10,12); tft.print("Aglari Taraniyor Lutfen Bekleyin...");
    
    numNetworks = WiFi.scanNetworks();
    
    tft.fillRect(0,0,320,35, WIN10_START); tft.setCursor(10,12); tft.print("Baglanilacak Agi Seciniz:");
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
    
    // Ozel Tuslar
    int y = 180;
    tft.drawRect(4, y, 40, 35, 0x4208); tft.setCursor(12, y+12); tft.print(kbMode==2 ? "ABC" : "123");
    tft.drawRect(48, y, 40, 35, 0x4208); tft.setCursor(60, y+12); tft.print("SH");
    tft.drawRect(92, y, 100, 35, 0x4208); tft.setCursor(125, y+12); tft.print("SPACE");
    tft.drawRect(196, y, 50, 35, 0x4208); tft.setCursor(210, y+12); tft.print("DEL");
    tft.fillRect(250, y, 66, 35, WIN10_START); tft.drawRect(250, y, 66, 35, COLOR_WHITE); 
    tft.setCursor(275, y+12); tft.print("OK");
}

void drawPaintApp() {
    tft.fillScreen(COLOR_WHITE);
    tft.fillRect(0, 0, 320, 35, 0xCE79);
    tft.fillRect(5, 5, 25, 25, COLOR_RED); tft.fillRect(35, 5, 25, 25, COLOR_GREEN);
    tft.fillRect(65, 5, 25, 25, COLOR_BLUE); tft.fillRect(95, 5, 25, 25, COLOR_YELLOW);
    tft.drawRect(130, 5, 60, 25, COLOR_BLACK); tft.setCursor(135, 12); tft.setTextColor(COLOR_BLACK); tft.print("SILGI");
    tft.drawRect(200, 5, 60, 25, COLOR_RED); tft.setCursor(205, 12); tft.setTextColor(COLOR_RED); tft.print("TEMIZ");
}

void drawHardwareTest(JoyData j1, JoyData j2) {
    tft.fillScreen(COLOR_BLACK);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(10,10); tft.print("KYNEX HW TEST (Cikis: Uzun Bas)");
    tft.setCursor(10,50); tft.printf("JOY1 X:%04d Y:%04d B:%d", j1.x, j1.y, j1.btn);
    tft.setCursor(10,85); tft.printf("JOY2 X:%04d Y:%04d B:%d", j2.x, j2.y, j2.btn);
}

// --- OYUN GÖVDELERİ ---
void spawnApple() { apple.x = random(1, 15) * 20; apple.y = random(3, 10) * 20; }

void drawSnakeGame() {
    tft.fillScreen(COLOR_BLACK);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(5, 5); tft.print("SNAKE - Skor: "); tft.print(snakeLen-3);
    for(int i=0; i<snakeLen; i++) tft.fillRect(snake[i].x, snake[i].y, 18, 18, COLOR_GREEN);
    tft.fillRect(apple.x, apple.y, 18, 18, COLOR_RED);
}

void updateSnake(JoyData j) {
    if (millis() - lastSnakeMove < 150) return;
    if (j.x < 1000) snakeDir = 3; else if (j.x > 3000) snakeDir = 1;
    else if (j.y < 1000) snakeDir = 0; else if (j.y > 3000) snakeDir = 2;
    for(int i=snakeLen-1; i>0; i--) snake[i] = snake[i-1];
    if(snakeDir==0) snake[0].y-=20; else if(snakeDir==1) snake[0].x+=20;
    else if(snakeDir==2) snake[0].y+=20; else if(snakeDir==3) snake[0].x-=20;
    if(snake[0].x<0 || snake[0].x>300 || snake[0].y<40 || snake[0].y>220) { snakeLen=3; snake[0]={160,120}; spawnApple(); }
    if(snake[0].x==apple.x && snake[0].y==apple.y) { snakeLen++; spawnApple(); playBeep(2000, 10); }
    drawSnakeGame(); lastSnakeMove = millis();
}

void drawPongGame() {
    tft.fillScreen(COLOR_BLACK);
    tft.fillRect(10, p1Y, 10, 50, COLOR_WHITE);
    tft.fillRect(300, p2Y, 10, 50, COLOR_WHITE);
    tft.fillCircle(bX, bY, 5, COLOR_RED);
}

void updatePong(JoyData j1, JoyData j2) {
    p1Y = constrain(p1Y + (j1.y-2048)/80, 0, 190);
    p2Y = constrain(p2Y + (j2.y-2048)/80, 0, 190);
    bX += bDX; bY += bDY;
    if(bY<=5 || bY>=235) bDY*=-1;
    if(bX<=25 && bY>=p1Y && bY<=p1Y+50) bDX*=-1;
    if(bX>=295 && bY>=p2Y && bY<=p2Y+50) bDX*=-1;
    if(bX<0 || bX>320) { bX=160; bY=120; bDX*=-1; playBeep(400, 50); }
    drawPongGame();
}

void drawScreen() {
    if (currentScreen == 0) drawDesktop(-1);
    else if (currentScreen == 1) { tft.fillScreen(COLOR_BLACK); drawHardwareTest(readJoy(JOY1_X, JOY1_Y, JOY1_SW), readJoy(JOY2_X, JOY2_Y, JOY2_SW)); }
    else if (currentScreen == 2) drawExplorerInfo();
    else if (currentScreen == 3) drawSettingsScreen();
    else if (currentScreen == 4) drawKynexKeyboard();
    else if (currentScreen == 6) { snakeLen=3; snake[0]={160,120}; spawnApple(); drawSnakeGame(); }
    else if (currentScreen == 7) drawPongGame();
    else if (currentScreen == 8) drawPaintApp();
    else if (currentScreen == 9) drawAboutScreen();
}

// --- EVRENSEL TIKLAMA YÖNETİCİSİ (HATA BURADA ÇÖZÜLDÜ) ---
void handleGlobalClick(int x, int y) {
    playBeep(1800, 20);
    
    if (currentScreen == 0) { // Masaustu
        if (!startMenuOpen) {
            // ALT GOREV CUBUGU BASLAT BUTONU
            if(y > 210 && x < 50) { startMenuOpen = true; drawScreen(); return; }
            
            if (x < 80) {
                if (y > 20 && y < 80) { currentScreen = 2; drawScreen(); }
                else if (y > 85 && y < 145) { currentScreen = 3; drawScreen(); }
                else if (y > 150 && y < 210) { currentScreen = 9; drawScreen(); }
            } else if (x > 80 && x < 160) {
                if (y > 20 && y < 80) { currentScreen = 8; drawScreen(); }
                else if (y > 85 && y < 145) { currentScreen = 6; drawScreen(); }
                else if (y > 150 && y < 210) { currentScreen = 7; drawScreen(); }
            }
        } else { // Baslat Menusu Aciksa
            if (x < 170) {
                if (y > 80 && y < 115) { prefs.remove("ssid"); prefs.remove("pass"); WiFi.disconnect(); startMenuOpen=false; drawScreen(); }
                if (y > 185) ESP.restart();
            } else { startMenuOpen = false; drawScreen(); }
        }
    } 
    else if (currentScreen == 3) { // WiFi Secim Ekrani (KLAVYE TETIKLEYICI)
        if (y > 40) {
            int idx = (y - 45) / 28;
            if (idx >= 0 && idx < numNetworks) { 
                prefs.putString("ssid_tmp", networks[idx]); 
                kbBuffer = ""; currentScreen = 4; drawScreen(); 
            }
        }
    } 
    else if (currentScreen == 4) { // Klavye Ekrani
        if (y >= 60 && y < 180) {
            int r = (y-60)/40; int c = (x-4)/26;
            if (c >= 0 && c < 12) { kbBuffer += kRows[kbMode][r].charAt(c); drawKynexKeyboard(); }
        } else if (y >= 180 && y < 220) {
            if (x >= 4 && x < 44) { kbMode = (kbMode == 2) ? 0 : 2; drawKynexKeyboard(); } 
            else if (x >= 48 && x < 88) { kbMode = (kbMode == 0) ? 1 : 0; drawKynexKeyboard(); } 
            else if (x >= 92 && x < 192) { kbBuffer += " "; drawKynexKeyboard(); } 
            else if (x >= 196 && x < 246) { if(kbBuffer.length()>0) kbBuffer.remove(kbBuffer.length()-1); drawKynexKeyboard(); } 
            else if (x >= 250) { 
                prefs.putString("ssid", prefs.getString("ssid_tmp")); prefs.putString("pass", kbBuffer);
                WiFi.begin(prefs.getString("ssid").c_str(), kbBuffer.c_str());
                currentScreen = 0; drawScreen();
            }
        }
    } 
    else if (currentScreen == 8) { // Paint
        if (y < 35) {
            if (x < 30) paintColor = COLOR_RED; else if (x < 60) paintColor = COLOR_GREEN;
            else if (x < 90) paintColor = COLOR_BLUE; else if (x < 120) paintColor = COLOR_YELLOW;
            else if (x < 190) paintColor = COLOR_WHITE; else if (x < 260) drawPaintApp();
        } else { tft.fillCircle(x, y, 2, paintColor); }
    }
}

// --- SETUP ---
void setup() {
    Serial.begin(115200); psramInit(); FFat.begin(true);
    prefs.begin("kynex", false);
    
    // Çift Mod (AP + STA) - İnternet kilitlenmesini çözer
    WiFi.mode(WIFI_AP_STA);
    
    String s = prefs.getString("ssid", "");
    String p = prefs.getString("pass", "");
    if(s != "") WiFi.begin(s.c_str(), p.c_str());

    WiFi.softAP("KynexOs-Win10", "*muhammed*krid*");
    server.on("/", handleWebRoot); 
    server.on("/del", handleFileDelete);
    server.on("/mkdir", handleMkdir);
    server.on("/upload", HTTP_POST, [](){ server.sendHeader("Location", "/"); server.send(303); }, handleFileUpload);
    server.begin();
    
    configTime(10800, 0, ntpServer);

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY1_SW, INPUT_PULLUP); pinMode(JOY2_SW, INPUT_PULLUP);
    
    TJpgDec.setJpgScale(1); TJpgDec.setCallback(tft_output);
    SPI.begin(TFT_SCK, MISO_PIN, TFT_MOSI, TFT_CS); 
    
    // EKRAN TAM 180 DERECE DÖNDÜRÜLDÜ
    tft.begin(); tft.setRotation(3); 
    ts.begin(); ts.setRotation(3);
    
    // GÜÇLÜ AÇILIŞ SESİ
    playBeep(1000, 400); 
    delay(400);
    playBeep(1500, 200);
    
    drawScreen();
}

// --- LOOP ---
void loop() {
    server.handleClient();
    if(WiFi.status() == WL_CONNECTED) updateClock();
    
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
            // EVRENSEL TIKLAMA: Hangi ekranda olursa olsun joystick ile tiklanabilir
            if (!longPressTriggered && dur > 50) {
                if (mouseEnabled) { handleGlobalClick(mouseX, mouseY); }
                else if (currentScreen == 0) { startMenuOpen = !startMenuOpen; drawScreen(); }
            }
            btnPressStart = 0; longPressTriggered = false;
        }
    }

    // OYUN MOTORLARI
    if(currentScreen == 6) updateSnake(j1);
    else if(currentScreen == 7) updatePong(j1, j2);
    
    // FARE MOTORU
    else if (mouseEnabled && currentScreen != 1) { // Test ekrani haric fare goster
        int dx = (j1.x - 2048) / 100; int dy = (j1.y - 2048) / 100;
        if (abs(dx) > 1 || abs(dy) > 1) {
            mouseX = constrain(mouseX + dx, 0, 310); mouseY = constrain(mouseY + dy, 0, 225);
            if (millis() - lastAction > 50) { 
                if(currentScreen == 0) { // Sadece masaüstünde ikon hover kontrolu
                    int hIdx = -1;
                    if (!startMenuOpen) {
                        if (mouseX < 80) {
                            if (mouseY > 20 && mouseY < 80) hIdx = 1;
                            else if (mouseY > 85 && mouseY < 145) hIdx = 2;
                            else if (mouseY > 150 && mouseY < 210) hIdx = 3;
                        } else if (mouseX > 80 && mouseX < 160) {
                            if (mouseY > 20 && mouseY < 80) hIdx = 4;
                            else if (mouseY > 85 && mouseY < 145) hIdx = 5;
                            else if (mouseY > 150 && mouseY < 210) hIdx = 6;
                        }
                    }
                    drawDesktop(hIdx);
                } else if (currentScreen != 8) { // Paint haric ekrani yenile
                    drawScreen();
                }
                
                if (currentScreen == 8) { // Paint uzerinde fare ile cizim
                    tft.fillCircle(mouseX, mouseY, 2, paintColor);
                } else { // Klasik Fare
                    tft.fillTriangle(mouseX, mouseY, mouseX+10, mouseY+10, mouseX, mouseY+14, COLOR_WHITE);
                }
                lastAction = millis(); 
            }
        }
    }

    // DOKUNMATIK MOTORU VE ANTI-SPAM (Bip dongusu cozumu)
    if (ts.touched() && millis() - lastTouchTime > 400) {
        TS_Point p = ts.getPoint();
        int tx = map(p.x, 200, 3850, 320, 0); int ty = map(p.y, 240, 3800, 240, 0);
        handleGlobalClick(tx, ty); 
        lastTouchTime = millis();
    }
}
