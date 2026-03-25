/* **************************************************************************
 * KynexOs Sovereign Build v230.120 - THE JOKER'S GAMBIT
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Clown XOX Game, Zero-Clipping Audio Engine (No Crackle), Retro-Go Boot
 * Donanım: ESP32-S3 N16R8 (V325 Pinout)
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
#include <time.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"
#include "esp_sleep.h"
#include "driver/i2s.h" 
#include <math.h>
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
#define JOY_SELECT 0 

// JOYSTICK PİNLERİ
#define J1_X 4
#define J1_Y 6
#define J2_X 7
#define J2_Y 15

// I2S PIN HARİTASI
#define I2S_LRC  18
#define I2S_BCLK 17
#define I2S_DOUT 5 

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS);
Preferences prefs;

enum State { DESKTOP, START_MENU, SETTINGS_HUB, WIFI_MENU, BT_MENU, CMD_PROMPT, SYS_INFO, PAINT, TEST_MENU, GAME_MENU, CALCULATOR, POWER_MENU, XOX_GAME };
State currentState = DESKTOP;
bool whiteTheme = false;
bool btState = false;
int globalVolume = 50; 
String savedSSID = "";
String savedPASS = "";
unsigned long pressTimer = 0;
unsigned long lastClockUpdate = 0;
bool isLongPress = false;
uint16_t paintColor = 0xF800;

// ---------------- I2S DİJİTAL SES MOTORU (SIFIR CIZIRTI MODU) ----------------
void initI2S() {
    Serial.println("[I2S] Baslatma dizisi basladi...");
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 16, 
        .dma_buf_len = 512, 
        .use_apll = false,
        .tx_desc_auto_clear = true 
    };
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE, 
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
}

void playToneI2S(float freq, int duration_ms) {
    if (freq <= 0 || globalVolume <= 0) return;
    int sampleRate = 44100;
    int samples = (sampleRate * duration_ms) / 1000;
    size_t bytes_written;
    
    // MUHAMMED: Dijital Kırpılmayı (Clipping/Cızırtı) önlemek için 32767 yerine 25000 kullanıyoruz.
    float amplitude = 25000.0 * (globalVolume / 100.0);
    
    for(int i=0; i<samples; i++) {
        int16_t sample = (int16_t)(amplitude * sin(2.0 * PI * freq * i / sampleRate));
        uint32_t sample32 = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
        i2s_write(I2S_NUM_0, &sample32, sizeof(sample32), &bytes_written, portMAX_DELAY);
    }
}

void playSquareWaveI2S(float freq, int duration_ms) {
    if (freq <= 0) return;
    int sampleRate = 44100;
    int samples = (sampleRate * duration_ms) / 1000;
    size_t bytes_written;
    float amplitude = 20000.0 * (globalVolume / 100.0); // Kaba kuvvet cızırtısı azaltıldı
    int half_period = sampleRate / (freq * 2);
    
    for(int i=0; i<samples; i++) {
        int16_t sample = ((i / half_period) % 2 == 0) ? (int16_t)amplitude : (int16_t)-amplitude;
        uint32_t sample32 = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
        i2s_write(I2S_NUM_0, &sample32, sizeof(sample32), &bytes_written, portMAX_DELAY);
    }
}

void playClick() { playToneI2S(1200, 15); } 
void playBeep() { playToneI2S(1500, 50); } 
void playError() { playToneI2S(300, 200); } 
void playBootSound() { 
    playToneI2S(523.25, 150); delay(50);
    playToneI2S(659.25, 150); delay(50);
    playToneI2S(783.99, 150); delay(50);
    playToneI2S(1046.50, 400); 
}

// Dokunmatik Kalibrasyonu 
int getTX(int rawX) { return map(rawX, 3900, 150, 0, 320); } 
int getTY(int rawY) { return map(rawY, 3800, 200, 0, 240); }

// ZAMAN VE MASAÜSTÜ YÖNETİMİ
void drawClock() {
    tft.fillRect(260, 215, 60, 25, whiteTheme ? 0xAD75 : 0x10A2); 
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF); tft.setTextSize(1); tft.setCursor(265, 224);
    if(WiFi.status() == WL_CONNECTED) {
        struct tm timeinfo;
        if(getLocalTime(&timeinfo, 10)) { tft.printf("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min); } 
        else { tft.print("SYNC.."); }
    } else { tft.setTextColor(0xF800); tft.print("OFFLINE"); }
}

void renderDesktop() {
    drawWin10HeroWallpaper(&tft, whiteTheme); 
    tft.fillRect(0, 215, 320, 25, whiteTheme ? 0xAD75 : 0x10A2); 
    drawRealWin10Logo(&tft, 8, 217, 24, 20, whiteTheme ? 0x0000 : 0xFFFF);
    
    if(WiFi.status() == WL_CONNECTED) tft.fillCircle(240, 227, 4, 0x07E0); else tft.drawCircle(240, 227, 4, whiteTheme ? 0x0000 : 0xFFFF);
    if(btState) tft.fillCircle(250, 227, 4, 0x03FF); else tft.drawCircle(250, 227, 4, whiteTheme ? 0x0000 : 0xFFFF);
    drawClock();
}

void renderStartMenu() {
    tft.fillRect(0, 5, 240, 210, whiteTheme ? 0xFFFF : 0x2104);
    tft.drawRect(0, 5, 240, 210, 0x07FF);
    tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setCursor(10, 15);  tft.print("> AGLAR (WIFI/BT)");
    tft.setCursor(10, 45);  tft.print("> AYARLAR & SISTEM");
    tft.setCursor(10, 75);  tft.print("> CMD PROMPT");
    tft.setCursor(10, 105); tft.print("> HESAP MAKINESI");
    tft.setCursor(10, 135); tft.print("> PAINT");
    tft.setCursor(10, 165); tft.print("> TESTLER");
    tft.setCursor(10, 195); tft.print("> OYUNLAR & UYG");

    tft.fillRect(140, 15, 80, 40, 0x03FF); tft.setCursor(145, 30); tft.setTextColor(0xFFFF); tft.print("WIFI");
    tft.fillRect(140, 60, 80, 40, 0x3186); tft.setCursor(145, 75); tft.print("BLUETOOTH");
    tft.fillRect(140, 105, 80, 40, 0xF800); tft.setCursor(145, 120); tft.print("GUC MENUSU");
    tft.fillRect(140, 150, 80, 55, 0x07E0); tft.setCursor(145, 170); tft.setTextColor(0); tft.print("RETRO-GO");
}

// ---------------- KLAVYE ----------------
String runKeyboard(String prompt) {
    String input = ""; int mode = 0; 
    const char* keysL[3][10] = { {"q","w","e","r","t","y","u","i","o","p"}, {"a","s","d","f","g","h","j","k","l","@"}, {"z","x","c","v","b","n","m",".","-","_"} };
    const char* keysU[3][10] = { {"Q","W","E","R","T","Y","U","I","O","P"}, {"A","S","D","F","G","H","J","K","L","@"}, {"Z","X","C","V","B","N","M",".","-","_"} };
    const char* keysN[3][10] = { {"1","2","3","4","5","6","7","8","9","0"}, {"!","#","$","%","&","*","+","=","/","?"}, {"(",")","<",">","[","]","{","}",":",";"} };

    tft.fillScreen(0x0000); bool drawKeys = true;
    while(true) {
        esp_task_wdt_reset();
        if(drawKeys) {
            tft.fillRect(0, 0, 320, 80, 0x10A2);
            tft.setTextColor(0xFFFF); tft.setTextSize(1); tft.setCursor(10, 10); tft.print(prompt);
            tft.setCursor(10, 40); tft.setTextSize(2); tft.print(input + "_"); tft.setTextSize(1);
            tft.fillRect(0, 80, 320, 160, 0x0000);
            for(int r=0; r<3; r++) {
                for(int c=0; c<10; c++) {
                    tft.drawRect(c*32, 80+r*40, 32, 40, 0xFFFF); tft.setCursor(c*32+12, 80+r*40+15);
                    if(mode==0) tft.print(keysL[r][c]); else if(mode==1) tft.print(keysU[r][c]); else tft.print(keysN[r][c]);
                }
            }
            tft.drawRect(0, 200, 60, 40, 0x07FF); tft.setCursor(15, 215); tft.print(mode==2?"ABC":"123");
            tft.drawRect(60, 200, 60, 40, 0x07FF); tft.setCursor(75, 215); tft.print(mode==1?"kucuk":"BUYUK");
            tft.drawRect(120, 200, 80, 40, 0xFFFF); tft.setCursor(140, 215); tft.print("BOSLUK");
            tft.drawRect(200, 200, 60, 40, 0xF800); tft.setCursor(215, 215); tft.print("SIL");
            tft.drawRect(260, 200, 60, 40, 0x07E0); tft.setCursor(275, 215); tft.print("TAMAM");
            drawKeys = false;
        }

        if (touch.touched()) {
            playClick(); TS_Point p = touch.getPoint(); int tx = getTX(p.x); int ty = getTY(p.y);
            if(ty >= 80 && ty < 200) { 
                int c = tx / 32; int r = (ty - 80) / 40;
                if(c>=0&&c<10 && r>=0&&r<3) {
                    if(mode==0) input += keysL[r][c]; else if(mode==1) input += keysU[r][c]; else input += keysN[r][c];
                    drawKeys = true;
                }
            } else if(ty >= 200) { 
                if(tx < 60) { mode = (mode==2) ? 0 : 2; drawKeys = true; } 
                else if(tx < 120) { mode = (mode==1) ? 0 : 1; drawKeys = true; } 
                else if(tx < 200) { input += " "; drawKeys = true; } 
                else if(tx < 260) { if(input.length()>0) input.remove(input.length()-1); drawKeys = true; } 
                else if(tx >= 260) { playBeep(); return input; } 
            }
            delay(200); 
        }
    }
}

// ---------------- AĞ VE BT ----------------
void runWifiManager() {
    tft.fillScreen(0x0000); tft.setTextColor(0xFFFF); tft.setCursor(10, 10); tft.print("WIFI AGLARI TARANIYOR...");
    WiFi.mode(WIFI_STA); int n = WiFi.scanNetworks();
    tft.fillScreen(0x0000); tft.fillRect(0, 0, 320, 30, 0x03FF); tft.setCursor(10, 10); tft.print("WIFI AYARLARI (Secmek Icin Dokun)");
    for (int i = 0; i < 4 && i < n; ++i) { tft.drawRect(10, 40 + i*40, 300, 35, 0x07FF); tft.setCursor(20, 50 + i*40); tft.print(WiFi.SSID(i)); }
    tft.fillRect(10, 200, 300, 35, 0xF800); tft.setCursor(120, 210); tft.print("GERI (CIKIS)");

    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        if (touch.touched()) {
            playClick(); TS_Point p = touch.getPoint(); int tx = getTX(p.x); int ty = getTY(p.y);
            if(ty > 40 && ty < 190) { 
                int idx = (ty - 40) / 40;
                if(idx < n) {
                    String selectedSSID = WiFi.SSID(idx);
                    String pass = runKeyboard("Sifre Girin: " + selectedSSID);
                    tft.fillScreen(0x0000); tft.setCursor(10, 100); tft.print("BAGLANILIYOR...");
                    WiFi.begin(selectedSSID.c_str(), pass.c_str());
                    int timeout = 0; while (WiFi.status() != WL_CONNECTED && timeout < 20) { delay(500); timeout++; esp_task_wdt_reset(); }
                    if(WiFi.status() == WL_CONNECTED) {
                        prefs.putString("ssid", selectedSSID); prefs.putString("pass", pass);
                        configTime(3 * 3600, 0, "pool.ntp.org"); 
                        tft.fillScreen(0x07E0); tft.setCursor(100, 120); tft.setTextColor(0x0000); tft.print("WIFI BAGLANDI!");
                        playBeep();
                    } else { tft.fillScreen(0xF800); tft.setCursor(100, 120); tft.setTextColor(0xFFFF); tft.print("BAGLANTI HATASI!"); playError(); }
                    delay(2000); break;
                }
            } else if(ty > 200) { break; } 
            delay(300);
        }
    }
    currentState = DESKTOP; renderDesktop();
}

void runBtManager() {
    tft.fillScreen(0x0000); tft.fillRect(0, 0, 320, 30, 0x3186); tft.setTextColor(0xFFFF); tft.setCursor(10, 10); tft.print("BLUETOOTH AYARLARI");
    tft.fillRect(50, 80, 220, 50, btState ? 0xF800 : 0x07E0);
    tft.setCursor(90, 100); tft.print(btState ? "BLUETOOTH KAPAT" : "BLUETOOTH AC");
    tft.fillRect(50, 150, 220, 50, 0x03FF); tft.setCursor(130, 170); tft.print("GERI");
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        if (touch.touched()) {
            playClick(); TS_Point p = touch.getPoint(); int ty = getTY(p.y);
            if(ty > 80 && ty < 130) { btState = !btState; if(btState) btStart(); else btStop(); playBeep(); break; }
            if(ty > 150 && ty < 200) { break; }
        }
    }
    currentState = DESKTOP; renderDesktop();
}

// ---------------- HESAP MAKİNESİ & CMD & SYS INFO ----------------
void runCalculator() {
    String eq = ""; const char* cKeys[4][4] = { {"7","8","9","/"}, {"4","5","6","*"}, {"1","2","3","-"}, {"C","0","=","+"} };
    tft.fillScreen(0x0000); bool drawC = true; delay(300);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        if(drawC) {
            tft.fillRect(10, 10, 300, 50, 0xFFFF);
            tft.setTextColor(0x0000); tft.setTextSize(2); tft.setCursor(20, 25); tft.print(eq); tft.setTextSize(1);
            for(int r=0; r<4; r++) {
                for(int c=0; c<4; c++) {
                    tft.drawRect(10+c*75, 70+r*40, 70, 35, 0x07FF); tft.setTextColor(0xFFFF); tft.setCursor(40+c*75, 80+r*40); tft.print(cKeys[r][c]);
                }
            } drawC = false;
        }
        if (touch.touched()) {
            playClick(); TS_Point p = touch.getPoint(); int tx = getTX(p.x); int ty = getTY(p.y);
            if(ty > 70 && ty < 230) {
                int c = (tx - 10) / 75; int r = (ty - 70) / 40;
                if(c>=0&&c<4 && r>=0&&r<4) {
                    String k = cKeys[r][c]; if(k == "C") eq = ""; else if(k == "=") eq = "Hesaplandi"; else eq += k;
                    drawC = true;
                }
            } delay(200);
        }
    }
    currentState = DESKTOP; renderDesktop();
}

void runCMD() {
    tft.fillScreen(0x0000); tft.setTextColor(0x07E0); tft.setTextSize(1);
    tft.setCursor(0, 5); tft.println("Microsoft Windows [Version 10.0.19045]"); tft.println("(c) Microsoft Corporation. Tum haklari saklidir.\n");
    delay(500); tft.println("C:\\KynexOS\\System32> ping KynexServer..."); delay(500);
    tft.println("Cevap 192.168.1.1: bayt=32 sure=14ms TTL=119"); delay(500); tft.println("Cevap 192.168.1.1: bayt=32 sure=18ms TTL=119"); delay(500);
    tft.println("\nC:\\KynexOS\\System32> ipconfig"); delay(500);
    tft.println("\nEthernet bagdastiricisi Sovereign_WLAN:");
    tft.print("   IPv4 Adresi. . . . . : "); tft.println(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "Bagli Degil");
    tft.println("\nC:\\KynexOS\\System32> _");
    tft.setTextColor(0xF800); tft.setCursor(0, 230); tft.print("SELECT ILE CIKIS YAPIN");
    while(digitalRead(JOY_SELECT) == HIGH) { esp_task_wdt_reset(); delay(50); }
    currentState = DESKTOP; renderDesktop();
}

void runSysInfo() {
    tft.fillScreen(whiteTheme ? 0xFFFF : 0x0000); tft.setTextColor(0x07FF); tft.setTextSize(2); tft.setCursor(10, 10); tft.print("SISTEM PANELI");
    tft.setTextSize(1); tft.setTextColor(whiteTheme ? 0x0000 : 0xFFFF);
    tft.setCursor(10, 50); tft.printf("CPU: %d MHz ESP32-S3", ESP.getCpuFreqMHz());
    tft.setCursor(10, 80); tft.printf("RAM: %d KB", ESP.getFreeHeap()/1024);
    tft.setCursor(10, 110); tft.printf("PSRAM: %d KB", ESP.getFreePsram()/1024);
    tft.setCursor(10, 140); tft.print("WIFI IP: " + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "OFFLINE"));
    tft.setTextColor(0xF800); tft.setCursor(10, 210); tft.print("SELECT TUSU CIKIS");
    while(digitalRead(JOY_SELECT) == HIGH) { esp_task_wdt_reset(); delay(100); }
    currentState = DESKTOP; renderDesktop();
}

// ---------------- OYUNLAR, BOYA, PİYANO VE TESTLER ----------------

// MUHAMMED: YENİ PALYAÇOLU XOX OYUNU (TIC-TAC-TOE)
void runXOX() {
    int board[9] = {0}; // 0: Boş, 1: X, 2: Palyaço
    int turn = 1;
    int winner = 0;
    
    tft.fillScreen(0x0000);
    tft.setTextColor(0xFFFF); tft.setTextSize(1);
    tft.setCursor(10, 10); tft.print("XOX - SELECT CIKIS");
    
    // 3x3 Tahtayı Çiz (Orta Boyut: 180x180, Kutular: 60x60)
    tft.drawRect(70, 30, 180, 180, 0xFFFF);
    tft.drawLine(130, 30, 130, 210, 0xFFFF); // Dikey 1
    tft.drawLine(190, 30, 190, 210, 0xFFFF); // Dikey 2
    tft.drawLine(70, 90, 250, 90, 0xFFFF);   // Yatay 1
    tft.drawLine(70, 150, 250, 150, 0xFFFF); // Yatay 2
    
    delay(300);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        if (touch.touched() && winner == 0) {
            TS_Point p = touch.getPoint(); int tx = getTX(p.x); int ty = getTY(p.y);
            
            if (tx > 70 && tx < 250 && ty > 30 && ty < 210) {
                int col = (tx - 70) / 60;
                int row = (ty - 30) / 60;
                int idx = row * 3 + col;
                
                if (board[idx] == 0) {
                    board[idx] = turn;
                    int cx = 70 + col * 60; // Kutunun X başlangıcı
                    int cy = 30 + row * 60; // Kutunun Y başlangıcı
                    
                    if (turn == 1) {
                        // X Çizimi
                        tft.drawLine(cx + 10, cy + 10, cx + 50, cy + 50, 0x07E0);
                        tft.drawLine(cx + 11, cy + 10, cx + 51, cy + 50, 0x07E0);
                        tft.drawLine(cx + 50, cy + 10, cx + 10, cy + 50, 0x07E0);
                        tft.drawLine(cx + 51, cy + 10, cx + 11, cy + 50, 0x07E0);
                        playToneI2S(800, 50); // X Sesi
                        turn = 2;
                    } else {
                        // PALYAÇO ÇİZİMİ (O Yerine)
                        tft.fillCircle(cx+30, cy+30, 22, 0xFFE0); // Sarı/Beyaz Yüz
                        tft.fillCircle(cx+10, cy+20, 10, 0x07FF); // Mavi Saç Sol
                        tft.fillCircle(cx+50, cy+20, 10, 0x07FF); // Mavi Saç Sağ
                        tft.fillCircle(cx+22, cy+25, 3, 0x0000);  // Göz Sol
                        tft.fillCircle(cx+38, cy+25, 3, 0x0000);  // Göz Sağ
                        tft.fillCircle(cx+30, cy+35, 6, 0xF800);  // Kırmızı Burun
                        tft.drawLine(cx+20, cy+45, cx+40, cy+45, 0xF800); // Ağız Çizgisi
                        tft.drawLine(cx+20, cy+45, cx+15, cy+40, 0xF800); // Gülümseme Sol
                        tft.drawLine(cx+40, cy+45, cx+45, cy+40, 0xF800); // Gülümseme Sağ
                        playToneI2S(1400, 50); // Palyaço Sesi
                        turn = 1;
                    }
                    
                    // Win Check
                    int wins[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
                    for(int i=0; i<8; i++) {
                        if(board[wins[i][0]] != 0 && board[wins[i][0]] == board[wins[i][1]] && board[wins[i][1]] == board[wins[i][2]]) {
                            winner = board[wins[i][0]];
                            tft.setCursor(90, 10); tft.fillRect(90, 0, 230, 25, 0x0000);
                            if(winner == 1) { tft.setTextColor(0x07E0); tft.print("X KAZANDI!"); }
                            else { tft.setTextColor(0xF800); tft.print("PALYACO KAZANDI!"); }
                            playToneI2S(winner == 1 ? 1200 : 600, 400); // Kazanma müziği
                            break;
                        }
                    }
                    
                    // Beraberlik Kontrolü
                    if(winner == 0) {
                        bool full = true;
                        for(int i=0; i<9; i++) if(board[i] == 0) full = false;
                        if(full) {
                            winner = 3;
                            tft.setCursor(120, 10); tft.fillRect(90, 0, 230, 25, 0x0000);
                            tft.setTextColor(0xFFFF); tft.print("BERABERE!");
                            playToneI2S(400, 300);
                        }
                    }
                }
            }
            delay(200);
        }
    }
    currentState = DESKTOP; renderDesktop();
}

void runPianoApp() {
    tft.fillScreen(0x0000); tft.setTextColor(0xFFFF); tft.setCursor(10, 10); tft.print("PIYANO - SELECT TUSU CIKIS");
    for(int i=0; i<8; i++) tft.fillRect(i*40, 40, 38, 200, 0xFFFF); 
    int bKeys[] = {1, 2, 4, 5, 6}; 
    for(int i=0; i<5; i++) tft.fillRect(bKeys[i]*40 - 10, 40, 20, 120, 0x0000);
    delay(300);
    
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        if(touch.touched()) {
            TS_Point p = touch.getPoint(); int tx = getTX(p.x); int ty = getTY(p.y);
            if(ty > 40) {
                int key = tx / 40;
                float freqs[] = {261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25}; 
                if(key>=0 && key<8) {
                    tft.fillRect(key*40, 180, 38, 60, 0x07FF); 
                    playToneI2S(freqs[key], 150); 
                    tft.fillRect(key*40, 180, 38, 60, 0xFFFF); 
                }
            }
        }
    }
    currentState = DESKTOP; renderDesktop();
}

void run3DCube() {
    tft.fillScreen(0x0000); float ax=0, ay=0; delay(300);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset(); ax += 0.05; ay += 0.03; 
        tft.fillScreen(0x0000); tft.setTextColor(0xFFFF); tft.setCursor(10,10); tft.print("3D AUTO CUBE - SELECT CIKIS");
        tft.drawRect(110+sin(ax)*20, 70+cos(ay)*20, 100, 100, 0x5DFF); tft.drawRect(130+sin(ax)*20, 90+cos(ay)*20, 100, 100, 0xFFFF);
        tft.drawLine(110+sin(ax)*20, 70+cos(ay)*20, 130+sin(ax)*20, 90+cos(ay)*20, 0x07E0); tft.drawLine(210+sin(ax)*20, 70+cos(ay)*20, 230+sin(ax)*20, 90+cos(ay)*20, 0x07E0);
        tft.drawLine(110+sin(ax)*20, 170+cos(ay)*20, 130+sin(ax)*20, 190+cos(ay)*20, 0x07E0); tft.drawLine(210+sin(ax)*20, 170+cos(ay)*20, 230+sin(ax)*20, 190+cos(ay)*20, 0x07E0);
        delay(30);
    }
    currentState = DESKTOP; renderDesktop();
}

void runSnake() {
    int x[50], y[50], len=5, dx=8, dy=0, ax=160, ay=120, score=0;
    for(int i=0;i<50;i++){x[i]=-10;y[i]=-10;} x[0]=160; y[0]=120;
    tft.fillScreen(0x0000); tft.fillCircle(ax+4, ay+4, 4, 0xF800); delay(300);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int jx = analogRead(J1_X); int jy = analogRead(J1_Y);
        if(jx < 1000 && dx==0) {dx=-8; dy=0;} else if(jx > 3000 && dx==0) {dx=8; dy=0;}
        else if(jy < 1000 && dy==0) {dx=0; dy=-8;} else if(jy > 3000 && dy==0) {dx=0; dy=8;}
        
        tft.fillRect(x[len-1], y[len-1], 8, 8, 0x0000);
        for(int i=len-1; i>0; i--) { x[i]=x[i-1]; y[i]=y[i-1]; }
        x[0]+=dx; y[0]+=dy;
        
        if(abs(x[0]-ax)<8 && abs(y[0]-ay)<8) { 
            if(len<49) len++; score+=10; ax = random(2, 38)*8; ay = random(2, 25)*8; 
            tft.fillCircle(ax+4, ay+4, 4, 0xF800); playToneI2S(1200, 50); 
        }
        tft.fillRect(x[0], y[0], 8, 8, 0x07E0); 
        if(x[0]<0 || x[0]>=320 || y[0]<0 || y[0]>=240) {
            playToneI2S(200, 500); tft.fillScreen(0xF800); tft.setCursor(100, 120); tft.setTextColor(0xFFFF); tft.printf("OYUN BITTI! SKOR: %d", score);
            delay(2000); break;
        } delay(60); 
    }
    currentState = DESKTOP; renderDesktop();
}

void runPong() {
    int p1y=100, p2y=100, bx=160, by=120, bdx=3, bdy=3, s1=0, s2=0; tft.fillScreen(0); delay(300);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        p1y = map(analogRead(J1_Y), 0, 4095, 0, 205); p2y = map(analogRead(J2_X), 0, 4095, 0, 205); 
        tft.fillScreen(0); tft.setCursor(130, 10); tft.setTextColor(0x03FF); tft.printf("%d - %d", s1, s2);
        tft.fillRect(10, p1y, 8, 35, 0xF800); tft.fillRect(302, p2y, 8, 35, 0x07E0); tft.fillCircle(bx, by, 3, 0xFFFF);
        bx+=bdx; by+=bdy;
        if(by<=0 || by>=240) { bdy=-bdy; playToneI2S(600, 30); } 
        if(bx<=20 && by>p1y && by<p1y+35) { bdx=-bdx; playToneI2S(800, 30); } 
        if(bx>=295 && by>p2y && by<p2y+35) { bdx=-bdx; playToneI2S(800, 30); } 
        if(bx<0) { s2++; bx=160; by=120; playToneI2S(300, 300); delay(500); } 
        if(bx>320) { s1++; bx=160; by=120; playToneI2S(300, 300); delay(500); } 
        delay(20);
    }
    currentState = DESKTOP; renderDesktop();
}

void runPaintApp() {
    tft.fillScreen(0xFFFF); tft.fillRect(280, 0, 40, 240, 0xC618);
    tft.fillRect(285, 10, 30, 30, 0xF800); tft.fillRect(285, 50, 30, 30, 0x07E0); tft.fillRect(285, 90, 30, 30, 0x001F); tft.fillRect(285, 130, 30, 30, 0xFFE0);
    tft.fillRect(285, 170, 30, 30, 0x0000); tft.fillRect(285, 210, 30, 25, 0xF81F); delay(300);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        if (touch.touched()) {
            TS_Point p = touch.getPoint(); int tx = getTX(p.x); int ty = getTY(p.y);
            if (tx > 280) {
                playClick();
                if (ty > 210) break;
                else if (ty > 10 && ty < 40) paintColor = 0xF800; else if (ty > 50 && ty < 80) paintColor = 0x07E0;
                else if (ty > 90 && ty < 120) paintColor = 0x001F; else if (ty > 130 && ty < 160) paintColor = 0xFFE0;
                else if (ty > 170 && ty < 200) paintColor = 0xFFFF; delay(100);
            } else tft.fillCircle(tx, ty, 3, paintColor);
        }
    }
    currentState = DESKTOP; renderDesktop();
}

void runJoyTest() {
    tft.fillScreen(0x0000); delay(300);
    while(digitalRead(JOY_SELECT) == HIGH) {
        esp_task_wdt_reset();
        int j1x = analogRead(J1_X); int j1y = analogRead(J1_Y); int j2x = analogRead(J2_X); int j2y = analogRead(J2_Y);
        tft.fillRect(20, 50, 130, 100, 0x1084); tft.setCursor(25, 60); tft.setTextColor(0xFFFF); tft.printf("SOL JOY\nX:%d Y:%d", j1x, j1y); tft.fillCircle(20 + map(j1x, 0, 4095, 5, 125), 50 + map(j1y, 0, 4095, 5, 95), 4, 0xF800);
        tft.fillRect(170, 50, 130, 100, 0x1084); tft.setCursor(175, 60); tft.printf("SAG JOY\nX:%d Y:%d", j2x, j2y); tft.fillCircle(170 + map(j2x, 0, 4095, 5, 125), 50 + map(j2y, 0, 4095, 5, 95), 4, 0x07E0);
        delay(30);
    }
    currentState = DESKTOP; renderDesktop();
}

// ---------------- RETRO-GO ABSOLUTE BOOTLOADER ----------------
void bootToRetroGo() {
    tft.fillScreen(0x0000); tft.setTextColor(0x07E0); tft.setTextSize(2); tft.setCursor(20, 110); 
    tft.print("RETRO-GO YUKLENIYOR..");
    
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    const esp_partition_t* target = NULL;
    while (it != NULL) {
        const esp_partition_t* part = esp_partition_get(it);
        if (part->address == 0x410000) { target = part; break; }
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
    
    if (target != NULL) {
        esp_ota_set_boot_partition(target);
        delay(200); ESP.restart();
    } else {
        tft.fillScreen(0xF800); tft.setTextColor(0xFFFF); tft.setTextSize(1); tft.setCursor(20, 120); 
        tft.print("HATA: 0x410000 ADRESINDE PARTITION YOK!");
        delay(3000); currentState = DESKTOP; renderDesktop();
    }
}

// ---------------- ANA DÖNGÜ (SETUP & LOOP) ----------------
void setup() {
    Serial.begin(115200);
    initI2S(); 
    
    WiFi.mode(WIFI_OFF);
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    pinMode(JOY_SELECT, INPUT_PULLUP);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin(20000000); tft.setRotation(1); touch.begin(); touch.setRotation(1);
    
    prefs.begin("sov_v325", false);
    whiteTheme = prefs.getBool("theme", false);
    globalVolume = prefs.getInt("vol", 50); 
    if(globalVolume <= 0 || globalVolume > 100) globalVolume = 50; 
    
    savedSSID = prefs.getString("ssid", ""); savedPASS = prefs.getString("pass", "");

    if(psramInit()) Serial.println("[SYSTEM] PSRAM Tespit Edildi.");
    esp_task_wdt_init(30, true);

    if(savedSSID != "") {
        WiFi.mode(WIFI_STA); WiFi.begin(savedSSID.c_str(), savedPASS.c_str());
        int timeout = 0; while (WiFi.status() != WL_CONNECTED && timeout < 15) { delay(300); timeout++; }
        if(WiFi.status() == WL_CONNECTED) configTime(3 * 3600, 0, "pool.ntp.org"); 
    }

    playBootSound(); 
    renderDesktop();
}

void loop() {
    esp_task_wdt_reset();

    if (currentState == DESKTOP && (millis() - lastClockUpdate > 5000)) {
        drawClock(); lastClockUpdate = millis();
    }

    if (digitalRead(JOY_SELECT) == LOW) {
        if (pressTimer == 0) pressTimer = millis();
        if (millis() - pressTimer > 2000 && !isLongPress) {
            playBeep(); isLongPress = true; currentState = POWER_MENU;
            tft.fillScreen(0x0000); 
            tft.fillRect(30, 20, 260, 200, 0xF800); 
            tft.setTextColor(0xFFFF); tft.setCursor(110, 35); tft.print("GUC & SES");
            tft.fillRect(50, 55, 220, 30, 0x0000); tft.setCursor(100, 65); tft.print("KAPAT (UYKU)");
            tft.fillRect(50, 95, 220, 30, 0x03FF); tft.setCursor(95, 105); tft.print("YENIDEN BASLAT");
            tft.setCursor(40, 145); tft.print("SES:");
            tft.drawRect(85, 140, 170, 20, 0xFFFF); tft.fillRect(85, 140, (globalVolume*170)/100, 20, 0x07E0);
            tft.fillRect(50, 175, 220, 30, 0x10A2); tft.setCursor(140, 185); tft.print("GERI"); 
            delay(500);
        }
    } else { pressTimer = 0; isLongPress = false; }

    if (touch.touched()) {
        TS_Point p = touch.getPoint(); int tx = getTX(p.x); int ty = getTY(p.y);

        if (currentState == DESKTOP) {
            if (tx < 50 && ty > 210) { playClick(); currentState = START_MENU; renderStartMenu(); delay(300); }
        } 
        else if (currentState == START_MENU) {
            if (tx > 140 && ty > 15 && ty < 55) { playClick(); currentState = WIFI_MENU; runWifiManager(); }
            else if (tx > 140 && ty > 60 && ty < 100) { playClick(); currentState = BT_MENU; runBtManager(); }
            else if (tx > 140 && ty > 105 && ty < 145) { 
                playClick(); currentState = POWER_MENU;
                tft.fillScreen(0x0000); 
                tft.fillRect(30, 20, 260, 200, 0xF800); 
                tft.setTextColor(0xFFFF); tft.setCursor(110, 35); tft.print("GUC & SES");
                tft.fillRect(50, 55, 220, 30, 0x0000); tft.setCursor(100, 65); tft.print("KAPAT (UYKU)");
                tft.fillRect(50, 95, 220, 30, 0x03FF); tft.setCursor(95, 105); tft.print("YENIDEN BASLAT");
                tft.setCursor(40, 145); tft.print("SES:");
                tft.drawRect(85, 140, 170, 20, 0xFFFF); tft.fillRect(85, 140, (globalVolume*170)/100, 20, 0x07E0);
                tft.fillRect(50, 175, 220, 30, 0x10A2); tft.setCursor(140, 185); tft.print("GERI"); 
                delay(300);
            }
            else if (tx > 140 && ty > 150) { playClick(); bootToRetroGo(); }
            
            else if (ty > 15 && ty < 40) { playClick(); currentState = SETTINGS_HUB; tft.fillScreen(whiteTheme?0xFFFF:0); tft.fillRect(50, 80, 220, 50, 0x07FF); tft.setTextColor(0xFFFF); tft.setCursor(100, 100); tft.print("TEMA DEGISTIR"); tft.fillRect(50, 150, 220, 50, 0x07E0); tft.setCursor(110, 170); tft.print("KAYDET VE CIK"); delay(300); }
            else if (ty > 40 && ty < 70) { playClick(); currentState = SYS_INFO; runSysInfo(); }
            else if (ty > 70 && ty < 100) { playClick(); currentState = CMD_PROMPT; runCMD(); }
            else if (ty > 100 && ty < 130) { playClick(); currentState = CALCULATOR; runCalculator(); }
            else if (ty > 130 && ty < 160) { playClick(); currentState = PAINT; runPaintApp(); }
            else if (ty > 160 && ty < 190) { playClick(); currentState = TEST_MENU; tft.fillRect(161, 150, 130, 75, 0x1084); tft.drawRect(161, 150, 130, 75, 0x07FF); tft.setCursor(170, 165); tft.print("1. DOKUNMA TEST"); tft.setCursor(170, 185); tft.print("2. JOYSTICK TEST"); tft.setCursor(170, 205); tft.print("3. I2S SES TEST"); delay(300); }
            else if (ty > 190) { 
                playClick(); currentState = GAME_MENU; 
                tft.fillRect(161, 100, 130, 125, 0x1084); 
                tft.drawRect(161, 100, 130, 125, 0xF81F); 
                tft.setCursor(170, 110); tft.print("1. 3D KUBE"); 
                tft.setCursor(170, 135); tft.print("2. YILAN"); 
                tft.setCursor(170, 160); tft.print("3. PONG 2P"); 
                tft.setCursor(170, 185); tft.print("4. PIYANO"); 
                tft.setCursor(170, 210); tft.print("5. XOX OYUNU"); 
                delay(300); 
            }
            else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == SETTINGS_HUB) {
            playClick();
            if (ty > 80 && ty < 130) { whiteTheme = !whiteTheme; prefs.putBool("theme", whiteTheme); delay(300); }
            if (ty > 150 && ty < 200) { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == GAME_MENU) {
            playClick();
            if (ty > 100 && ty < 125) run3DCube(); 
            else if (ty > 125 && ty < 150) runSnake(); 
            else if (ty > 150 && ty < 175) runPong(); 
            else if (ty > 175 && ty < 200) runPianoApp();
            else if (ty > 200 && ty < 225) runXOX(); // PALYAÇO GÖREVE HAZIR!
            else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == TEST_MENU) {
            playClick();
            if (ty > 150 && ty < 175) { tft.fillScreen(0xFFFF); tft.setTextColor(0); tft.setCursor(10,10); tft.print("DOKUNMA TEST - SELECT:CIKIS"); delay(300); while(digitalRead(JOY_SELECT)==HIGH) { if(touch.touched()){ TS_Point tp = touch.getPoint(); tft.drawCircle(getTX(tp.x), getTY(tp.y), 10, 0x07FF); } esp_task_wdt_reset(); } currentState = DESKTOP; renderDesktop(); }
            else if (ty > 175 && ty < 195) { runJoyTest(); }
            else if (ty > 195 && ty < 225) { 
                tft.fillScreen(0x0000); tft.setTextColor(0xFFFF); tft.setCursor(50, 120); tft.print("I2S SINUS SES TESTI...");
                playToneI2S(440, 1000); // ARTIK CIZIRTI YOK, SİNÜS VAR
                delay(1000); currentState = DESKTOP; renderDesktop();
            }
            else { currentState = DESKTOP; renderDesktop(); delay(300); }
        }
        else if (currentState == POWER_MENU) {
            if (tx >= 85 && tx <= 255 && ty > 130 && ty < 165) { 
                globalVolume = ((tx - 85) * 100) / 170;
                if(globalVolume < 0) globalVolume = 0; if(globalVolume > 100) globalVolume = 100;
                prefs.putInt("vol", globalVolume); 
                tft.fillRect(85, 140, 170, 20, 0x0000); 
                tft.fillRect(85, 140, (globalVolume*170)/100, 20, 0x07E0); 
                tft.drawRect(85, 140, 170, 20, 0xFFFF);
                playToneI2S(1000, 50); // CIZIRTISIZ GERİ BİLDİRİM
                delay(100);
            }
            else if (tx > 50 && tx < 270 && ty > 55 && ty < 85) { playClick(); tft.fillScreen(0); tft.setCursor(100,120); tft.print("UYKU MODU..."); delay(1000); esp_deep_sleep_start(); }
            else if (tx > 50 && tx < 270 && ty > 95 && ty < 125) { playBeep(); ESP.restart(); }
            else if (tx > 50 && tx < 270 && ty > 175 && ty < 205) { playClick(); currentState = DESKTOP; renderDesktop(); delay(300); }
            else if (tx < 30 || tx > 290 || ty < 20 || ty > 220) { currentState = DESKTOP; renderDesktop(); delay(300); } 
        }
    }
}
