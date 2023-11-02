/* 
 *  GlobalVars.h - Deklaration
 *  GlobalVars.cpp - Definition
 *  ***
 *  Dateien mit allen Variablen
 *  die global benötigt werden
 *  ***
 *  Michel 02.11.2023
 */

#pragma once

/* ### Firmware und Produktinfo ### */
/* default */
extern String d_Firmware;
extern String d_Build;
extern String d_ProductName;
/* custom */
extern String c_Firmware;
extern String c_Build;
extern String c_ProductName;
extern String c_Product;
extern String c_ProductMac;
extern String c_ChipModel;
extern uint8_t c_ChipRevision;

/* ### Diverse allgemeine Variablen */
extern int s; // s== Sekunde
extern int m; // m == Minute
extern int h; // h == Stunde
extern int s1;  // 1'ner Sekunde
extern int m1;  // 1'ner Minute
extern int h1;  // 1'ner Stunde
extern int s10;  // 10'er Sekunde
extern int m10;  // 10'er Minute
extern int h10;  // 10'er Stunde
extern int so; // Sekunde old
extern int mo; // Minute old
extern int ho; // Stunde old

/* ### Wifi ### */
/* default */
extern String ssid;
extern String pass;
extern String d_softAPName;
extern IPAddress d_ip;
extern IPAddress d_dns;
extern IPAddress d_gateway;
extern IPAddress d_mask;
extern String d_UserAgent;
/* custom */
extern String c_softAPName;
extern IPAddress c_ip;
extern IPAddress c_dns;
extern IPAddress c_gateway;
extern IPAddress c_mask;
extern String c_UserAgent;

/* ### MDNS ### */
/* dafault */
extern String d_Hostname;
/* custom */
extern String c_Hostname;

/* ### NTP ### */
/* default */
extern const char* ntpServer;
extern const long updateInterval;
extern int32_t d_gmtOffset;
// custom
extern int32_t c_gmtOffset;
