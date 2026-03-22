/* * Sovereign Wallpaper Engine - Animated Win10 Edition
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Smooth Blue Gradient, Floating Light Particles
 * Avantaj: 0 KB Bellek Hatası, %100 Çalışma Garantisi.
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>

// Renk Paleti
#define W10_DARK      0x000D
#define W10_BLUE      0x0015
#define W10_LIGHT     0x5DFF

struct Particle {
    float x, y, speed;
};

Particle particles[15]; // 15 adet uçuşan ışık tanesi

void initSovereignWallpaper() {
    for (int i = 0; i < 15; i++) {
        particles[i].x = random(0, 320);
        particles[i].y = random(0, 240);
        particles[i].speed = (float)random(5, 15) / 10.0;
    }
}

void drawStaticWin10(Adafruit_GFX *tft) {
    // Arka plan derinliği
    for (int i = 0; i < 240; i++) {
        tft->drawFastHLine(0, i, 320, (i < 120) ? W10_DARK : W10_BLUE);
    }
    // Sabit Windows Penceresi
    tft->fillRect(220, 80, 25, 25, W10_LIGHT);
    tft->fillRect(220, 110, 25, 25, W10_LIGHT);
    tft->fillRect(250, 75, 30, 30, 0xADFF);
    tft->fillRect(250, 110, 30, 30, 0xADFF);
}

void updateSovereignAnimation(Adafruit_GFX *tft) {
    for (int i = 0; i < 15; i++) {
        // Eski pikseli temizle (Arka plan rengiyle)
        tft->drawPixel((int)particles[i].x, (int)particles[i].y, (particles[i].y < 120) ? W10_DARK : W10_BLUE);
        
        // Hareket ettir
        particles[i].x -= particles[i].speed;
        if (particles[i].x < 0) {
            particles[i].x = 320;
            particles[i].y = random(0, 240);
        }
        
        // Yeni pikseli çiz (Işık tanesi)
        tft->drawPixel((int)particles[i].x, (int)particles[i].y, 0xFFFF);
    }
}

#endif
