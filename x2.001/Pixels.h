/* 
 *  Pixels.h - Deklaration
 *  Pixels.cpp - Definition
 *  ***
 *  Die eigentliche Uhr
 *  ***
 *  Michel 02.11.2023
 */

#pragma once

/* 
 *  LED Farben definieren
 *  HSV...
 *  H == Farbe HUE 0 - 65535 0 - 360 Grad
 *  S == Sättigung SAT 0- 255 0 - 100% 0 == ws >0 == color
 *  V == Helligkeit VAL 0 - 255 /  0 - 100%
 */

/* Farbe der Uhr */
/* default >> WHITE */
extern uint16_t d_UhrH;
extern uint8_t d_UhrS;
extern uint8_t d_UhrV;
/* current */
extern uint16_t c_UhrH;
extern uint8_t c_UhrS;
extern uint8_t c_UhrV;

/* Farbe der Trenner */
/* default >> ROT */
extern uint16_t d_SepH;
extern uint8_t d_SepS;
extern uint8_t d_SepV;
/* current */
extern uint16_t c_SepH;
extern uint8_t c_SepS;
extern uint8_t c_SepV;

/*
 * Seperator State
 * 0 == nie an
 * 1 == flash original
 * 2 == immer an 
 * d_SepState >> default 1
 * c_SepState >> current
 */
extern int d_SepState;
extern int c_SepState;

void BeginPixels();
void ShowPixels();
void MakePixels();

// ENDE
