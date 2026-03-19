/* * Sovereign Wallpaper Data
 * Geliştirici: Muhammed (Kynex)
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <pgmspace.h>

// MUHAMMED: Bu iki değişken adı main.cpp ile tam olarak eşleşmeli!
const unsigned char wallpaper_jpg[] PROGMEM __attribute__((aligned(4))) = {
    // BURAYA HEX KODLARINI YAPIŞTIR (0xFF, 0xD8 ile başlayan kodlar)

};

const unsigned int wallpaper_jpg_len = sizeof(wallpaper_jpg); 

#endif
