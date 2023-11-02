/* 
 *  Pixels.h - Deklaration
 *  Pixels.cpp - Definition
 *  ***
 *  Die eigentliche Uhr
 *  ***
 *  Michel 02.11.2023
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "GlobalVars.h"
#include "Pixels.h" // Deklaration

/* Pin für den Data Anschluss */
const int UHR_PIN = D10;
/* Anzahl Led's */
const int NUMPIXELS = 46;

/* 
 *  LED Farben definieren
 *  HSV...
 *  H == Farbe HUE 0 - 65535 0 - 360 Grad
 *  S == Sättigung SAT 0- 255 0 - 100% 0 == ws >0 == color
 *  V == Helligkeit VAL 0 - 255 /  0 - 100%
*/

// LED Werte
// LED aus
uint16_t offH = 0;
uint8_t offS = 0;
uint8_t offV = 0;

/* Farbe der Uhr */
/* default >> WHITE */
uint16_t d_UhrH = 0;
uint8_t d_UhrS = 0;
uint8_t d_UhrV = 10;
/* current */
uint16_t c_UhrH;
uint8_t c_UhrS;
uint8_t c_UhrV;
/* temp */
uint16_t t_UhrH;
uint8_t t_UhrS;
uint8_t t_UhrV;

/* Farbe der Trenner */
/* default >> ROT */
uint16_t d_SepH = 360;
uint8_t d_SepS = 100;
uint8_t d_SepV = 10;
/* current */
uint16_t c_SepH;
uint8_t c_SepS;
uint8_t c_SepV;
/* temp */
uint16_t t_SepH;
uint8_t t_SepS;
uint8_t t_SepV;

/*
 * Seperator State
 * 0 == nie an
 * 1 == flash original
 * 2 == immer an 
 * d_SepState >> default 1
 * c_SepState >> current
*/
 
int d_SepState = 1;
int c_SepState;

// Zeiger für Schleife
int i; 

/* 46 LED Adressen [0,...,45] mit Zuordnung zu ihrer Funktion */
byte oneSecond[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };           //09
byte tenSecond[] = { 10, 11, 12, 13, 14 };                  //15,16
byte oneMinute[] = { 17, 18, 19, 20, 21, 22, 23, 24, 25 };  //26
byte tenMinute[] = { 27, 28, 29, 30, 31 };                  //32,33
byte oneHour[] = { 34, 35, 36, 37, 38, 39, 40, 41, 42 };    //43
byte tenHour[] = { 44, 45 };
byte Separator[] = { 9, 15, 16, 26, 32, 33, 43 };

Adafruit_NeoPixel pixels(NUMPIXELS, UHR_PIN, NEO_GRB + NEO_KHZ800);
#define LED(x, h, s, v) pixels.setPixelColor(x, pixels.ColorHSV(h, s, v))

void BeginPixels(){
    pinMode(UHR_PIN, OUTPUT); // set Pin als Output
}

void ShowPixels(){
    pixels.show();  // showtime - Init all pixels OFF
}

void MakePixels() {

  t_UhrH = map(c_UhrH, 0, 360, 0, 65535);
  t_UhrS = map(c_UhrS, 0, 100, 0, 255);
  t_UhrV = map(c_UhrV, 0, 100, 0, 255);
  
  t_SepH = map(c_SepH, 0, 360, 0, 65535);
  t_SepS = map(c_SepS, 0, 100, 0, 255);
  t_SepV = map(c_SepV, 0, 100, 0, 255);

  // Sekunden
  s1 = s % 10;
  s10 = s / 10;

  // Minuten
  m1 = m % 10;
  m10 = m / 10;

  // Stunden
  h1 = h % 10;
  h10 = h / 10;

  // Set Pixels AN/AUS
  for (i = 0; i < sizeof(oneSecond); i++)
    (s1 <= i ? LED(oneSecond[i], offH, offS, offV) : LED(oneSecond[i], t_UhrH, t_UhrS, t_UhrV));

  for (i = 0; i < sizeof(tenSecond); i++)
    (s10 <= i ? LED(tenSecond[i], offH, offS, offV) : LED(tenSecond[i], t_UhrH, t_UhrS, t_UhrV));

  for (i = 0; i < sizeof(oneMinute); i++)
    (m1 <= i ? LED(oneMinute[i], offH, offS, offV) : LED(oneMinute[i], t_UhrH, t_UhrS, t_UhrV));

  for (i = 0; i < sizeof(tenMinute); i++)
    (m10 <= i ? LED(tenMinute[i], offH, offS, offV) : LED(tenMinute[i], t_UhrH, t_UhrS, t_UhrV));

  for (i = 0; i < sizeof(oneHour); i++)
    (h1 <= i ? LED(oneHour[i], offH, offS, offV) : LED(oneHour[i], t_UhrH, t_UhrS, t_UhrV));

  for (i = 0; i < sizeof(tenHour); i++)
    (h10 <= i ? LED(tenHour[i], offH, offS, offV) : LED(tenHour[i], t_UhrH, t_UhrS, t_UhrV));

  // Separator
  if (((s1 == 9) && (s10 == 5) && c_SepState == 1) || ((m1 == 9) && (m10 == 5) && c_SepState == 1) || c_SepState == 2) {
    for (int i = 0; i < sizeof(Separator); i++) LED(Separator[i], t_SepH, t_SepS, t_SepV);
  } else {
    for (int i = 0; i < sizeof(Separator); i++) LED(Separator[i], offH, offS, offV);
  }

  delay(20);
  pixels.show();  // showtime
}
