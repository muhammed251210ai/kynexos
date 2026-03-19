/* * Sovereign Wallpaper Engine - Win10 Pure GFX v230.23
 * Geliştirici: Muhammed (Kynex)
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>

#define W10_DARK  0x0011
#define W10_BLUE  0x0019
#define W10_LIGHT 0x5DFF

void drawWin10Background(Adafruit_GFX *tft) {
    // Gradyan Arka Plan
    for (int i = 0; i < 240; i++) {
        tft->drawFastHLine(0, i, 320, (i < 120) ? W10_DARK : W10_BLUE);
    }
    // Windows Logosunun basitleştirilmiş hali
    tft->fillRect(135, 85, 20, 20, W10_LIGHT);
    tft->fillRect(135, 110, 20, 20, W10_LIGHT);
    tft->fillRect(160, 80, 25, 25, 0xADFF);
    tft->fillRect(160, 110, 25, 25, 0xADFF);
    
    Serial.println("[GFX] Wallpaper Render Edildi.");
}

#endif
