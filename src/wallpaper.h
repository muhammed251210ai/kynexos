/* * Sovereign Wallpaper Engine - Windows 10 Hero HD v230.56
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Deep Blue Gradient, Light Beams, High-Contrast Win10 Logo
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h>

// Windows 10 Renk Paleti
#define W10_DARK_BLUE 0x000B // Çok Koyu Mavi
#define W10_MID_BLUE  0x0015 // Orta Mavi
#define W10_GLOW      0x5DFF // Hero Işığı
#define W10_WHITE     0xFFFF 

void drawWin10HeroWallpaper(Adafruit_GFX *tft) {
    // 1. Derinlikli Gradyan Arka Plan
    for (int i = 0; i < 240; i++) {
        uint16_t color = (i < 120) ? W10_DARK_BLUE : W10_MID_BLUE;
        tft->drawFastHLine(0, i, 320, color);
        if (i % 30 == 0) esp_task_wdt_reset();
    }

    // 2. Hero Işık Huzmeleri (Radial Glow Simülasyonu)
    for(int i = 0; i < 60; i++) {
        tft->drawLine(160, 120, 160 + (i*3), 0, W10_MID_BLUE | 0x1010);
        tft->drawLine(160, 120, 160 - (i*3), 240, W10_MID_BLUE | 0x1010);
    }

    // 3. Keskin Windows 10 Logosu (Merkezi Glowlu)
    int lx = 145, ly = 95, ls = 35;
    int gap = 3;
    int h = ls / 2;
    
    // Logoyu Işık Huzmesiyle Çiz
    tft->fillRect(lx, ly, h-gap, h-gap, W10_GLOW);           // Sol Üst
    tft->fillRect(lx, ly+h, h-gap, h-gap, W10_GLOW);         // Sol Alt
    tft->fillRect(lx+h, ly-4, h-gap, h+4-gap, W10_WHITE);    // Sağ Üst (Perspektif)
    tft->fillRect(lx+h, ly+h, h-gap, h+4-gap, W10_WHITE);    // Sağ Alt (Perspektif)

    // Orta Işık Efekti
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
