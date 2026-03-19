/* * Sovereign Wallpaper Engine - Win10 Hero Pure GFX
 * Geliştirici: Muhammed (Kynex)
 * Görev: Windows 10 Temasını Saf Kodla Çizer (Resim Dosyası Gerektirmez).
 * Avantaj: Sıfır RAM hatası, Maksimum Açılış Hızı.
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>

// Win10 Sovereign Renk Paleti
#define W10_DEEP_BLUE    0x0011
#define W10_DARK_BLUE    0x000D
#define W10_LIGHT_BLUE   0x5DFF
#define W10_PANE_GLOW    0xADFF

void drawWin10Background(Adafruit_GFX *tft) {
    // 1. Derinlik Efekti: Yukarıdan aşağıya basit gradyan simülasyonu
    for (int i = 0; i < 240; i++) {
        uint16_t line_color = (i < 100) ? W10_DARK_BLUE : W10_DEEP_BLUE;
        tft->drawFastHLine(0, i, 320, line_color);
    }

    // 2. Windows "Hero" Penceresi (Işık ve Perspektif)
    // Sol Üst Parça
    tft->fillRoundRect(125, 75, 30, 25, 1, W10_LIGHT_BLUE);
    // Sol Alt Parça
    tft->fillRoundRect(125, 105, 30, 25, 1, W10_LIGHT_BLUE);
    
    // Sağ Üst Parça (Daha Parlak)
    tft->fillRoundRect(160, 70, 40, 30, 1, W10_PANE_GLOW);
    // Sağ Alt Parça (Daha Parlak)
    tft->fillRoundRect(160, 105, 40, 30, 1, W10_PANE_GLOW);

    // 3. Işık Hüzmesi Efekti (Pencere Arasındaki Boşluklar)
    tft->drawFastVLine(157, 70, 65, W10_DARK_BLUE);
    tft->drawFastHLine(125, 102, 75, W10_DARK_BLUE);
    
    Serial.println("[WALLPAPER] Win10 Hero Render Edildi.");
}

#endif
