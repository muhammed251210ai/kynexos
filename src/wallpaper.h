/* * Sovereign Wallpaper Engine - Retro-Wave & Theme Sync v230.44
 * Geliştirici: Muhammed (Kynex)
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h>

// Renk Tanımları
#define R_DEEP      0x180E 
#define R_PURPLE    0x3012 
#define R_SUN_MID   0xFB00 
#define R_CYAN      0x03BF 
#define W_BG        0xFFFF 
#define W_ACCENT    0x03FF 

void drawSovereignWallpaper(Adafruit_GFX *tft, bool whiteTheme) {
    if (whiteTheme) {
        tft->fillScreen(W_BG);
        // Modern Minimalist Arka Plan
        for(int i=0; i<240; i+=40) tft->drawFastHLine(0, i, 320, 0xDEFB);
        Serial.println("[GFX] Beyaz Tema Render Edildi.");
    } else {
        // Full Retro-Wave Render
        for (int i = 0; i < 240; i++) {
            uint16_t color = (i < 120) ? R_DEEP : R_PURPLE;
            tft->drawFastHLine(0, i, 320, color);
            if (i % 30 == 0) esp_task_wdt_reset();
        }
        for (int r = 0; r < 60; r++) tft->drawCircle(200, 90, r, R_SUN_MID);
        for (int y = 200; y < 240; y += 5) tft->drawFastHLine(0, y, 320, R_CYAN);
        Serial.println("[GFX] Retro-Wave Render Edildi.");
    }
}
#endif
