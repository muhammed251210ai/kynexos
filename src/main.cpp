/* * KynexOs v131.0 - The Sovereign Gaming Edition (Complete OS & Game Core)
 * Geliştirici: Muhammed (Kynex)
 * Donanım: ESP32-S3 N16R8 (DIO+OPI Hybrid)
 * Özellikler: Snake Game, Pong 2P, WiFi Scanner, TR QWERTY, Web FM, BLE, FFat
 * Hata Düzeltme: Exit from games via Joy1 Long Press, Redraw flickering reduced, WiFi Scan fix
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
int currentScreen = 0; // 0: Masaüstü, 1: HW Test, 2: Web FM, 3: WiFi Scanner, 4: Klavye, 5: BT Scanner, 6: Snake, 7: Pong
bool startMenuOpen = false;
bool mouseEnabled = true;
int mouseX = 160;
int mouseY = 120;

unsigned long btnPressStart = 0;
bool longPressTriggered = false;
unsigned long lastAction = 0;

// --- WIFI VE KLAVYE DEĞİŞKENLERİ ---
int numNetworks = 0;
String networks[6];
String kbBuffer = "";
int kbMode = 0; 
String kRows[3][3] = {
  {"qwertyuiopgu", "asdfghjklsi-", "zxcvbnmoc.  "},
  {"QWERTYUIOPGU", "ASDFGHJKLSI-", "ZXCVBNMOC.  "},
  {"1234567890-=", "!@#$%&*()_+/", "[]{};:'\",.<>"}
};

// --- SNAKE GAME DATA ---
#define SNAKE_MAX 64
struct Point { int x, y; };
Point snake[SNAKE_MAX];
int snakeLen = 3;
Point apple;
int snakeDir = 0; // 0: UP, 1: RIGHT, 2: DOWN, 3: LEFT
unsigned long lastSnakeMove = 0;
int snakeSpeed = 150;

// --- PONG GAME DATA ---
struct Paddle { int x, y; int w, h; };
Paddle p1 = {10, 100, 10, 50};
Paddle p2 = {300, 100, 10, 50};
int ballX=160, ballY=120, ballDX=5, ballDY=5;

// --- NESNELER ---
Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS); 
WebServer server(80);
Preferences prefs;
File fsUploadFile; 

// --- JOYSTICK STRUCT ---
struct JoyData { int x, y; bool btn; };

// --- FONKSİYON PROTOTİPLERİ ---
void drawScreen();
void drawKynexKeyboard();
void drawExplorerInfo();
void drawSettingsScreen();
void drawBTScreen();
void drawDesktop(int hoverIdx);
void drawTaskbar();
void drawMouse();
void drawHardwareTest(JoyData j1, JoyData j2);
void spawnApple();
void drawSnakeGame();
void updateSnake(JoyData j);
void drawPongGame();
void updatePong(JoyData j1, JoyData j2);

// --- JPG DECODER ---
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

// --- DONANIM KONTROL ---
void playBeep(int f, int d) { tone(SPEAKER_PIN, f, d); }

JoyData readJoy(int px, int py, int psw) {
    JoyData d; 
    d.x = analogRead(px); 
    d.y = analogRead(py); 
    d.btn = (digitalRead(psw) == LOW); 
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
    while(file) {
        h += "<div style='display:flex; justify-content:space-between; padding:10px; border-bottom:1px solid #eee;'>";
        h += "<span>" + String(file.name()) + "</span>";
        h += "<span><a href='/del?f=" + String(file.name()) + "' style='color:red;'>[Sil]</a></span></div>";
        file = root.openNextFile();
    }
    h += "<br><form action='/upload' method='POST' enctype='multipart/form-data'><input type='file' name='u'> <input type='submit' value='Yukle'></form>";
    h += "</div></body></html>";
    server.send(200, "text/html", h);
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
    tft.setTextColor(COLOR_WHITE); tft.setCursor(x - 10, y + 40); tft.print(txt);
}

void drawTaskbar() {
    tft.fillRect(0, 215, 320, 25, WIN10_TASKBAR);
    tft.fillRect(0, 215, 45, 25, startMenuOpen ? WIN10_HOVER : WIN10_START);
    drawWindowsLogo(16, 221, 12, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(240, 225);
    tft.print(WiFi.status() == WL_CONNECTED ? "BAĞLI" : "KOPUK");
}

void drawDesktop(int hoverIdx) {
    tft.fillRect(0, 0, 320, 240, COLOR_BLACK); 
    size_t wallpaper_len = wallpaper_jpg_end - wallpaper_jpg_start;
    TJpgDec.drawJpg(0, 0, wallpaper_jpg_start, wallpaper_len); 
    
    renderIcon(30, 20, "Explorer", COLOR_ICON_PC, hoverIdx == 1);
    renderIcon(30, 85, "WiFi Panel", COLOR_ICON_SET, hoverIdx == 2);
    renderIcon(30, 150, "Sistem Test", COLOR_ICON_YEL, hoverIdx == 3);
    renderIcon(100, 20, "Yılan Oyunu", 0x07E0, hoverIdx == 4);
    renderIcon(100, 85, "Pong 2P", 0xF81F, hoverIdx == 5);
    
    drawTaskbar();
    
    if (startMenuOpen) {
        tft.fillRect(0, 40, 170, 175, WIN10_TASKBAR);
        tft.drawRect(0, 40, 170, 175, WIN10_START);
        tft.setCursor(15, 60); tft.setTextColor(COLOR_WHITE); tft.print("Kynex Sovereign OS");
        tft.drawFastHLine(10, 75, 150, 0x4208);
        tft.setCursor(15, 95); tft.print("> WiFi Ayarları");
        tft.setCursor(15, 125); tft.print("> Bluetooth");
        tft.fillRect(0, 185, 170, 30, COLOR_RED); tft.setCursor(65, 197); tft.print("KAPAT");
    }
}

void drawMouse() {
    if (!mouseEnabled) return;
    tft.fillTriangle(mouseX, mouseY, mouseX+12, mouseY+12, mouseX, mouseY+16, COLOR_WHITE);
    tft.drawTriangle(mouseX, mouseY, mouseX+12, mouseY+12, mouseX, mouseY+16, COLOR_BLACK);
}

// --- EKRAN YÖNETİMİ ---
void drawScreen() {
    if (currentScreen == 0) drawDesktop(-1);
    else if (currentScreen == 1) tft.fillScreen(COLOR_BLACK);
    else if (currentScreen == 2) drawExplorerInfo();
    else if (currentScreen == 3) drawSettingsScreen();
    else if (currentScreen == 4) drawKynexKeyboard();
    else if (currentScreen == 5) drawBTScreen();
    else if (currentScreen == 6) drawSnakeGame();
    else if (currentScreen == 7) drawPongGame();
}

// --- OYUN FONKSİYONLARI ---
void spawnApple() { apple.x = random(1, 15) * 20; apple.y = random(1, 10) * 20; }

void drawSnakeGame() {
    tft.fillScreen(COLOR_BLACK);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(5, 5); tft.print("YILAN - Skor: "); tft.print(snakeLen-3);
    for(int i=0; i<snakeLen; i++) tft.fillRect(snake[i].x, snake[i].y, 18, 18, 0x07E0);
    tft.fillRect(apple.x, apple.y, 18, 18, COLOR_RED);
}

void updateSnake(JoyData j) {
    if (millis() - lastSnakeMove < snakeSpeed) return;
    if (j.x < 1000) snakeDir = 3; else if (j.x > 3000) snakeDir = 1;
    else if (j.y < 1000) snakeDir = 0; else if (j.y > 3000) snakeDir = 2;
    for(int i=snakeLen-1; i>0; i--) snake[i] = snake[i-1];
    if(snakeDir==0) snake[0].y-=20; else if(snakeDir==1) snake[0].x+=20;
    else if(snakeDir==2) snake[0].y+=20; else if(snakeDir==3) snake[0].x-=20;
    if(snake[0].x<0 || snake[0].x>300 || snake[0].y<20 || snake[0].y>220) { snakeLen=3; snake[0]={160,120}; }
    if(snake[0].x==apple.x && snake[0].y==apple.y) { snakeLen++; spawnApple(); playBeep(2000, 10); }
    drawSnakeGame(); lastSnakeMove = millis();
}

void drawPongGame() {
    tft.fillScreen(COLOR_BLACK);
    tft.fillRect(p1.x, p1.y, p1.w, p1.h, COLOR_WHITE);
    tft.fillRect(p2.x, p2.y, p2.w, p2.h, COLOR_WHITE);
    tft.fillCircle(ballX, ballY, 5, COLOR_RED);
}

void updatePong(JoyData j1, JoyData j2) {
    p1.y = constrain(p1.y + (j1.y-2048)/80, 0, 190);
    p2.y = constrain(p2.y + (j2.y-2048)/80, 0, 190);
    ballX += ballDX; ballY += ballDY;
    if(ballY<=5 || ballY>=235) ballDY*=-1;
    if(ballX<=25 && ballY>=p1.y && ballY<=p1.y+p1.h) ballDX*=-1;
    if(ballX>=295 && ballY>=p2.y && ballY<=p2.y+p2.h) ballDX*=-1;
    if(ballX<0 || ballX>320) { ballX=160; ballY=120; ballDX*=-1; playBeep(400, 100); }
    drawPongGame();
}

// --- ALT EKRAN DETAYLARI ---
void drawExplorerInfo() {
    tft.fillScreen(COLOR_WHITE); tft.fillRect(0,0,320,35,WIN10_START);
    tft.setCursor(10,12); tft.setTextColor(COLOR_WHITE); tft.print("Web Explorer - IP: "); tft.print(WiFi.softAPIP());
}

void drawSettingsScreen() {
    tft.fillScreen(COLOR_WHITE); tft.fillRect(0,0,320,35,WIN10_START);
    tft.setCursor(10,12); tft.setTextColor(COLOR_WHITE); tft.print("WiFi Aglari Taranıyor...");
    numNetworks = WiFi.scanNetworks();
    tft.fillRect(0,0,320,35,WIN10_START); tft.setCursor(10,12); tft.print("Baglanmak icin tiklayin:");
    tft.setTextColor(COLOR_BLACK);
    for(int i=0; i<min(numNetworks,6); i++) {
        networks[i] = WiFi.SSID(i);
        tft.setCursor(10, 50+(i*25)); tft.print(networks[i]);
    }
}

void drawBTScreen() {
    tft.fillScreen(COLOR_WHITE); tft.fillRect(0,0,320,35,WIN10_START);
    tft.setCursor(10,12); tft.setTextColor(COLOR_WHITE); tft.print("Bluetooth BLE Aktif");
}

void drawKynexKeyboard() {
    tft.fillScreen(COLOR_BLACK); tft.drawRect(10,10,300,35,COLOR_WHITE);
    tft.setCursor(15,22); tft.setTextColor(COLOR_WHITE); tft.print(kbBuffer);
    for(int r=0; r<3; r++) {
        for(int c=0; c<12; c++) {
            tft.drawRect(c*26+4, 60+r*40, 24, 35, 0x4208);
            tft.setCursor(c*26+10, 72+r*40); tft.print(kRows[kbMode][r].charAt(c));
        }
    }
}

void drawHardwareTest(JoyData j1, JoyData j2) {
    tft.fillScreen(COLOR_BLACK); tft.setTextColor(COLOR_WHITE);
    tft.setCursor(10,10); tft.print("DONANIM TESTI");
    tft.setCursor(10,50); tft.printf("JOY1 X:%d Y:%d B:%d", j1.x, j1.y, j1.btn);
    tft.setCursor(10,80); tft.printf("JOY2 X:%d Y:%d B:%d", j2.x, j2.y, j2.btn);
}

// --- TIKLAMA YÖNETİCİSİ ---
void handleGlobalClick(int x, int y) {
    playBeep(1800, 20);
    if (currentScreen == 0) {
        if (!startMenuOpen) {
            if (x < 90) {
                if (y > 20 && y < 80) { currentScreen = 2; drawScreen(); }
                else if (y > 85 && y < 145) { currentScreen = 3; drawScreen(); }
                else if (y > 150 && y < 210) { currentScreen = 1; drawScreen(); }
            } else if (x > 90 && x < 180) {
                if (y > 20 && y < 80) { currentScreen = 6; snakeLen=3; snake[0]={160,120}; drawScreen(); }
                else if (y > 85 && y < 145) { currentScreen = 7; drawScreen(); }
            }
        } else {
            if (x < 170 && y > 75 && y < 115) { currentScreen = 3; startMenuOpen = false; drawScreen(); }
            else if (x < 170 && y > 185) ESP.restart();
            else { startMenuOpen = false; drawScreen(); }
        }
    }
}

// --- SETUP VE LOOP ---
void setup() {
    Serial.begin(115200); psramInit(); FFat.begin(true);
    WiFi.mode(WIFI_AP_STA); WiFi.softAP("KynexOs-Win10", "*muhammed*krid*");
    server.on("/", handleWebRoot); server.begin();
    BLEDevice::init("Kynex-Sovereign-BLE");

    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY1_SW, INPUT_PULLUP); pinMode(JOY2_SW, INPUT_PULLUP);
    
    TJpgDec.setJpgScale(1); TJpgDec.setCallback(tft_output);
    SPI.begin(TFT_SCK, MISO_PIN, TFT_MOSI, TFT_CS); 
    tft.begin(); tft.setRotation(3); ts.begin(); ts.setRotation(3);
    
    drawScreen();
}

void loop() {
    server.handleClient();
    JoyData j1 = readJoy(JOY1_X, JOY1_Y, JOY1_SW);
    JoyData j2 = readJoy(JOY2_X, JOY2_Y, JOY2_SW);

    // --- AKILLI GERİ TUŞU (SOL JOY UZUN BASIŞ) ---
    if (j1.btn) {
        if (btnPressStart == 0) btnPressStart = millis();
        if (millis() - btnPressStart > 1500 && !longPressTriggered) {
            longPressTriggered = true; playBeep(600, 300);
            currentScreen = 0; startMenuOpen = false; drawScreen();
        }
    } else {
        if (btnPressStart != 0) {
            unsigned long dur = millis() - btnPressStart;
            if (!longPressTriggered && dur > 50) {
                if (mouseEnabled && currentScreen == 0) { handleGlobalClick(mouseX, mouseY); }
                else if (currentScreen == 0) { startMenuOpen = !startMenuOpen; drawScreen(); }
            }
            btnPressStart = 0; longPressTriggered = false;
        }
    }

    // --- EKRAN İŞLEME ---
    if (currentScreen == 6) updateSnake(j1);
    else if (currentScreen == 7) updatePong(j1, j2);
    else if (currentScreen == 0 && mouseEnabled) {
        int dx = (j1.x - 2048) / 100; int dy = (j1.y - 2048) / 100;
        if (abs(dx) > 1 || abs(dy) > 1) {
            mouseX = constrain(mouseX + dx, 0, 310); mouseY = constrain(mouseY + dy, 0, 225);
            if (millis() - lastAction > 50) { drawScreen(); drawMouse(); lastAction = millis(); }
        }
    }

    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        int tx = map(p.x, 200, 3850, 320, 0); int ty = map(p.y, 240, 3800, 240, 0);
        handleGlobalClick(tx, ty); delay(200);
    }
    
    if (currentScreen == 1) {
        if (millis() - lastAction > 100) { drawHardwareTest(j1, j2); lastAction = millis(); }
    }
}
