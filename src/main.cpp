/* * KynexOs v137.1 - The Quantum Sovereign (Full Stability Build)
 * Geliştirici: Muhammed (Kynex)
 * Donanım: ESP32-S3 N16R8 (DIO+OPI Hybrid)
 * Özellikler: Fixed Touch Mapping, 30FPS Game Engine, About Links, WiFi Master
 * Hata Düzeltme: drawScreen flicker removed, Touch coordination fixed, Rotation(1) applied
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
int currentScreen = 0; 
bool startMenuOpen = false;
bool mouseEnabled = true;
int mouseX = 160;
int mouseY = 120;
unsigned long btnPressStart = 0;
bool longPressTriggered = false;
unsigned long lastAction = 0;
unsigned long lastTouchTime = 0;

// --- WIFI VE SAAT ---
int numNetworks = 0;
String networks[6];
String kbBuffer = "";
int kbMode = 0; 
const char* ntpServer = "pool.ntp.org";
String currentTime = "00:00";

String kRows[3][3] = {
  {"qwertyuiopgu", "asdfghjklsi-", "zxcvbnmoc.  "},
  {"QWERTYUIOPGU", "ASDFGHJKLSI-", "ZXCVBNMOC.  "},
  {"1234567890-=", "!@#$%&*()_+/", "[]{};:'\",.<>?"}
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
unsigned long lastPongMove = 0;

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

void playBeep(int f, int d) { tone(SPEAKER_PIN, f, d); }

JoyData readJoy(int px, int py, int psw) {
    JoyData d; d.x = analogRead(px); d.y = analogRead(py); d.btn = (digitalRead(psw) == LOW);
    return d;
}

// --- WEB FM VE DOSYA İŞLEMLERİ ---
void handleWebRoot() {
    String h = "<html><head><meta charset='UTF-8'></head><body><h1>Kynex Explorer</h1>";
    File root = FFat.open("/");
    File file = root.openNextFile();
    while(file) {
        h += "<div>" + String(file.name()) + " <a href='/del?f=" + String(file.name()) + "'>[Sil]</a></div>";
        file = root.openNextFile();
    }
    h += "<br><form action='/upload' method='POST' enctype='multipart/form-data'><input type='file' name='u'><input type='submit' value='Yukle'></form></body></html>";
    server.send(200, "text/html", h);
}

void handleFileDelete() { if(server.hasArg("f")) FFat.remove("/" + server.arg("f")); server.sendHeader("Location", "/"); server.send(303); }
void handleMkdir() { if(server.hasArg("d")) FFat.mkdir("/" + server.arg("d")); server.sendHeader("Location", "/"); server.send(303); }
void handleFileUpload() {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START) fsUploadFile = FFat.open("/" + upload.filename, FILE_WRITE);
    else if(upload.status == UPLOAD_FILE_WRITE && fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
    else if(upload.status == UPLOAD_FILE_END && fsUploadFile) fsUploadFile.close();
}

// --- SİSTEM BİLEŞENLERİ ---
void updateClock() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return;
    char timeStr[6]; strftime(timeStr, 6, "%H:%M", &timeinfo);
    currentTime = String(timeStr);
}

void drawTaskbar() {
    tft.fillRect(0, 215, 320, 25, WIN10_TASKBAR);
    tft.fillRect(0, 215, 45, 25, startMenuOpen ? WIN10_HOVER : WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(270, 225); tft.print(currentTime);
}

void renderIcon(int x, int y, const char* txt, uint16_t c, bool h) {
    if (h) tft.drawRoundRect(x-5, y-5, 52, 52, 6, COLOR_WHITE);
    tft.fillRect(x, y, 40, 32, h ? WIN10_HOVER : c);
    tft.drawRect(x+2, y+2, 36, 28, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setTextSize(1); tft.setCursor(x - 5, y + 40); tft.print(txt);
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
        tft.setCursor(15, 60); tft.print("Kynex Zenith OS");
        tft.fillRect(0, 185, 170, 30, COLOR_RED); tft.setCursor(65, 197); tft.print("RST");
    }
}

void drawAboutScreen() {
    tft.fillScreen(COLOR_WHITE); tft.fillRect(0,0,320,35, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10,12); tft.print("Sistem Bilgileri");
    tft.setTextColor(COLOR_BLACK);
    tft.setCursor(10, 60); tft.print("Cihaz: Kynex Sovereign S3");
    tft.setCursor(10, 80); tft.print("Versiyon: v137.1 Quantum");
    tft.setCursor(10, 110); tft.print("WiFi: "); tft.print(WiFi.SSID());
    tft.setCursor(10, 140); tft.setTextColor(COLOR_BLUE); tft.print("FM: http://"); tft.print(WiFi.localIP().toString());
    tft.setCursor(10, 160); tft.setTextColor(COLOR_RED); tft.print("AP: http://192.168.4.1");
}

void drawExplorerInfo() {
    tft.fillScreen(COLOR_WHITE); tft.fillRect(0, 0, 320, 35, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10, 12); tft.print("Web File Manager");
    tft.setTextColor(COLOR_BLACK); tft.setCursor(10, 60); tft.print("Link: http://"); tft.print(WiFi.localIP().toString());
}

void drawSettingsScreen() {
    tft.fillScreen(COLOR_WHITE); tft.fillRect(0,0,320,35, WIN10_START);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10,12); tft.print("WiFi Aglari");
    numNetworks = WiFi.scanNetworks();
    tft.setTextColor(COLOR_BLACK);
    for(int i=0; i<min(numNetworks,6); i++) {
        networks[i] = WiFi.SSID(i);
        tft.drawRect(5, 45+(i*28), 310, 25, 0xCE79); tft.setCursor(15, 53+(i*28)); tft.print(networks[i]);
    }
}

void drawKynexKeyboard() {
    tft.fillScreen(COLOR_BLACK); tft.drawRect(10, 10, 300, 35, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(15, 22); tft.print(kbBuffer);
    for(int r=0; r<3; r++) {
        for(int c=0; c<12; c++) {
            tft.drawRect(c*26+4, 60+r*40, 24, 35, 0x4208);
            tft.setCursor(c*26+10, 72+r*40); tft.print(kRows[kbMode][r].charAt(c));
        }
    }
    tft.fillRect(250, 180, 65, 35, WIN10_START); tft.setCursor(270, 192); tft.print("OK");
}

void drawPaintApp() {
    tft.fillScreen(COLOR_WHITE); tft.fillRect(0, 0, 320, 35, 0xCE79);
    tft.fillRect(5, 5, 25, 25, COLOR_RED); tft.fillRect(35, 5, 25, 25, COLOR_GREEN);
    tft.fillRect(65, 5, 25, 25, COLOR_BLUE); tft.drawRect(130, 5, 60, 25, COLOR_BLACK);
    tft.setCursor(135, 12); tft.setTextColor(COLOR_BLACK); tft.print("SILGI");
}

void drawHardwareTest(JoyData j1, JoyData j2) {
    tft.fillScreen(COLOR_BLACK); tft.setTextColor(COLOR_WHITE);
    tft.setCursor(10,10); tft.print("HW TEST");
    tft.setCursor(10,50); tft.printf("J1 X:%d Y:%d B:%d", j1.x, j1.y, j1.btn);
    tft.setCursor(10,80); tft.printf("J2 X:%d Y:%d B:%d", j2.x, j2.y, j2.btn);
}

// --- OYUNLAR ---
void spawnApple() { apple.x = random(1, 15) * 20; apple.y = random(3, 10) * 20; }
void drawSnakeGame() {
    tft.fillScreen(COLOR_BLACK); tft.fillRect(0,0,320,25,0x2104);
    for(int i=0; i<snakeLen; i++) tft.fillRect(snake[i].x, snake[i].y, 18, 18, COLOR_GREEN);
    tft.fillRect(apple.x, apple.y, 18, 18, COLOR_RED);
}
void updateSnake(JoyData j) {
    if (millis() - lastSnakeMove < 150) return;
    if (j.x < 1000 && snakeDir != 1) snakeDir = 3; else if (j.x > 3000 && snakeDir != 3) snakeDir = 1;
    else if (j.y < 1000 && snakeDir != 2) snakeDir = 0; else if (j.y > 3000 && snakeDir != 0) snakeDir = 2;
    int otX = snake[snakeLen-1].x; int otY = snake[snakeLen-1].y;
    for(int i=snakeLen-1; i>0; i--) snake[i] = snake[i-1];
    if(snakeDir==0) snake[0].y-=20; else if(snakeDir==1) snake[0].x+=20;
    else if(snakeDir==2) snake[0].y+=20; else if(snakeDir==3) snake[0].x-=20;
    if(snake[0].x<0 || snake[0].x>300 || snake[0].y<40 || snake[0].y>220) { snakeLen=3; snake[0]={160,120}; drawSnakeGame(); }
    if(snake[0].x==apple.x && snake[0].y==apple.y) { snakeLen++; spawnApple(); tft.fillRect(apple.x, apple.y, 18, 18, COLOR_RED); }
    else tft.fillRect(otX, otY, 18, 18, COLOR_BLACK);
    tft.fillRect(snake[0].x, snake[0].y, 18, 18, COLOR_GREEN); lastSnakeMove = millis();
}
void drawPongGame() { tft.fillScreen(COLOR_BLACK); tft.fillRect(10, p1Y, 10, 50, COLOR_WHITE); tft.fillRect(300, p2Y, 10, 50, COLOR_WHITE); tft.fillCircle(bX, bY, 5, COLOR_RED); }
void updatePong(JoyData j1, JoyData j2) {
    if (millis() - lastPongMove < 35) return;
    int op1 = p1Y, op2 = p2Y, obx = bX, oby = bY;
    p1Y = constrain(p1Y + (j1.y-2048)/80, 0, 190); p2Y = constrain(p2Y + (j2.y-2048)/80, 0, 190);
    bX += bDX; bY += bDY;
    if(bY<=5 || bY>=235) bDY*=-1;
    if((bX<=25 && bY>=p1Y && bY<=p1Y+50) || (bX>=295 && bY>=p2Y && bY<=p2Y+50)) bDX*=-1;
    if(bX<0 || bX>320) { bX=160; bY=120; drawPongGame(); return; }
    tft.fillRect(10, op1, 10, 50, COLOR_BLACK); tft.fillRect(300, op2, 10, 50, COLOR_BLACK);
    tft.fillCircle(obx, oby, 5, COLOR_BLACK); tft.fillRect(10, p1Y, 10, 50, COLOR_WHITE);
    tft.fillRect(300, p2Y, 10, 50, COLOR_WHITE); tft.fillCircle(bX, bY, 5, COLOR_RED); lastPongMove = millis();
}

// --- MERKEZİ SİSTEM ---
void drawScreen() {
    if (currentScreen == 0) drawDesktop(-1);
    else if (currentScreen == 2) drawExplorerInfo();
    else if (currentScreen == 3) drawSettingsScreen();
    else if (currentScreen == 4) drawKynexKeyboard();
    else if (currentScreen == 6) drawSnakeGame();
    else if (currentScreen == 7) drawPongGame();
    else if (currentScreen == 8) drawPaintApp();
    else if (currentScreen == 9) drawAboutScreen();
}

void handleGlobalClick(int x, int y) {
    playBeep(1800, 20);
    if (currentScreen == 0) {
        if (y > 210 && x < 50) { startMenuOpen = true; drawScreen(); return; }
        if (!startMenuOpen) {
            if (x < 80) {
                if (y > 20 && y < 80) { currentScreen = 2; drawScreen(); }
                else if (y > 85 && y < 145) { currentScreen = 3; drawScreen(); }
                else if (y > 150 && y < 210) { currentScreen = 9; drawScreen(); }
            } else if (x > 80 && x < 160) {
                if (y > 20 && y < 80) { currentScreen = 8; drawScreen(); }
                else if (y > 85 && y < 145) { currentScreen = 6; drawScreen(); }
                else if (y > 150 && y < 210) { currentScreen = 7; drawScreen(); }
            }
        } else { if (y > 185 && x < 170) ESP.restart(); else { startMenuOpen = false; drawScreen(); } }
    } else if (currentScreen == 3 && y > 40) {
        int idx = (y - 45) / 28; if (idx >= 0 && idx < numNetworks) { prefs.putString("sid_t", networks[idx]); kbBuffer = ""; currentScreen = 4; drawScreen(); }
    } else if (currentScreen == 4) {
        if (y > 60 && y < 180) { int r = (y-60)/40, c = (x-4)/26; if (c < 12) { kbBuffer += kRows[kbMode][r].charAt(c); drawKynexKeyboard(); } }
        else if (y > 180 && x > 250) { prefs.putString("ssid", prefs.getString("sid_t")); prefs.putString("pass", kbBuffer); WiFi.begin(prefs.getString("ssid").c_str(), kbBuffer.c_str()); currentScreen = 0; drawScreen(); }
    }
}

void setup() {
    Serial.begin(115200); psramInit(); FFat.begin(true); prefs.begin("kynex", false);
    WiFi.mode(WIFI_AP_STA); WiFi.softAP("KynexOs-Win10", "*muhammed*krid*");
    server.on("/", handleWebRoot); server.begin();
    SPI.begin(TFT_SCK, MISO_PIN, TFT_MOSI, TFT_CS); 
    tft.begin(); tft.setRotation(1); ts.begin(SPI); ts.setRotation(1);
    playBeep(1000, 500); drawScreen();
}

void loop() {
    server.handleClient(); updateClock();
    JoyData j1 = readJoy(JOY1_X, JOY1_Y, JOY1_SW);
    JoyData j2 = readJoy(JOY2_X, JOY2_Y, JOY2_SW);

    if (j1.btn) {
        if (btnPressStart == 0) btnPressStart = millis();
        if (millis() - btnPressStart > 1500 && !longPressTriggered) { longPressTriggered = true; playBeep(600, 200); currentScreen = 0; startMenuOpen = false; drawScreen(); }
    } else {
        if (btnPressStart != 0 && !longPressTriggered) { if (mouseEnabled && currentScreen == 0) handleGlobalClick(mouseX, mouseY); else if (currentScreen == 0) startMenuOpen = !startMenuOpen; else handleGlobalClick(mouseX, mouseY); }
        btnPressStart = 0; longPressTriggered = false;
    }

    if(currentScreen == 6) updateSnake(j1);
    else if(currentScreen == 7) updatePong(j1, j2);
    else if (currentScreen == 0 && mouseEnabled) {
        int dx = (j1.x - 2048) / 100, dy = (j1.y - 2048) / 100;
        if (abs(dx) > 1 || abs(dy) > 1) { mouseX = constrain(mouseX + dx, 0, 310); mouseY = constrain(mouseY + dy, 0, 225); if (millis() - lastAction > 50) { drawScreen(); tft.fillTriangle(mouseX, mouseY, mouseX+10, mouseY+10, mouseX, mouseY+14, COLOR_WHITE); lastAction = millis(); } }
    }

    if (ts.touched() && millis() - lastTouchTime > 400) {
        TS_Point p = ts.getPoint();
        int tx = map(p.x, 200, 3850, 320, 0); int ty = map(p.y, 240, 3800, 240, 0);
        handleGlobalClick(tx, ty); lastTouchTime = millis();
    }
}
