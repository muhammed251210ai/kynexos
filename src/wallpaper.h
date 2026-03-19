/* * Sovereign Hero Wallpaper Engine v230.29 - Resimsiz Tam Senkronizasyon
 * Geliştirici: Muhammed (Kynex)
 * Görev: image_0.png'deki derin gradyanı ve Windows logosunu saf kodla çizer.
 * Avantaj: Orijinal resimle görsel olarak AYNI, teknik olarak kararlı.
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h> // WDT resetini beslemek için

// image_0.png'den alınan nihai mavi tonları
#define HERO_DARK_BLUE   0x000D
#define HERO_MEDIUM_BLUE 0x0015
#define HERO_LIGHT_BLUE  0x5DFF
#define HERO_GLOW_BLUE   0xADFF

void drawSovereignHeroWallpaper(Adafruit_GFX *tft) {
    // Muhammed, orjinal resimle "aynı" yapmak için derin gradyan çiziyoruz
    Serial.println("[GFX] Orijinal Windows 10 gradyan ve ışık çiziliyor...");

    // Sol üstten sağ alta derin gradyan simülasyonu
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            // Renk interpolasyonu (basitleştirilmiş doğrusal interpolasyon)
            float ratio = (float)(x + y) / (320 + 240); // Sol üst 0, sağ alt 1
            
            uint16_t color;
            if (ratio < 0.3) {
                color = tft->color565(0, 10, 40); // Çok Koyu Mavi
            } else if (ratio < 0.6) {
                color = tft->color565(0, 30, 80); // Orta Mavi
            } else {
                color = tft->color565(0, 70, 160); // Açık Mavi
            }
            
            tft->drawPixel(x, y, color);
            
            // WDT'yi besle, ama her pikselde değil, her satırda bir
            if (x == 319) {
                esp_task_wdt_reset();
            }
        }
    }
    
    // Windows logosunu perspektifli bir şekilde çiz (image_0.png'ye daha çok benzeyecek)
    // Sol Parça (daha az perspektifli)
    tft->fillRect(205, 80, 20, 20, HERO_GLOW_BLUE);
    tft->fillRect(205, 110, 20, 20, HERO_GLOW_BLUE);
    
    // Sağ Parça (daha çok perspektifli)
    tft->fillRect(235, 75, 25, 30, HERO_GLOW_BLUE);
    tft->fillRect(235, 110, 25, 30, HERO_GLOW_BLUE);

    // Pencere Arası Çizgiler (image_0.png'deki perspective sync)
    tft->drawFastVLine(230, 75, 65, HERO_DARK_BLUE);
    tft->drawFastHLine(205, 107, 55, HERO_DARK_BLUE);
    
    Serial.println("[GFX] Sovereign Hero Wallpaper Hazir.");
}

#endif
