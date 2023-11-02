/* 
 *  GlobalVars.h - Deklaration
 *  GlobalVars.cpp - Definition
 *  ***
 *  Dateien mit allen Variablen
 *  die global benötigt werden
 *  ***
 *  Michel 02.11.2023
 */

#include <Arduino.h>
#include "GlobalVars.h" // Deklaration

/* ### Firmware und Produktinfo ### */
/* default */
String d_Firmware = "x2.101";
String d_Build = "0001";
String d_ProductName = "Test Split Files";
/* custom */
String c_Firmware;
String c_Build;
String c_ProductName;
String c_Product;
String c_ProductMac;
String c_ChipModel;
uint8_t c_ChipRevision;

/* ### Diverse allgemeine Variablen */
int s;  // s== Sekunde
int m;  // m == Minute
int h;  // h == Stunde
int s1;  // 1'ner Sekunde
int m1;  // 1'ner Minute
int h1;  // 1'ner Stunde
int s10;  // 10'er Sekunde
int m10;  // 10'er Minute
int h10;  // 10'er Stunde
int so = 99;  // Sekunde old mit Vorbelegung für ersten loop
int mo = 99;  // Minute old mit Vorbelegung für ersten loop
int ho = 99;  // Stunde old mit Vorbelegung für ersten loop

/* ### Wifi ### */
/* default */
String ssid = "A";
String pass = "B";
String d_softAPName = "rheinturm";
String d_UserAgent = "MAF-RT";
IPAddress d_ip(192, 168, 0, 200);
IPAddress d_dns(8, 8, 8, 8);
IPAddress d_gateway(192, 168, 0, 1);
IPAddress d_mask(255, 255, 255, 0);
/* custom */
String c_softAPName;
String c_UserAgent;
IPAddress c_ip;
IPAddress c_dns;
IPAddress c_gateway;
IPAddress c_mask;

/* ### MDNS ### */
/* dafault */
String d_Hostname = "rheinturm";
/* custom */
String c_Hostname;

/* ### NTP ### */
/* default */
const char* ntpServer = "pool.ntp.org";
const long updateInterval = 3600000;  // 60 * 60 * 1000 == 1 Stunde
int32_t d_gmtOffset = 0;
/* custom */
int32_t c_gmtOffset = 0;
