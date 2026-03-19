/* * Sovereign Hero Wallpaper Engine v230.30 - True Smooth Gradient
 * Geliştirici: Muhammed (Kynex)
 * Hata Giderildi: Adafruit_GFX color565 hatası çözüldü (Kendi RGB565 motorumuz eklendi).
 * Geliştirme: Şeritli renkler yerine gerçeğe en yakın pürüzsüz gradyan eklendi.
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h>

// MUHAMMED: GFX sınıfında olmayan color565 fonksiyonunu kendimiz tanımlıyoruz.
// Bu sayede derleyici asla hata vermez!
uint16_t heroColor565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void drawSovereignHeroWallpaper(Adafruit_GFX *tft) {
    Serial.println("[GFX] Orijinal Windows 10 gradyan ve isik ciziliyor...");

    // 1. Gerçek Pürüzsüz Gradyan (Smooth Gradient Simülasyonu)
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            // Işığın merkezini (sağ alt) baz alan yumuşak geçiş matematiği
            float ratio = (float)(x + y) / 560.0; 
            
            uint8_t r = 0;
            uint8_t g = (uint8_t)(100.0 * ratio);      // Yeşile çalan açık mavi (Cyan efekti)
            uint8_t b = 40 + (uint8_t)(215.0 * ratio); // Koyu maviden parlak maviye geçiş
            
            tft->drawPixel(x, y, heroColor565(r, g, b));
            
            // Bekçi köpeğini sadece satır sonlarında besle (Çizim hızı düşmesin)
            if (x == 319) {
                esp_task_wdt_reset();
            }
        }
    }
    
    // 2. Windows Hero Penceresi (Orijinal resimdeki gibi Parlak Mavi - Glow)
    uint16_t window_color = heroColor565(50, 180, 255); // Çok parlak açık mavi
    uint16_t dark_gap = heroColor565(0, 30, 90);        // Aradaki koyu derinlik boşlukları
    
    // Sol Parçalar (Daha küçük - Perspektif Hissi)
    tft->fillRect(130, 85, 30, 28, window_color);
    tft->fillRect(130, 118, 30, 28, window_color);
    
    // Sağ Parçalar (Işığa daha yakın, daha büyük)
    tft->fillRect(165, 78, 40, 35, window_color);
    tft->fillRect(165, 118, 40, 35, window_color);

    // Aradaki Kesici Çizgiler (Pencereyi 4'e bölen derinlik)
    tft->fillRect(160, 78, 5, 75, dark_gap);   // Dikey boşluk
    tft->fillRect(130, 113, 75, 5, dark_gap);  // Yatay boşluk
    
    Serial.println("[GFX] Sovereign Hero Wallpaper Hazir.");
}

#endif
