/* * Sovereign Wallpaper Engine - The Refined UI v230.118
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Orijinal 3D Perspektifli Windows 10 Logosu
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h>

void drawRealWin10Logo(Adafruit_GFX *tft, int x, int y, int w, int h, uint16_t color) {
    int gap = (w > 25) ? 3 : 1; 
    float slope = 0.16; 
    for (int px = 0; px < w; px++) {
        if (px >= (w/2) - gap && px <= (w/2) + gap) continue; 
        int yOffset = (w - px) * slope; 
        int topY = y + yOffset;
        int topH = (h / 2) - gap - yOffset;
        if (topH > 0) tft->drawFastVLine(x + px, topY, topH, color);
        int botY = y + (h / 2) + gap;
        int botH = (h / 2) - gap - yOffset;
        if (botH > 0) tft->drawFastVLine(x + px, botY, botH, color);
    }
}

void drawWin10HeroWallpaper(Adafruit_GFX *tft, bool whiteTheme) {
    uint16_t c_dark = whiteTheme ? 0xFFFF : 0x000B;
    uint16_t c_mid  = whiteTheme ? 0xDEFB : 0x0015;
    uint16_t c_glow = whiteTheme ? 0x03FF : 0x5DFF;
    uint16_t c_logo = whiteTheme ? 0x0000 : 0xFFFF;

    for (int i = 0; i < 215; i++) {
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
    tft->fillCircle(160, 105, 45, c_glow | 0x1010);
    tft->fillCircle(160, 105, 25, c_glow);
    drawRealWin10Logo(tft, 130, 75, 60, 60, c_logo);
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
