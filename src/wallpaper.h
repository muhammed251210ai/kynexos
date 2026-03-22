/* * Sovereign Wallpaper Engine - Animated Win10 Edition v230.38
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Smooth Blue Gradient, Floating Stars
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>

#define SOV_DARK   0x0008
#define SOV_BLUE   0x0014
#define SOV_GLOW   0x5DFF

struct Particle {
    float x, y, speed;
};

Particle particles[15];

void initSovereignWallpaper() {
    for (int i = 0; i < 15; i++) {
        particles[i].x = random(0, 320);
        particles[i].y = random(0, 240);
        particles[i].speed = (float)random(10, 25) / 10.0;
    }
}

void drawHeroBackground(Adafruit_GFX *tft) {
    for (int i = 0; i < 240; i++) {
        tft->drawFastHLine(0, i, 320, (i < 120) ? SOV_DARK : SOV_BLUE);
    }
    // Windows Logosunu Çiz
    tft->fillRect(220, 80, 25, 25, SOV_GLOW);
    tft->fillRect(220, 110, 25, 25, SOV_GLOW);
    tft->fillRect(250, 75, 30, 30, 0xADFF);
    tft->fillRect(250, 110, 30, 30, 0xADFF);
}

void updateAnimation(Adafruit_GFX *tft) {
    for (int i = 0; i < 15; i++) {
        tft->drawPixel((int)particles[i].x, (int)particles[i].y, (particles[i].y < 120) ? SOV_DARK : SOV_BLUE);
        particles[i].x -= particles[i].speed;
        if (particles[i].x < 0) {
            particles[i].x = 320;
            particles[i].y = random(0, 240);
        }
        tft->drawPixel((int)particles[i].x, (int)particles[i].y, 0xFFFF);
    }
}

#endif
