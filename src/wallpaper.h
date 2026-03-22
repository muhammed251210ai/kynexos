/* * Sovereign Wallpaper Engine - HD v230.52
 * Geliştirici: Muhammed (Kynex)
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h>

#define RETRO_SKY     0x180E 
#define RETRO_PURPLE  0x3012 
#define RETRO_SUN_T   0xFDE0 
#define RETRO_SUN_B   0xF800 
#define RETRO_CYAN    0x07FF 
#define RETRO_MAGENTA 0xF81F

void drawRetroWallpaper(Adafruit_GFX *tft) {
    for (int i = 0; i < 160; i++) {
        tft->drawFastHLine(0, i, 320, (i < 80) ? RETRO_SKY : RETRO_PURPLE);
        if (i % 40 == 0) esp_task_wdt_reset();
    }
    for (int r = 0; r < 65; r++) {
        tft->drawCircle(160, 95, r, (r < 30) ? RETRO_SUN_T : RETRO_SUN_B);
    }
    tft->fillTriangle(0, 160, 70, 110, 140, 160, 0x1004);
    tft->drawTriangle(0, 160, 70, 110, 140, 160, RETRO_MAGENTA);
    tft->fillTriangle(180, 160, 250, 100, 320, 160, 0x1004);
    tft->drawTriangle(180, 160, 250, 100, 320, 160, RETRO_MAGENTA);
    tft->fillRect(0, 158, 320, 3, RETRO_CYAN);
    for (int y = 161; y < 240; y += 8) tft->drawFastHLine(0, y, 320, RETRO_PURPLE);
    for (int x = -100; x < 420; x += 25) tft->drawLine(x, 240, 160, 161, RETRO_PURPLE);
    esp_task_wdt_reset();
}

void drawWin10Logo(Adafruit_GFX *tft, int x, int y, int size) {
    int half = size / 2;
    tft->fillRect(x, y, half-1, half-1, 0xFFFF);
    tft->fillRect(x, y+half, half-1, half-1, 0xFFFF);
    tft->fillRect(x+half, y-1, half-1, half, 0xFFFF);
    tft->fillRect(x+half, y+half, half-1, half, 0xFFFF);
}

#endif
