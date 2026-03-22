/* * Sovereign Wallpaper Engine - Eternal Glow v230.37
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Deep Blue Gradient, Floating Cyber-Stars
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>

#define SOV_DARK   0x0008
#define SOV_BLUE   0x0014
#define SOV_GLOW   0x5DFF

struct SovereignStar {
    float x, y, speed;
    uint16_t color;
};

SovereignStar stars[20];

void initSovereignStars() {
    for (int i = 0; i < 20; i++) {
        stars[i].x = random(0, 320);
        stars[i].y = random(0, 240);
        stars[i].speed = (float)random(10, 30) / 10.0;
        stars[i].color = (random(0, 2) == 0) ? 0xFFFF : 0xADFF;
    }
}

void drawHeroBackground(Adafruit_GFX *tft) {
    // Muhammed, pürüzsüz geçiş için gradyan çiziyoruz
    for (int i = 0; i < 240; i++) {
        uint16_t c = (i < 120) ? SOV_DARK : SOV_BLUE;
        tft->drawFastHLine(0, i, 320, c);
    }
    // Windows Logosunun GFX hali
    tft->fillRect(220, 80, 25, 25, SOV_GLOW);
    tft->fillRect(220, 110, 25, 25, SOV_GLOW);
    tft->fillRect(250, 75, 30, 30, 0xADFF);
    tft->fillRect(250, 110, 30, 30, 0xADFF);
}

void updateStars(Adafruit_GFX *tft) {
    for (int i = 0; i < 20; i++) {
        // Eski yıldızı sil
        tft->drawPixel((int)stars[i].x, (int)stars[i].y, (stars[i].y < 120) ? SOV_DARK : SOV_BLUE);
        
        // Hareket
        stars[i].x -= stars[i].speed;
        if (stars[i].x < 0) {
            stars[i].x = 320;
            stars[i].y = random(0, 240);
        }
        
        // Yeni yıldızı çiz
        tft->drawPixel((int)stars[i].x, (int)stars[i].y, stars[i].color);
    }
}

#endif
