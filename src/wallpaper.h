/* * Sovereign Wallpaper Engine - Win10 Hero HD v230.57
 * Geliştirici: Muhammed (Kynex)
 * Görev: Win10 Hero Duvar Kağıdı ve Gerçek Logo Çizimi
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h>

#define W10_DARK      0x000B 
#define W10_BLUE      0x0015 
#define W10_GLOW      0x5DFF 
#define W10_WHITE     0xFFFF 

// MUHAMMED: İsim uyuşmazlığı hatasını önlemek için isimleri eşledik
void drawWin10HeroWallpaper(Adafruit_GFX *tft) {
    for (int i = 0; i < 240; i++) {
        uint16_t color = (i < 120) ? W10_DARK : W10_BLUE;
        tft->drawFastHLine(0, i, 320, color);
        if (i % 40 == 0) esp_task_wdt_reset();
    }
    // Işık Efektleri
    for(int i = 0; i < 40; i++) {
        tft->drawLine(160, 120, 160 + (i*4), 0, W10_BLUE | 0x0808);
        tft->drawLine(160, 120, 160 - (i*4), 240, W10_BLUE | 0x0808);
    }
    // Ana Logo
    int lx = 145, ly = 95, ls = 36;
    int gap = 3, h = ls / 2;
    tft->fillRect(lx, ly, h-gap, h-gap, W10_GLOW);
    tft->fillRect(lx, ly+h, h-gap, h-gap, W10_GLOW);
    tft->fillRect(lx+h, ly-4, h-gap, h+4-gap, W10_WHITE);
    tft->fillRect(lx+h, ly+h, h-gap, h+4-gap, W10_WHITE);
    tft->drawCircle(160, 115, 45, W10_GLOW);
    esp_task_wdt_reset();
}

void drawWin10SmallLogo(Adafruit_GFX *tft, int x, int y, int size) {
    int half = size / 2;
    tft->fillRect(x, y, half-1, half-1, 0xFFFF);
    tft->fillRect(x, y+half, half-1, half-1, 0xFFFF);
    tft->fillRect(x+half, y-1, half-1, half, 0xFFFF);
    tft->fillRect(x+half, y+half, half-1, half, 0xFFFF);
}

#endif
