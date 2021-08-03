#include <Arduino.h>

#ifndef HEATIMG_H
#define HEATIMG_H

// Значок нагрева воды
static const unsigned char PROGMEM heatImg[] =
    {
        B00001100, B00100000,
        B00001000, B01100000,
        B00011000, B01100000,
        B00011000, B11100000,
        B00011100, B11100000,
        B00011100, B01100000,
        B00001110, B01110000,
        B00001110, B00110000,
        B00000110, B00110000,
        B00000110, B00110000,
        B00000100, B00110000,
        B00001100, B00100000,
        B00001000, B01000000,
        B00000000, B00000000,
        B00111111, B11111100,
        B00111111, B11111100,
        B00111111, B11111100,
        B00000000, B00000000};

#endif
