/* ESP8266 Alle Funktionen...

- WiFi Portal
- NTPClient
- Externe IP ermitteln
- Zeitzone ermitteln
- MQTT Client

*/

#pragma region Includes

// Arduino.h - Main include file for the Arduino SDK
#include <Arduino.h>

//#include <SPI.h>

// time.h - low level time and date functions
#include <Time.h>
#include <TimeLib.h>

// ESP8266WiFi.h - esp8266 Wifi support.
#include <WiFi.h>
 
// Portal incl. WLAN Management
#include <AutoConnect.h>

// Webserver für WiFi MAnger
#include <WebServer.h>

// WiFiUdp.h - implement UDP via WiFi
//#include <WiFiUdp.h>

// NTPClinet.h - implement the Class NTPClient
#include <NTPClient.h>

// HttpClient.h - implement the Class HttpClient to simplify HTTP fetching on Arduino
#include <HttpClient.h>

// ArduinoJson.h - https://arduinojson.org
#include <ArduinoJson.h>

//
#include <FastLED.h>

// PubSubClient.h - A simple client for MQTT
//#include <PubSubClient.h>
#include <PubSubClient.h>

#pragma endregion Includes

#pragma region allgemeine Variablen

#define MQTT_ID

uint8_t baseMac[6];
char baseMacChr;

String Product;
String ProductId;
String ProductMac;
String ChipModel;
uint8_t ChipRevision;

// Check Stunde
int NewCheck; // Stunde an dem der Check IP and TimeZone durchgeführt wird
int LastCheck; // Stunde an dem der Check (IP und Zeitzone) durchgeführt wurde

String HttpUserAgent;
String MyUrl;

String ExternalIP;

int TzOffset;

char LEDSTATE[32] = "ON"; // ON or OFF from MQTT

//StaticJsonDocument<4096> MQTT_in;
//StaticJsonDocument<4096> MQTT_out;

DynamicJsonDocument MQTT_in(4096);
DynamicJsonDocument MQTT_out(4096);

char TMP_Json[4096];
size_t TMP_Json_size;

char MQTT_MSG_TYPE[32];
char MQTT_MSG_COUNT[32];
char UhrRGB[6];


#pragma endregion allgemeine Variablen

#pragma region AutoConnect Konfiguration

// Server Portal und Config
WebServer Server;
AutoConnect Portal(Server);

AutoConnectConfig acConfig;       // Enable autoReconnect supported on v0.9.4

AutoConnectAux aux1("/turm_config", "Configure Turm");

ACText(Text_1, "Das ist mein erster Turmtext");
ACText(Text_2, "Das ist der 2. Text");

#pragma endregion AutoConnect Konfiguration

#pragma region -- NTP Client

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 21600000);

#pragma endregion ## NTP Client

#pragma region -- externe IP ermitteln

// öffentliche IP des Anschlusses ermitteln
// http://checkip.dyndns.com 
//response <Current IP Address: 79.211.88.42>

void CheckIP(){
 
  HTTPClient http;
  MyUrl = "http://checkip.dyndns.com";

  http.setUserAgent(HttpUserAgent);
  http.begin(MyUrl);

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    //Serial.print("HTTP Response code (2): ");
    //Serial.println(httpResponseCode);
    String payload = http.getString();
    
    payload.toUpperCase();
    
    // Response: <html><head><title>Current IP Check</title></head><body>Current IP Address: nnn.nnn.nnn.nnn</body></html>
    // Upper     0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
    //                                                             n|||||||||||||||||||||||||***************n
    // Response: <HTML><HEAD><TITLE>CURRENT IP CHECK</TITLE></HEAD><BODY>CURRENT IP ADDRESS: nnn.nnn.nnn.nnn</BODY></HTML>

    int OpenBodyTag = payload.indexOf("<BODY>");
    int ClosingBodyTag = payload.indexOf("</BODY>");

    int StringBegin = OpenBodyTag+26; // index <BODY> + <BODY><CURRENT IP ADDRESS: > 
    int StringEnd = ClosingBodyTag; // index </BODY>

    if (ExternalIP != payload.substring(StringBegin,StringEnd)){

      Serial.println("Old External IP:" + ExternalIP);
      
      ExternalIP = payload.substring(StringBegin,StringEnd);

      Serial.println("New External IP:" + ExternalIP);
    }
    else{

      Serial.println("External IP no change!");
    }
     
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  
}

#pragma endregion ## externe IP ermitteln


#pragma region -- MQTT Client

// https://github.com/tuielectronics/ESP8266-mqtt-over-websocket/blob/master/ESP8266-mqtt-over-websocket.ino


/* Informationen an rheinturm.one senden und ggf. empfangen

MQTT Client übermittelt zyklisch Standortdaten an die Rheinturm Base 
MQTT Cleint empfängt Daten von der Rheinturm Base

Daten werden im JSON format ausgetauscht.

Maximale Paketgröße 4096 Byte
Davon sind 23Byte Haeder und 4073Byte Nutzdaten.

Aufbau der Telegramme...
{
  "msg-type": "GEOINFO",
  "data":{
    "Continent": "Europe",
    "Stadt": "Düsseldorf",
    "lat": "51.2402",
    "long": "6.7785",
  }
}

{
  "msg-type": "DEVICEINFO",
  "data":{
  
    "Timestamp": "",
    "Product-ID": "sgsg-874594-09099",
    "IPv4": "123.123.123.123",
  }
}

{
  "msg-type": "CHANGEVALUE",
  "data":{
    "LEDSATATE": "on", ON|OFF
    "UhrR": "23", rot int
    "UhrG": "23", rot int
    "UhrB": "23", rot int
    "SeparartorR": "23", rot int
    "SeparartorG": "23", rot int
    "SeparartorB": "23", rot int
    ....  alle Variablen sind möglich!!!!
  }
}


*/

// server > mqtt-broker.rheinturm.one
// port > 8883
// user > rheinturm
// turmPWD@mqtt

// client ident > ist die ProductId ( HEX MAC oder 64Bit integer)


  const char *MQTT_HOST = "mqtt.rheinturm.cloud";
  const int MQTT_PORT = 443;
  const char *MQTT_CLIENT_ID;
  const char *MQTT_USER = "turm"; // leave blank if no credentials used
  const char *MQTT_PASS = "turmPWD@mqtt"; // leave blank if no credentials used

  String SubTopicString;
  const char *MQTT_SUB_TOPIC;
  String PubTopicString;
  const char *MQTT_PUB_TOPIC;

 const char* local_root_ca = \
"-----BEGIN CERTIFICATE-----\n"\
"MIIGljCCBX6gAwIBAgIQDE4agKIvfUJrZwJrCM5ZMDANBgkqhkiG9w0BAQsFADBZ\n"\
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMTMwMQYDVQQDEypS\n"\
"YXBpZFNTTCBUTFMgRFYgUlNBIE1peGVkIFNIQTI1NiAyMDIwIENBLTEwHhcNMjEx\n"\
"MjAyMDAwMDAwWhcNMjIxMjAxMjM1OTU5WjAcMRowGAYDVQQDDBEqLnJoZWludHVy\n"\
"bS5jbG91ZDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKDaiRg59mnL\n"\
"+L0USF3d4xLxHDW45O6H8YfJNzwSjUEhmTxw0CYbyIRXg/HnQ0J26bcGNu57jXrm\n"\
"J5PgIeRrJ0GcbyYkQtByV5X0DlkLe2cwUIJEymHUpzSB3wNUrXSRVQmyk9AlWNwK\n"\
"uUnxgv4qvv8RVKtq1uK03IWmnH14TkSBBgfpCB8P9whGxE/xSyUyOnH/o5oioIJm\n"\
"8jjdCFN/TQp14Z3YWbIE17zl1vxB9DLR4Mk1KAjYWZ6RPnTkqNkhA8aOoWoopFWA\n"\
"pTXM4UFMhahiW7im5Ylm8Apdv4WgG9WDQH22m/Uob44rWQw+FYYznWDqqU1KPr68\n"\
"jOGpy6xct0ECAwEAAaOCA5UwggORMB8GA1UdIwQYMBaAFKSN5b58eeRwI20uKTSt\n"\
"I1jc9TF/MB0GA1UdDgQWBBSbRMXr/iyUn4ZXwyKxjuXnD0rl9DAtBgNVHREEJjAk\n"\
"ghEqLnJoZWludHVybS5jbG91ZIIPcmhlaW50dXJtLmNsb3VkMA4GA1UdDwEB/wQE\n"\
"AwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwgZsGA1UdHwSBkzCB\n"\
"kDBGoESgQoZAaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL1JhcGlkU1NMVExTRFZS\n"\
"U0FNaXhlZFNIQTI1NjIwMjBDQS0xLmNybDBGoESgQoZAaHR0cDovL2NybDQuZGln\n"\
"aWNlcnQuY29tL1JhcGlkU1NMVExTRFZSU0FNaXhlZFNIQTI1NjIwMjBDQS0xLmNy\n"\
"bDA+BgNVHSAENzA1MDMGBmeBDAECATApMCcGCCsGAQUFBwIBFhtodHRwOi8vd3d3\n"\
"LmRpZ2ljZXJ0LmNvbS9DUFMwgYUGCCsGAQUFBwEBBHkwdzAkBggrBgEFBQcwAYYY\n"\
"aHR0cDovL29jc3AuZGlnaWNlcnQuY29tME8GCCsGAQUFBzAChkNodHRwOi8vY2Fj\n"\
"ZXJ0cy5kaWdpY2VydC5jb20vUmFwaWRTU0xUTFNEVlJTQU1peGVkU0hBMjU2MjAy\n"\
"MENBLTEuY3J0MAkGA1UdEwQCMAAwggF+BgorBgEEAdZ5AgQCBIIBbgSCAWoBaAB1\n"\
"AEalVet1+pEgMLWiiWn0830RLEF0vv1JuIWr8vxw/m1HAAABfXrpCRMAAAQDAEYw\n"\
"RAIgIwvvaY2hvAyMuTB+01u7I/4lnCE6gwcwIySSNhhfVZICICOASo/x3/pEN7Ws\n"\
"Q98+Di2GIvwupJFQuS0DlqHwH5LGAHcAQcjKsd8iRkoQxqE6CUKHXk4xixsD6+tL\n"\
"x2jwkGKWBvYAAAF9eukJHgAABAMASDBGAiEAtZkmYDyeikUEbKwKqOFG3raPpQ3p\n"\
"1wRLU6CZZZD2Pn4CIQDRpVxayWU3U/qXaLz1XIv4BlzzLIULG10Bn531wxl8kAB2\n"\
"AN+lXqtogk8fbK3uuF9OPlrqzaISpGpejjsSwCBEXCpzAAABfXrpCSMAAAQDAEcw\n"\
"RQIhAL61stKctKgBVj6JCYAQlLkN5FCjFxKIJFqoYQTi5EPYAiBIse/Y5OK0OAUg\n"\
"5oKiy+44UZk08JM0Vtu88Szvj4jRKTANBgkqhkiG9w0BAQsFAAOCAQEAzlbTandB\n"\
"IZXvyyc1VFAnBkIb5b0daPj+frzrVc3Ndc5grP5kHa8+zPInChBc9Zz2//IzjIwy\n"\
"tw3n3mbySFlZd0K/NiyBjybIeO2z+dS4MSmCnVLOH1B6m2nzQAW9JLuq72Owdz+J\n"\
"R3xl7XzTsMxpqp9iIAovv8h9+73BrNKa3/R1QhjTK1bTWVgL3dhXX6gGdQ1VCSmt\n"\
"beUPaz+JI3sZOuXnGWCy7X8G2xhhDB7x5dr/PMTl9uCGsfjUGWkgaDW9arhU3rAx\n"\
"YSzAUfDvS13pzHPyxFHOqBmbRTFRhyXalObb/RqNfjrOmbiovU4/2j/gQldbTtH4\n"\
"mMYo1lP8QnHd6g==\n"\
"-----END CERTIFICATE-----";


WiFiClientSecure SecureClient;
PubSubClient MQTTClient(SecureClient);

void mqtt_connect()
{
    while (!MQTTClient.connected()) {
    Serial.println("MQTT connecting");
    if (MQTTClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {

      Serial.println("MQTT connected");

      MQTTClient.subscribe(MQTT_SUB_TOPIC);
      Serial.print("Subscribe: ");
      Serial.println(MQTT_SUB_TOPIC);

    } else {
      Serial.print("failed, status code =");
      Serial.println(MQTTClient.state());
    }
  }  
}

void receivedCallback(const char* topic, byte* payload, unsigned int length) {
 
  auto error = deserializeJson(MQTT_in, (const byte*)payload, length);

  if (error) {
    Serial.print(F("MQTT Reponse deserializeJson failed with code: "));
    Serial.println(error.c_str());
    return;
  }
  else{
    Serial.print("Received Topic: ");
    Serial.println(topic);
    Serial.print("Received Length: ");
    Serial.println(length);
  
    Serial.println("DATA BEGIN: ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println("...");
    Serial.println("DATA END");

    strlcpy(MQTT_MSG_TYPE, MQTT_in["MSG_TYPE"] | MQTT_MSG_TYPE, sizeof(MQTT_MSG_TYPE));
    strlcpy(MQTT_MSG_COUNT, MQTT_in["MSG_COUNT"] | MQTT_MSG_COUNT, sizeof(MQTT_MSG_COUNT));

    // werte aus dem Empfangsdaten übernehemen
    // wenn nicht vorhanden bleibt der alte Wert bestehen

    strlcpy(LEDSTATE, MQTT_in["data"]["LEDSTATE"] | LEDSTATE, sizeof(LEDSTATE));
    strlcpy(UhrRGB, MQTT_in["data"]["UHR-RGB-HEX"] | UhrRGB, sizeof(UhrRGB));

    Serial.print("LEDSTATE: ");
    Serial.println(LEDSTATE);

  }
}

#pragma endregion ## MQTT Client

#pragma region -- Zeitzone der externen IP ermitteln

StaticJsonDocument<4096> doc;

void GetTimeZoneByIP(){
  // http://ip-api.com/json/79.211.94.221

  // http://ip-api.com/csv/79.211.94.221?fields=status,message,continent,continentCode,country,countryCode,region,regionName,city,district,zip,lat,lon,timezone,offset,currency,isp,org,as,asname,reverse,mobile,proxy,hosting,query
  // http://ip-api.com/json/<IPADRESS>?fields=status,message,continent,continentCode,country,countryCode,region,regionName,city,district,zip,lat,lon,timezone,offset,currency,isp,org,as,asname,query
  
  HTTPClient http;
  http.setUserAgent(HttpUserAgent);
  MyUrl = "http://ip-api.com/json/" + ExternalIP + "?fields=status,message,continent,continentCode,country,countryCode,region,regionName,city,district,zip,lat,lon,timezone,offset,currency,isp,org,as,asname,reverse,mobile,proxy,hosting,query";
  
  http.begin(MyUrl);
  int httpCode = http.GET();
  if (httpCode == 200) { //200 = OK

    String payload = http.getString();
     
    DeserializationError error = deserializeJson(doc, payload);

    http.end(); //Die Verbindung beenden

    if (error) {
      Serial.print(F("deserializeJson() failed(current time): "));
      Serial.println(error.c_str());

      return;
    }

    int offset = doc["offset"];

    TzOffset = offset;

    MQTT_out["geo-info"]["offset"] =  doc["offset"] ;
    MQTT_out["geo-info"]["city"] = doc["city"] ;

  } else {
    Serial.println("Error on HTTP request");
  }

}

#pragma endregion ## Zeitzone der externen IP ermitteln

#pragma region -- Check Location, TimeZone and publish Data over MQTT



void CheckLocation(){

  if(NewCheck != LastCheck){

    Serial.print("TimeCheck Uhrzeit alt: "); 
    Serial.println(timeClient.getFormattedTime());
    
    // external IP ermitteln
    CheckIP();

    // Offstet der Zeitzone ermitteln
    GetTimeZoneByIP();

    // Offset der Zeitzone setzen
    timeClient.setTimeOffset(TzOffset);

    // UTC Zeit holen
    timeClient.update();

    LastCheck = timeClient.getHours();

    Serial.print("TimeCheck Uhrzeit neu: "); 
    Serial.println(timeClient.getFormattedTime());

    Serial.print("Publish Check Timezone: ");
    Serial.println(MQTT_PUB_TOPIC);

    // MQTT Daten senden
    // ArduinoJson Dokumentes in ein temporäres char Element 
    // doc >> TMP_Json mit ermittlung der Byte Anzahl

    TMP_Json_size = serializeJson(MQTT_out, TMP_Json);
    // senden an Publish Topic
    //MQTTClient.publish(MQTT_PUB_TOPIC, TMP_Json, TMP_Json_size);

  }
}

#pragma endregion ## Check Location, TimeZone and publish Data over MQTT

#pragma region Pixels

// How many leds in your strip?
#define NUM_LEDS 54

// GPIO Data
#define DATA_PIN 16

// Define the array of leds
CRGB leds[NUM_LEDS];

int i,s,so,m,mo,h;
int s10,s1; //TENSecond ONESecond 
int m10,m1; //TENMinute ONESecond
int h10,h1; //Hour ..

#pragma region LED Farben definieren

int UhrR = 0;
int UhrG = 0;
int UhrB = 120;

int SeparatorR = 20;
int SeparatorG = 50;
int SeparatorB = 80;

int RestaurantR = 0; 
int RestaurantG = 0;
int RestaurantB = 120;

int FlugbefeuerungR = 120;
int FlugbefeuerungG = 0;
int FlugbefeuerungB = 0;

#pragma endregion LED Farben definieren

#pragma region LED Adressen
//54 LED Adressen [0,...,53] mit Zuordnung zu ihrer Funktion

byte oneSecond[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};     //09
byte tenSecond[5] = {10, 11, 12, 13, 14};            //15,16
byte oneMinute[9] = {17, 18, 19, 20, 21, 22, 23, 24, 25}; //26
byte tenMinute[5] = {27, 28, 29, 30, 31};            //32,33
byte oneHour[9] = {34, 35, 36, 37, 38, 39, 40, 41, 42}; //43
byte tenHour[2] = {44, 45};
byte Separator[7] = {9, 15, 16, 26, 32, 33, 43};
byte Restaurant[4] = {46, 47, 48, 49};
byte Flugbefeuerung[4] = {50, 51, 52, 53};

#pragma endregion LED Adressen

void MakePixels() {

// strcmp(corpo, "123") == 0
if(( LEDSTATE[0] == 'O' ) && ( LEDSTATE[1] == 'N' )){
  //if(strcmp(LEDSTATE, "ON") == 1){
      s1 = s % 10; // 1er Sekunden
      s10 = s / 10; // 10er Sekunden

      m1 = m % 10; // 1er Minuten
      m10 = m / 10; // 10er Minuten

      h1 = h % 10; // 1er Stunden
      h10 = h / 10; // 10er Stunden

       for(int i=0;i<sizeof(oneSecond);i++)
       (s1 <=i?leds[oneSecond[i]] = CRGB(0,0,0):leds[oneSecond[i]] = CRGB(UhrR,UhrG,UhrB));
       //(s1 <=i?leds[oneSecond[i]] = CRGB(0,0,0):leds[oneSecond[i]] = CRGB::Silver);

       for(int i=0;i<sizeof(tenSecond);i++)
       (s10 <=i?leds[tenSecond[i]] = CRGB(0,0,0):leds[tenSecond[i]] = CRGB(UhrR,UhrG,UhrB));
       //(s10 <=i?leds[tenSecond[i]] = CRGB(0,0,0):leds[tenSecond[i]] = CRGB::Silver);

       for(int i=0;i<sizeof(oneMinute);i++)
       (m1 <=i?leds[oneMinute[i]] = CRGB(0,0,0):leds[oneMinute[i]] = CRGB(UhrR,UhrG,UhrB));

       for(int i=0;i<sizeof(tenMinute);i++)
       (m10 <=i?leds[tenMinute[i]] = CRGB(0,0,0):leds[tenMinute[i]] = CRGB(UhrR,UhrG,UhrB));

       for(int i=0;i<sizeof(oneHour);i++)
       (h1 <=i?leds[oneHour[i]] = CRGB(0,0,0):leds[oneHour[i]] = CRGB(UhrR,UhrG,UhrB));

       for(int i=0;i<sizeof(tenHour);i++)
       (h10 <=i?leds[tenHour[i]] = CRGB(0,0,0):leds[tenHour[i]] = CRGB(UhrR,UhrG,UhrB));

       // Separator
 if(((s1==9) && (s10==5)) || ((m1==9) && (m10==5))){
  for(int i=0;i<sizeof(Separator);i++)leds[Separator[i]] = CRGB(SeparatorR,SeparatorG,SeparatorB);
 }
 else{
  for(int i=0;i<sizeof(Separator);i++)leds[Separator[i]] = CRGB(0,0,0);
  }

 // Restaurant
 if((h>=18) || (h<=17)){
  for(int i=0;i<sizeof(Restaurant);i++)leds[Restaurant[i]] = CRGB(RestaurantR,RestaurantG,RestaurantB);
 }
 else{
  for(int i=0;i<sizeof(Restaurant);i++)leds[Restaurant[i]] = CRGB(0,0,0);
  }

// Flugbefeuerung 0-59 modulo 6 [0,1,2]
// LED#s haben 2 Helligkeitsstufen dadurch kommt es zu einer Überbelndung von 1 > 2 > 3 > 1...
 if((s % 6) == 0){
   leds[Flugbefeuerung[0]] = CRGB(FlugbefeuerungR,FlugbefeuerungG,FlugbefeuerungB);
   leds[Flugbefeuerung[1]] = CRGB(0,0,0);
   leds[Flugbefeuerung[2]] = CRGB(0,0,0);
 
 }

 if((s % 6) == 1){
   leds[Flugbefeuerung[0]] = CRGB(0,0,0);
   leds[Flugbefeuerung[1]] = CRGB(FlugbefeuerungR,FlugbefeuerungG,FlugbefeuerungB);
   leds[Flugbefeuerung[2]] = CRGB(0,0,0);
 }

 if((s % 6) == 2){
   leds[Flugbefeuerung[0]] = CRGB(0,0,0);
   leds[Flugbefeuerung[1]] = CRGB(0,0,0);
   leds[Flugbefeuerung[2]] = CRGB(FlugbefeuerungR,FlugbefeuerungG,FlugbefeuerungB);
 }

  if((s % 6) == 3){
   leds[Flugbefeuerung[0]] = CRGB(FlugbefeuerungR,FlugbefeuerungG,FlugbefeuerungB);
   leds[Flugbefeuerung[1]] = CRGB(0,0,0);
   leds[Flugbefeuerung[2]] = CRGB(0,0,0);
 }

 if((s % 6) == 4){
   leds[Flugbefeuerung[0]] = CRGB(0,0,0);
   leds[Flugbefeuerung[1]] = CRGB(FlugbefeuerungR,FlugbefeuerungG,FlugbefeuerungB);
   leds[Flugbefeuerung[2]] = CRGB(0,0,0);
 }

 if((s % 6) == 5){
   leds[Flugbefeuerung[0]] = CRGB(0,0,0);
   leds[Flugbefeuerung[1]] = CRGB(0,0,0);
   leds[Flugbefeuerung[2]] = CRGB(FlugbefeuerungR,FlugbefeuerungG,FlugbefeuerungB);
 }

 leds[Flugbefeuerung[3]] = CRGB(FlugbefeuerungR,FlugbefeuerungG,FlugbefeuerungB);

 FastLED.show();
}

else{
  FastLED.clear();
  FastLED.show();
}
}
 


#pragma endregion Pixels

#pragma region -- Turm Konfigurations Seite

/* Root Seite des Webservers
*
* Startseite enthällt die Komponenten zur Kofiguration des Turmes
*
*/

void rootPage() {
  String content = "Rheinturm...";
  content += AUTOCONNECT_LINK(COG_24);

  Server.send(200, "text/html", content);
}

#pragma endregion Startseite

#pragma region -- Hilfsfunktionen

String getMacAsString() {

    uint8_t baseMac[6];
    // Get MAC address for WiFi station
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    char baseMacChr[18] = {0};
    sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);

    return String(baseMacChr);
}

void setup() {

  // serial Monitor set Baud
  Serial.begin(9600);
  Serial.println("");

  pinMode(DATA_PIN, OUTPUT);

  delay(10);

  // Produkt Daten ermittelm  
  ChipModel = ESP.getChipModel();
  ChipRevision = ESP.getChipRevision();
  ProductMac = getMacAsString();

  Product = ChipModel + "-" + ChipRevision + "-" + ProductMac;
  HttpUserAgent = "MAF-RT/" + ProductMac;

  MQTT_CLIENT_ID = ProductMac.c_str();

  PubTopicString = "PROD/IN/" + ProductMac + "/out";
  MQTT_PUB_TOPIC = PubTopicString.c_str();
 
  SubTopicString = "PROD/OUT/" + ProductMac + "/in";
  MQTT_SUB_TOPIC = SubTopicString.c_str();

  Serial.print("Product ID: "); Serial.println(ProductId);
  Serial.print("Product: "); Serial.println(Product);
  Serial.print("UserAgent: "); Serial.println(HttpUserAgent);

  sprintf(UhrRGB, "%02X%02X%02X", UhrR, UhrG, UhrB);

  JsonObject root = MQTT_out.to<JsonObject>();

  //JsonObject GeoInfo = root.createNestedObject("geo");

  JsonObject DeviceInfo = root.createNestedObject("device");
  DeviceInfo["id"] = ProductMac;
  DeviceInfo["model"] = ChipModel;
  DeviceInfo["revision"] = ChipRevision;
  DeviceInfo["firmware"] = "A.0.0.1";

  //JsonObject LoopInfo = root.createNestedObject("loop");
  //LoopInfo["MSG_COUNT"] = MQTT_MSG_COUNT;
  //LoopInfo["Useragent"] = HttpUserAgent;

  delay(10);

  FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(100);

  NewCheck = 0; // Stunde an dem der Check IP and TimeZone durchgeführt wird
  LastCheck = 99; // Stunde an dem der Check (IP und Zeitzone) durchgeführt wurde

  // Setup Portal
  acConfig.hostName = "rheinturm";
  acConfig.apid = "Rheinturm"; // AP Name
  acConfig.psk = ""; // AP ohne Password
  acConfig.title ="Rheinturm"; // Name links im _ac Head
  acConfig.ota = AC_OTA_BUILTIN;
  acConfig.menuItems = AC_MENUITEM_HOME | AC_MENUITEM_CONFIGNEW;
  Portal.config(acConfig);

  aux1.add({Text_1, Text_2});

  Portal.join({aux1});

  // Root Page
  Server.on("/", rootPage);

  // Establish a connection with an autoReconnect option.
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }

  timeClient.begin();

  //SecureClient.setCACert(local_root_ca);
  //MQTTClient.setServer(MQTT_HOST, MQTT_PORT);
  //MQTTClient.setBufferSize(4096);
  //MQTTClient.setCallback(receivedCallback);

  //mqtt_connect();

  Serial.println("Setup ends now.");

}

void loop() {

  // Portal Clientaufrufe bearbeiten
  Portal.handleClient();
  Portal.handleRequest();   // Need to handle AutoConnect menu.

 // if (!MQTTClient.connected()){
 // mqtt_connect();
 // }
 // else{
 //  MQTTClient.loop();
 // }

  // Uhrzeit des Standortes ermitteln
  CheckLocation();

  // aktuelle Stunde für die Überprüfung der Uhrzeit setzen
  NewCheck = timeClient.getHours();

  s=timeClient.getSeconds();
  m=timeClient.getMinutes();
  h=timeClient.getHours();


  if(so!=s){    // so Sekunde old ungleich aktuelle Sekunde
    so=s;       // sekunde in Sekunde old merken
    MakePixels();
  }

  // Ausgabe doc minütlich als test
  //if(mo!=m){    // mo Minute old ungleich aktuelle Minute
   // mo=m;       // Minute in old merken

    // zyklische Ausgabe an MQTT Broker
 
    //TMP_Json_size = serializeJson(MQTT_out, TMP_Json);
    //MQTTClient.publish(MQTT_PUB_TOPIC, TMP_Json, TMP_Json_size);

  //}

  delay(5);

}