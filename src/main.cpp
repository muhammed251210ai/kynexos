/* * KynexOs v105.0 - Full Integrated System
 * Geliştirici: Muhammed (Klynex)
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
#include <Preferences.h>
#include <FS.h>
#include <FFat.h>
#include "time.h"

// --- RENK PALETİ ---
#define COLOR_BG      0xFFFF
#define COLOR_TEXT    0x0000
#define COLOR_ACCENT  0x03EF
#define COLOR_BAR     0xF7BE
#define COLOR_BTN     0xE71C
#define COLOR_WHITE   0xFFFF
#define COLOR_CLICK   0xAD55
#define COLOR_LINE    0xDEDB
#define COLOR_RED     0xF800
#define COLOR_RETRO   0x3186
#define COLOR_BLACK   0x0000

// --- SİSTEM DEĞİŞKENLERİ ---
int currentScreen = 0; 
String selectedFile = "";
int menuCursor = 0; 

// Wallpaper binary tanımı
extern const uint8_t jpg_start[] asm("_binary_src_my_wallpaper_jpg_start");
extern const uint8_t jpg_end[]   asm("_binary_src_my_wallpaper_jpg_end");

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS); 
WebServer server(80);

// --- FONKSİYONLAR ---
void playBootSound() {
    ledcWriteTone(0, 800); delay(100);
    ledcWriteTone(0, 1200); delay(150);
    ledcWriteTone(0, 0);
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return false;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return true;
}

// Web Server İşleyicileri
void handleRoot() {
    String html = "<html><body style='font-family:sans-serif; background:#f0f0f0;'>";
    html += "<h1>KynexOs Admin Panel</h1><p>N16R8 Kapasite: 8MB PSRAM OK</p>";
    html += "<h3>Dosya Listesi:</h3><ul>";
    File root = FFat.open("/");
    File file = root.openNextFile();
    while(file){
        html += "<li>" + String(file.name()) + " (" + String(file.size()) + " byte)</li>";
        file = root.openNextFile();
    }
    html += "</ul><hr><form method='POST' action='/upload' enctype='multipart/form-data'>";
    html += "<input type='file' name='u'><button type='submit'>Karta Yukle</button></form></body></html>";
    server.send(200, "text/html", html);
}

void handleUpload() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        selectedFile = upload.filename;
        if (!selectedFile.startsWith("/")) selectedFile = "/" + selectedFile;
        File f = FFat.open(selectedFile, FILE_WRITE); f.close();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        File f = FFat.open(selectedFile, FILE_APPEND); f.write(upload.buf, upload.currentSize); f.close();
    } else if (upload.status == UPLOAD_FILE_END) {
        server.sendHeader("Location", "/"); server.send(303);
    }
}

// --- UI ÇİZİMLERİ ---
void drawStatusBar() {
    tft.fillRect(0, 0, 320, 22, COLOR_BAR);
    tft.setTextColor(COLOR_TEXT); tft.setTextSize(1); tft.setCursor(10, 7);
    tft.print(WiFi.status() == WL_CONNECTED ? "BAGLI" : "AP: KYNEX (192.168.4.1)");
}

void drawNavBar(int act) {
    tft.fillRect(0, 210, 320, 30, COLOR_BAR);
    tft.drawFastHLine(0, 210, 320, COLOR_LINE);
    tft.drawCircle(160, 225, 9, (act == 1 ? COLOR_CLICK : COLOR_TEXT)); // Ana sayfa butonu
}

void drawDesktop() {
    tft.fillScreen(COLOR_BG); 
    drawStatusBar();
    
    // Uygulama İkonları
    tft.fillRoundRect(30, 60, 50, 50, 12, 0x4A69);
    tft.setCursor(32, 115); tft.setTextColor(COLOR_TEXT); tft.print("Ayarlar");

    tft.fillRoundRect(110, 60, 50, 50, 12, 0xFD20);
    tft.setCursor(112, 115); tft.print("Dosyalar");

    tft.fillRoundRect(190, 60, 50, 50, 12, COLOR_RETRO);
    tft.setCursor(192, 115); tft.print("RetroGo");
    
    drawNavBar(-1);
}

void drawRetroMenu() {
    tft.fillScreen(COLOR_BLACK);
    tft.fillRect(0, 0, 320, 25, COLOR_RETRO);
    tft.setTextColor(COLOR_WHITE); tft.setCursor(10, 8); tft.print("RETROGO v2.0 - OPI PSRAM ACTIVE");
    tft.setCursor(20, 100); tft.print("NES Oyunlari yukleniyor...");
    drawNavBar(1);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("KYNEXOS BOOTING...");

    // PSRAM Onayı
    if(psramInit()) { Serial.println("PSRAM OK!"); }

    // Dosya Sistemi
    if(!FFat.begin(true)) { FFat.format(); }

    // WiFi AP Modu
    WiFi.softAP("KYNEX", "*muhammed*krid*");
    server.on("/", handleRoot);
    server.on("/upload", HTTP_POST, [](){ server.send(200); }, handleUpload);
    server.begin();

    // Donanım Başlatma (Hata vermeyen güvenli pinler)
    SPI.begin(TFT_SCK, MISO_PIN, TFT_MOSI, TFT_CS); 
    tft.begin(); 
    tft.setRotation(1);
    ts.begin(); 
    ts.setRotation(1);
    
    // Hoparlör setup
    ledcAttachPin(SPEAKER_PIN, 0);
    playBootSound();

    drawDesktop();
    Serial.println("KYNEXOS SISTEM HAZIR!");
}

void loop() {
    server.handleClient();
    
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        // Kalibrasyon değerlerini senin ekranına göre ayarlayabilirsin
        int tx = map(p.x, 3850, 200, 0, 320); 
        int ty = map(p.y, 3800, 240, 0, 240);
        
        // Nav bar kontrolü
        if (ty > 210 && tx > 130 && tx < 190) { 
            currentScreen = 0; drawDesktop(); delay(200); 
        }
        
        // İkon tıklama
        if (currentScreen == 0) {
            if (tx > 180 && tx < 245 && ty > 55 && ty < 110) { 
                currentScreen = 10; drawRetroMenu(); delay(200); 
            }
        }
    }
}
