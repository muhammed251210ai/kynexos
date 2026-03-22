/* * Sovereign Wallpaper Engine - The True Identity v230.101
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Orijinal 3D Perspektifli Windows 10 Logosu (Matematiksel Çizim)
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h>

// GERÇEK PERSPEKTİFLİ WIN10 LOGO ÇİZİM MOTORU
void drawRealWin10Logo(Adafruit_GFX *tft, int x, int y, int w, int h, uint16_t color) {
    int gap = (w > 25) ? 3 : 1; // Logoya göre dinamik yatay/dikey pencere boşluğu
    float slope = 0.16; // 3D Perspektif eğimi (Sağa doğru açılma açısı)
    
    for (int px = 0; px < w; px++) {
        // Dikey orta pencere boşluğu (Haçın dikey çizgisi)
        if (px >= (w/2) - gap && px <= (w/2) + gap) continue; 
        
        // Solda çok eğim, sağda sıfır eğim (Perspektif etkisi)
        int yOffset = (w - px) * slope; 
        
        // Üst Pencere Camı
        int topY = y + yOffset;
        int topH = (h / 2) - gap - yOffset;
        if (topH > 0) tft->drawFastVLine(x + px, topY, topH, color);
        
        // Alt Pencere Camı
        int botY = y + (h / 2) + gap;
        int botH = (h / 2) - gap - yOffset;
        if (botH > 0) tft->drawFastVLine(x + px, botY, botH, color);
    }
}

void drawWin10HeroWallpaper(Adafruit_GFX *tft, bool whiteTheme) {
    uint16_t c_dark = whiteTheme ? 0xFFFF : 0x000B;
    uint16_t c_mid  = whiteTheme ? 0xDEFB : 0x0015;
    uint16_t c_glow = whiteTheme ? 0x03FF : 0x5DFF;
    uint16_t c_logo = whiteTheme ? 0x0000 : 0xFFFF;

    // Arka Plan Derinliği
    for (int i = 0; i < 215; i++) {
        uint16_t color = (i < 100) ? c_dark : c_mid;
        tft->drawFastHLine(0, i, 320, color);
        if (i % 40 == 0) esp_task_wdt_reset();
    }

    // Hero Işık Huzmeleri (Sadece Koyu Temada)
    if (!whiteTheme) {
        for(int i = 0; i < 40; i++) {
            tft->drawLine(160, 100, 160 + (i*4), 0, c_mid | 0x0808);
            tft->drawLine(160, 100, 160 - (i*4), 215, c_mid | 0x0808);
        }
    }

    // Merkez Glow Efekti (Pencerelerin arkasından vuran ışık)
    tft->fillCircle(160, 105, 45, c_glow | 0x1010);
    tft->fillCircle(160, 105, 25, c_glow);

    // DEV ORİJİNAL LOGO (Merkezde)
    drawRealWin10Logo(tft, 130, 75, 60, 60, c_logo);
    esp_task_wdt_reset();
}

#endif
