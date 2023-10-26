/*
 * 
 * Die Uhr mit 46 Pixeln
 * 
 */

// Pin für den Data Anschluss
const int UHR_PIN = D10;

// Anzahl Led's
const int NUMPIXELS = 181;

// Alles was direkt für die Uhr benötigt wird
int i; // Zeiger
int s, m, h;  // s== Sekunde m == Minute h == Stunde
int so = 99;  // Sekunde old mit Vorbelegung für ersten loop
int mo = 99;  // Minute old mit Vorbelegung für ersten loop
int ho = 99;  // Stunde old mit Vorbelegung für ersten loop
int s10, s1;  // TENSecond ONESecond
int m10, m1;  // TENMinute ONEMinute
int h10, h1;  // TENHour ONEHour

// LED Farben definieren
// HSV...
// h == Farbe HUE 0 - 65535 0 - 360 Grad
// s == Sättigung SAT 0- 255 0 - 100% 0 == ws >0 == color
// v == Helligkeit VAL 0 - 255 /  0 - 100%

// LED Werte
// LED aus
uint16_t offH = 0;
uint8_t offS = 0;
uint8_t offV = 0;

// Farbe der Uhr
// default >> WHITE
uint16_t d_UhrH = 0;
uint8_t d_UhrS = 0;
uint8_t d_UhrV = 10;
// current
uint16_t c_UhrH;
uint8_t c_UhrS;
uint8_t c_UhrV;
// temp
uint16_t t_UhrH;
uint8_t t_UhrS;
uint8_t t_UhrV;

// Farbe der Trenner
// default >> ROT
uint16_t d_SepH = 360;
uint8_t d_SepS = 100;
uint8_t d_SepV = 10;
// current
uint16_t c_SepH;
uint8_t c_SepS;
uint8_t c_SepV;
// temp
uint16_t t_SepH;
uint8_t t_SepS;
uint8_t t_SepV;

int d_SepState = 1;  // 0 nie an 1 falsh 2 immer an
int c_SepState;

// 46 LED Adressen [0,...,46] mit Zuordnung zu ihrer Funktion
byte oneSecond[] = { 0, 4, 8, 12, 16, 20, 24, 28, 32 };           //09
byte tenSecond[] = { 40, 44, 48, 52, 56};                  //15,16
byte oneMinute[] = { 68, 72, 76, 80, 84, 88, 92, 96, 100 };  //26
byte tenMinute[] = { 108, 112, 116, 120, 124 };                  //32,33
byte oneHour[] = { 136, 140, 144, 148, 152, 156, 160, 164, 168 };    //43
byte tenHour[] = { 176, 180 };
byte Separator[] = { 36, 60, 64, 104, 128, 132, 172 };

Adafruit_NeoPixel pixels(NUMPIXELS, UHR_PIN, NEO_GRB + NEO_KHZ800);
#define LED(x, h, s, v) pixels.setPixelColor(x, pixels.ColorHSV(h, s, v))

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

// ENDE Pixel
