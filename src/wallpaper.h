/* * Sovereign Wallpaper Engine - Theme & Animation Sync v230.41
 * Geliştirici: Muhammed (Kynex)
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h>

struct SovereignStar {
    float x, y, speed;
    uint16_t color;
};

SovereignStar stars[15];

void initSovereignWallpaper() {
    for (int i = 0; i < 15; i++) {
        stars[i].x = random(0, 320);
        stars[i].y = random(0, 240);
        stars[i].speed = (float)random(10, 25) / 10.0;
        stars[i].color = 0xADFF;
    }
}

void drawHeroBackground(Adafruit_GFX *tft, bool whiteTheme) {
    uint16_t topColor = whiteTheme ? 0xFFFF : 0x0008;
    uint16_t botColor = whiteTheme ? 0xDEFB : 0x0014;
    uint16_t winColor = whiteTheme ? 0x03FF : 0x5DFF;

    for (int i = 0; i < 240; i++) {
        tft->drawFastHLine(0, i, 320, (i < 120) ? topColor : botColor);
        if (i % 20 == 0) esp_task_wdt_reset();
    }
    
    // Windows Logosunun GFX hali (Perspektif)
    tft->fillRect(220, 80, 25, 25, winColor);
    tft->fillRect(220, 110, 25, 25, winColor);
    tft->fillRect(250, 75, 30, 30, winColor);
    tft->fillRect(250, 110, 30, 30, winColor);
}

void updateStars(Adafruit_GFX *tft, bool whiteTheme) {
    uint16_t topColor = whiteTheme ? 0xFFFF : 0x0008;
    uint16_t botColor = whiteTheme ? 0xDEFB : 0x0014;
    for (int i = 0; i < 15; i++) {
        tft->drawPixel((int)stars[i].x, (int)stars[i].y, (stars[i].y < 120) ? topColor : botColor);
        stars[i].x -= stars[i].speed;
        if (stars[i].x < 0) {
            stars[i].x = 320;
            stars[i].y = random(0, 240);
        }
        tft->drawPixel((int)stars[i].x, (int)stars[i].y, whiteTheme ? 0x0011 : 0xFFFF);
    }
}

#endif
