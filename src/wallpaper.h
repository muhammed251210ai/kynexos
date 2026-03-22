/* * Sovereign Wallpaper Engine - Retro-Wave GFX Sync v230.43
 * Geliştirici: Muhammed (Kynex)
 * Özellikler: Smooth GFX Rendering, RGB565 High-Contrast Colors
 * Donanım: 8MB PSRAM Optimized, Safe WDT Beslemesi
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <Adafruit_GFX.h>
#include <esp_task_wdt.h>

// MUHAMMED: Retro-Wave Canlı RGB565 Renk Paleti (BW hatasını önlemek için High-Contrast)
#define R_DEEP      0x180E // Çok Koyu Mor (Gökyüzü Tepe)
#define R_PURPLE    0x3012 // Orta Mor
#define R_MAGENTA   0x8010 // Pembe/Eflatun (Ufuk)
#define R_SUN_TOP   0xFDE0 // Sarı (Güneş Tepe)
#define R_SUN_MID   0xFB00 // Turuncu
#define R_SUN_BOT   0xF800 // Kırmızı/Pembe (Güneş Alt)
#define R_CYAN      0x03BF // Elektrik Mavisi (Dağ Çizgileri)
#define R_PINK_GLOW 0xA019 // Ufuk Işığı Huzmesi

void drawRetroWaveWallpaper(Adafruit_GFX *tft, bool whiteTheme) {
    uint16_t skyTop = whiteTheme ? 0xFFFF : R_DEEP;
    uint16_t skyBot = whiteTheme ? 0xDEFB : R_PURPLE;
    uint16_t sunColor = R_SUN_TOP; 

    // Muhammed, ekranı soluk göstermemek için piksel piksel gradyan çiziyoruz
    for (int i = 0; i < 240; i++) {
        uint16_t color = (i < 120) ? skyTop : skyBot;
        tft->drawFastHLine(0, i, 320, color);
        if (i % 20 == 0) esp_task_wdt_reset(); // Çizim uzun süreceği için Watchdog'u sık sık besle!
    }

    // 2. Gradyan Güneş (Devasa ve Canlı)
    // tft->fillCircle(200, 100, 70, R_SUN_MID); // Basit güneş (HD hissi için gradyan uygulayın)
    for (int r = 0; r < 70; r++) {
        uint16_t color;
        if (r < 25) color = R_SUN_TOP;
        else if (r < 50) color = R_SUN_MID;
        else color = R_SUN_BOT;
        tft->drawCircle(200, 100, r, color);
    }
    esp_task_wdt_reset();

    // 3. Wireframe Piramit Dağlar (Tel Kafes)
    // Koyu mor dolgu
    tft->fillTriangle(10, 200, 90, 140, 170, 200, R_PURPLE); // Büyük Sol
    tft->fillTriangle(150, 200, 230, 130, 310, 200, R_PURPLE); // Büyük Sağ
    tft->fillTriangle(110, 200, 150, 170, 190, 200, R_PURPLE); // Küçük Orta
    // Tel kafes siyan çizgiler
    tft->drawTriangle(10, 200, 90, 140, 170, 200, R_CYAN);
    tft->drawTriangle(150, 200, 230, 130, 310, 200, R_CYAN);
    tft->drawTriangle(110, 200, 150, 170, 190, 200, R_CYAN);
    // Piramitlerin dikey perspektif çizgileri
    tft->drawFastVLine(90, 140, 60, R_CYAN);
    tft->drawFastVLine(230, 130, 70, R_CYAN);
    tft->drawFastVLine(150, 170, 30, R_CYAN);
    esp_task_wdt_reset();

    // 4. Pembe/Magenta Ufuk Işığı (Hafif Gradyan)
    tft->fillRect(0, 198, 320, 4, R_MAGENTA);
    tft->fillRect(0, 199, 320, 2, R_PINK_GLOW);

    // 5. Perspektif Grid Zemin (Sürekli GFX Beslemesi)
    // Yatay çizgiler (Görüntü kalitesi için high-contrast)
    for (int y = 202; y < 240; y += 4) {
        tft->drawFastHLine(0, y, 320, R_CYAN);
    }
    // Perspektif dikey çizgiler (Sürekli GFX Beslemesi)
    for (int x = -100; x < 420; x += 15) {
        tft->drawLine(x, 240, 160, 202, R_CYAN);
    }
    esp_task_wdt_reset();
}

// Önceki yıldız animasyonunu Retro temaya entegre ediyoruz
void updateStarsRetro(Adafruit_GFX *tft, bool whiteTheme) {
    static int starX = 320;
    static int starY = 100;
    uint16_t clearColor = (starY < 120) ? (whiteTheme ? 0xFFFF : R_DEEP) : (whiteTheme ? 0xDEFB : R_PURPLE);
    
    tft->drawPixel(starX, starY, clearColor); // Temizle
    starX -= 2; // Daha yavaş, HD hissi
    if (starX < 0) { starX = 320; starY = random(20, 200); }
    tft->drawPixel(starX, starY, whiteTheme ? 0x0000 : 0xFFFF); // Çiz
}

#endif
