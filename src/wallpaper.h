/* * Sovereign Wallpaper Engine - True Retro-Wave HD v230.48
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Smooth Sun Gradient, Wireframe Mountains, Perspective Grid
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h>

// HD Retro Renkler
#define RETRO_SKY     0x180E 
#define RETRO_PURPLE  0x3012 
#define RETRO_SUN_T   0xFDE0 
#define RETRO_SUN_B   0xF800 
#define RETRO_CYAN    0x07FF 
#define RETRO_MAGENTA 0xF81F

void drawRetroWallpaper(Adafruit_GFX *tft) {
    // 1. Gökyüzü Gradyanı
    for (int i = 0; i < 160; i++) {
        uint16_t c = (i < 80) ? RETRO_SKY : RETRO_PURPLE;
        tft->drawFastHLine(0, i, 320, c);
        if (i % 40 == 0) esp_task_wdt_reset();
    }

    // 2. Dev Retro Güneş (Gradyan Circle)
    for (int r = 0; r < 65; r++) {
        uint16_t sc = (r < 30) ? RETRO_SUN_T : RETRO_SUN_B;
        tft->drawCircle(160, 95, r, sc);
    }

    // 3. Dağlar (Piramitler)
    // Sol Dağ
    tft->fillTriangle(0, 160, 70, 110, 140, 160, 0x1004);
    tft->drawTriangle(0, 160, 70, 110, 140, 160, RETRO_MAGENTA);
    // Sağ Dağ
    tft->fillTriangle(180, 160, 250, 100, 320, 160, 0x1004);
    tft->drawTriangle(180, 160, 250, 100, 320, 160, RETRO_MAGENTA);

    // 4. Ufuk Çizgisi (Glow)
    tft->fillRect(0, 158, 320, 3, RETRO_CYAN);

    // 5. Zemin Izgarası (Perspective Grid)
    for (int y = 161; y < 240; y += 6) {
        tft->drawFastHLine(0, y, 320, RETRO_PURPLE);
    }
    for (int x = -100; x < 420; x += 20) {
        tft->drawLine(x, 240, 160, 161, RETRO_PURPLE);
    }
    esp_task_wdt_reset();
}

// Windows 10 Gerçek Logosu (4 Parçalı)
void drawWin10Logo(Adafruit_GFX *tft, int x, int y, int size) {
    int gap = 2;
    int half = size / 2;
    tft->fillRect(x, y, half-gap, half-gap, 0xFFFF);           // Sol Üst
    tft->fillRect(x, y+half, half-gap, half-gap, 0xFFFF);      // Sol Alt
    tft->fillRect(x+half, y-2, half-gap, half+2-gap, 0xFFFF);  // Sağ Üst (Perspektif)
    tft->fillRect(x+half, y+half, half-gap, half+2-gap, 0xFFFF); // Sağ Alt (Perspektif)
}

#endif
