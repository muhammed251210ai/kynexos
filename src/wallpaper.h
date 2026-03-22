/* * Sovereign Wallpaper Engine - Win10 Absolute v230.100
 * Geliştirici: Muhammed (Kynex)
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h>

void drawWin10HeroWallpaper(Adafruit_GFX *tft, bool whiteTheme) {
    uint16_t c_dark = whiteTheme ? 0xFFFF : 0x000B;
    uint16_t c_mid  = whiteTheme ? 0xDEFB : 0x0015;
    uint16_t c_glow = whiteTheme ? 0x03FF : 0x5DFF;
    uint16_t c_logo = whiteTheme ? 0x0000 : 0xFFFF;

    for (int i = 0; i < 215; i++) { // Taskbar sınırına kadar çiz
        uint16_t color = (i < 100) ? c_dark : c_mid;
        tft->drawFastHLine(0, i, 320, color);
        if (i % 40 == 0) esp_task_wdt_reset();
    }

    if (!whiteTheme) {
        for(int i = 0; i < 40; i++) {
            tft->drawLine(160, 100, 160 + (i*4), 0, c_mid | 0x0808);
            tft->drawLine(160, 100, 160 - (i*4), 215, c_mid | 0x0808);
        }
    }

    // Windows 10 Hero Logo
    int lx = 145, ly = 85, ls = 36;
    int gap = 3, h = ls / 2;
    tft->fillRect(lx, ly, h-gap, h-gap, c_glow);
    tft->fillRect(lx, ly+h, h-gap, h-gap, c_glow);
    tft->fillRect(lx+h, ly-4, h-gap, h+4-gap, c_logo);
    tft->fillRect(lx+h, ly+h, h-gap, h+4-gap, c_logo);
    tft->drawCircle(160, 105, 45, c_glow);
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
