/* * Sovereign Wallpaper Engine - Win10 Hero Edition
 * Geliştirici: Muhammed (Kynex)
 * Görev: Windows 10 Duvar Kağıdını Saf Kodla Çizer.
 * Avantaj: 0 KB Bellek Kullanımı, %100 Stabilite.
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>

// Win10 Renk Paleti
#define WIN10_DARK_BLUE   0x0011
#define WIN10_MEDIUM_BLUE 0x0019
#define WIN10_LIGHT_BLUE  0x5DFF
#define WIN10_GLOW        0xADFF

void drawSovereignWallpaper(Adafruit_GFX *tft) {
    // 1. Gradyan Arka Plan Çizimi (Yukarıdan Aşağıya Derinlik)
    for (int i = 0; i < 240; i++) {
        // Basit bir renk geçişi simülasyonu
        uint16_t color = (i < 120) ? WIN10_DARK_BLUE : WIN10_MEDIUM_BLUE;
        tft->drawFastHLine(0, i, 320, color);
    }

    // 2. Windows "Hero" Işık Hüzmesi (Arka Plan Aydınlatması)
    for (int r = 0; r < 80; r++) {
        tft->drawCircle(160, 100, r, 0x0015); // Hafif bir parlama efekti
    }

    // 3. İkonik Windows Penceresi (Perspektifli Çizim)
    // Sol Kanat
    tft->fillRoundRect(120, 75, 35, 30, 2, WIN10_LIGHT_BLUE);
    tft->fillRoundRect(120, 110, 35, 30, 2, WIN10_LIGHT_BLUE);
    
    // Sağ Kanat (Daha Büyük ve Aydınlık)
    tft->fillRoundRect(160, 70, 45, 35, 2, WIN10_GLOW);
    tft->fillRoundRect(160, 110, 45, 35, 2, WIN10_GLOW);

    // Pencere Çizgileri (Haç işareti)
    tft->drawFastVLine(157, 70, 75, WIN10_DARK_BLUE); // Dikey boşluk
    tft->drawFastHLine(120, 107, 85, WIN10_DARK_BLUE); // Yatay boşluk
}

#endif
