/*** Projekt
 ###
 
 Projekt für Rheinturm-André
 
 Firmware: x2.004
 Build: main
   
 Sandwich Modell mit 46 Pixeln
 AdaFruit Stripe 20mm Spezial

 ###
***/

 
/*** Includes
 ###
 
 Includes und Variablen
 
 ###
***/

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <NTPClient.h>
#include <Preferences.h>
#include "time.h"
#include <TimeLib.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

/* Nicht flüchtige Variablen als prefs */
Preferences prefs;

/* Firmware und Produktinfo */
/* default */
String d_Firmware = "x2.004";
String d_Build = "r1";
String d_ProductName = "Rheinturm Andrè";
/* custom */
String c_Firmware;
String c_Build;
String c_ProductName;
String c_Product;
String c_ProductMac;
String c_ChipModel;
uint8_t c_ChipRevision;

bool _FirstLoop;
uint8_t c_LoopM;

// ### Wifi ###
// default
String ssid = "A";
String pass = "B";
String d_softAPName = "rheinturm";
IPAddress d_ip(192, 168, 0, 200);
IPAddress d_dns(8, 8, 8, 8);
IPAddress d_gateway(192, 168, 0, 1);
IPAddress d_mask(255, 255, 255, 0);
String d_UserAgent = "MAF-RT";
// custom
String c_softAPName;
IPAddress c_ip;
IPAddress c_dns;
IPAddress c_gateway;
IPAddress c_mask;
String c_UserAgent;

// Create HTTPClient
WiFiClientSecure wifiClient;
HTTPClient httpClient;
    
// ### MDNS ###
// dafault
String d_Hostname = "rheinturm-andre";
// custom
String c_Hostname = "";

// ### Webserver ###
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// ### NTP ###
// default
const char* ntpServer = "pool.ntp.org";
const long updateInterval = 3600000;  // 60 * 60 * 1000 == 1 Stunde
int32_t d_gmtOffset = 0;
// custom
int32_t c_gmtOffset = 0;
// Create NTP-Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, c_gmtOffset, updateInterval);

// ### JSON ###
char JSONBuffer[2048];
size_t JSON_Buffer_size;
DynamicJsonDocument myInfo(2048);
DynamicJsonDocument toKafka(2048);

/*
 * 
 * Die Uhr mit 46 Pixeln
 * 
 */

/* Pin für den Data Anschluss */
const int UHR_PIN = D10;
/* Anzahl Led's */
const int NUMPIXELS = 46;

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

/* 46 LED Adressen [0,...,46] mit Zuordnung zu ihrer Funktion */
byte oneSecond[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };           //09
byte tenSecond[] = { 10, 11, 12, 13, 14 };                  //15,16
byte oneMinute[] = { 17, 18, 19, 20, 21, 22, 23, 24, 25 };  //26
byte tenMinute[] = { 27, 28, 29, 30, 31 };                  //32,33
byte oneHour[] = { 34, 35, 36, 37, 38, 39, 40, 41, 42 };    //43
byte tenHour[] = { 44, 45 };
byte Separator[] = { 9, 15, 16, 26, 32, 33, 43 };

// NEO Farbkanäle RGB
Adafruit_NeoPixel pixels(NUMPIXELS, UHR_PIN, NEO_RGB + NEO_KHZ800);
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

// Timer variables
unsigned long previousMillis = 0;
const long interval = 5000;  // interval to wait for Wi-Fi connection (milliseconds)

bool initWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi ..");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  c_ip = WiFi.softAPIP();
  Serial.print("Station IP address: ");
  Serial.println(c_ip); 
  
  return true;
}

// HTML Prozessor
// Platzhalter durch aktuelle Werte ersetzen
String processor(const String& var) {
  if(var == "STATE_UH") {
    return String(c_UhrH);
  }
  if(var == "STATE_US") {
    return String(c_UhrS);
  }
  if(var == "STATE_UV") {
    return String(c_UhrV);
  }
  if(var == "STATE_TH") {
    return String(c_SepH);
  }
  if(var == "STATE_TS") {
    return String(c_SepS);
  }
  if(var == "STATE_TV") {
    return String(c_SepV);
  }
  if((var == "STATE_SEP_0") && (c_SepState == 0)) {
    return "selected";
  }
  if((var == "STATE_SEP_1") && (c_SepState == 1)) {
    return "selected";
  }
  if((var == "STATE_SEP_2") && (c_SepState == 2)) {
    return "selected";
  }
  if(var == "STATE_DEFAULT_UH") {
    return String(d_UhrH);
  }
  if(var == "STATE_DEFAULT_US") {
    return String(d_UhrS);
  }
  if(var == "STATE_DEFAULT_UV") {
    return String(d_UhrV);
  }
  if(var == "STATE_DEFAULT_TH") {
    return String(d_SepH);
  }
  if(var == "STATE_DEFAULT_TS") {
    return String(d_SepS);
  }
  if(var == "STATE_DEFAULT_TV") {
    return String(d_SepV);
  }
  if(var == "STATE_HOSTNAME") {
    return c_Hostname;
  }
  if(var == "STATE_IP") {
    return c_ip.toString();
  }
  if(var == "STATE_DNS") {
    return c_dns.toString();
  }  
  if(var == "STATE_GATEWAY") {
    return c_gateway.toString();
  }
  if(var == "STATE_MASK") {
    return c_mask.toString();
  } 
  return String();
}

// Initialize SPIFFS
void initSPIFFS() {
  
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
}

// MAC als String ausgeben
String getMacAsString() {
  uint8_t baseMac[6];
  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  char baseMacChr[18] = {0};
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);

  return String(baseMacChr);
}

// das letze Byte der MAC Modulo 60 Rest als LoopTimer Minuten
uint8_t getLoopM() {
  uint8_t baseMac[6];
  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  return uint8_t(baseMac[5] % 60);
}

// ### Get my Location ###
void getMyInfo(){

  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status

    // https://ipapi.co/json

    wifiClient.setInsecure();  // HTTPS ohne SSL Überprüfung              
    String myUrl = "https://ipapi.co/json";

    httpClient.begin(wifiClient, myUrl); //Specify the URL
    int httpCode = httpClient.GET(); //Make the request

    if (httpCode > 0) { //Check for the returning code

      String payload = httpClient.getString();
      Serial.println(httpCode);
      Serial.println(payload);

      myInfo.clear();

      auto error = deserializeJson(myInfo, (const byte*)payload.c_str(), strlen(payload.c_str()));

      if (error) {
        Serial.print(F("IPAPI Reponse deserializeJson failed with code: "));
        Serial.println(error.c_str());

        return;
      }
      else {
            
        //{
        //"ip": "2003:ee:3f04:5000:6163:e32d:2a4:7aa",
        //"network": "2003:ee:3c00::/38",
        //"version": "IPv6", or "IPv4"
        //"city": "Düsseldorf",
        //"region": "North Rhine-Westphalia",
        //"region_code": "NW",
        //"country": "DE",
        //"country_name": "Germany",
        //"country_code": "DE",
        //"country_code_iso3": "DEU",
        //"country_capital": "Berlin",
        //"country_tld": ".de",
        //"continent_code": "EU",
        //"in_eu": true,
        //"postal": "40476",
        //"latitude": 51.2459,
        //"longitude": 6.7989,
        //"timezone": "Europe/Berlin",
        //"utc_offset": "+0200",
        //"country_calling_code": "+49",
        //"currency": "EUR",
        //"currency_name": "Euro",
        //"languages": "de",
        //"country_area": 357021.0,
        //"country_population": 82927922,
        //"asn": "AS3320",
        //"org": "Deutsche Telekom AG"
        //}

        // open myPref Preferences in RW
        prefs.begin("myPref", false);
  
        if(!myInfo["utc_offset"].isNull()){

          String tmp = myInfo["utc_offset"];
          
          // "utc_offset": "+0200" >> +0200 == 5 CHAR >> Format is HHMM
          if(tmp.length() == 5){

            // (02 * 3600) + (00 * 60) = 7200 >> +0200
            int offset = (tmp.substring(1,3).toInt()*3600) + (tmp.substring(3,5).toInt()*60);

            // + oder - vor den Betrag
            String offsetString = tmp.substring(0,1) + offset;

            if(offsetString.toInt() != c_gmtOffset){
              
              // Offset übernehmen
              c_gmtOffset = offsetString.toInt();
              prefs.putInt("GmtOffset", c_gmtOffset);

              // NTP Client mit neuem Offset versorgen
              timeClient.setTimeOffset(prefs.getInt("GmtOffset"));
              
              // NTP Update anfordern
              timeClient.forceUpdate();

              Serial.println("utc_offset neu!!");
            }
            else{
              Serial.println("utc_offset unverändert");
            }
          }
          else{
            Serial.println("utc_offset no valid");
          }
        }
        else{
          Serial.println("utc_offset not in list");
        }
      }
    }
    
    else {
      Serial.println("Error on HTTP request");
    }

    //httpClient.end(); //Free the resources
    httpClient.end(); //Free the resources
  }

}

// Send to Kafka
void putToKafka(){

  // https://bridge.kafka.rheinturm.cloud

  //String myUrl = "https://bridge.kafka.rheinturm.cloud/topics/" + c_ProductMac;
  String myUrl = "https://bridge.kafka.rheinturm.cloud/topics/FDX";
  // JSON leeren
  toKafka.clear();
    
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status

    wifiClient.setInsecure();  // HTTPS ohne SSL Überprüfung  
     
    httpClient.begin(wifiClient, myUrl);  
    httpClient.addHeader("Content-Type", "application/vnd.kafka.json.v2+json");  
    httpClient.setAuthorization("turm@kafkabridge", "vC9dg635Tzui87hstz3pJ2Kmcu78sh265sgkeOS2Bns74");
    
    JsonObject fdx = toKafka.to<JsonObject>();
    
    JsonArray records = fdx.createNestedArray("records");  
    JsonObject nestRecords = records.createNestedObject();
      nestRecords["key"] = c_ProductMac;
      JsonObject nestValue = nestRecords.createNestedObject("value");
        JsonObject nestProduct = nestValue.createNestedObject("product"); 
        JsonObject nestDevice = nestValue.createNestedObject("device");
          JsonObject nestDeviceHw = nestDevice.createNestedObject("hw");
          JsonObject nestDeviceSw = nestDevice.createNestedObject("sw");
          
        JsonObject nestLocation = nestValue.createNestedObject("location");
          JsonObject nestCustomLocation = nestLocation.createNestedObject("custom");
          JsonObject nestAutoLocation = nestLocation.createNestedObject("ipapi");

    nestDeviceHw["id"] = c_ProductMac;
    nestDeviceHw["model"] = c_ChipModel;
    nestDeviceHw["revision"] = c_ChipRevision;
    
    nestDeviceSw["firmware"] = d_Firmware;
    nestDeviceSw["build"] = d_Build;
    nestDeviceSw["starts"] = prefs.getULong64("SystemStarts");
    nestDeviceSw["fs_totalbytes"] = SPIFFS.totalBytes();
    nestDeviceSw["fs_usedbytes"] = SPIFFS.usedBytes();
    nestDeviceSw["useragent"] = c_UserAgent;
    nestDeviceSw["ip"] = WiFi.localIP().toString();
    nestDeviceSw["hostname"] = c_Hostname;
    nestDeviceSw["loop_m"] = c_LoopM;
     
    nestProduct["name"] = c_ProductName;
    nestProduct["seriennummer"] = "000000"; 
     
    nestAutoLocation["city"]= myInfo["city"];
    nestAutoLocation["region"] = myInfo["region"];
    nestAutoLocation["country"] = myInfo["country"];
    nestAutoLocation["country_name"] = myInfo["country_name"];
    nestAutoLocation["latitude"] = myInfo["latitude"];
    nestAutoLocation["longitude"] = myInfo["longitude"];
    nestAutoLocation["timezone"] = myInfo["timezone"];
    nestAutoLocation["utc_offset"] = myInfo["utc_offset"];

    nestCustomLocation["city"] = "";
    nestCustomLocation["country"] = "";
    nestCustomLocation["country_name"] = "";
    nestCustomLocation["latitude"] = "";
    nestCustomLocation["longitude"] = "";
    nestCustomLocation["timezone"] = "";
    nestCustomLocation["utc_offset"] = "";  
    
    String requestBody;
    serializeJson(fdx, requestBody);
    Serial.println(requestBody);
          
    int httpResponseCode = httpClient.POST(requestBody);
 
    if(httpResponseCode>0){
       
      String response = httpClient.getString();                       
       
      Serial.println(httpResponseCode);   
      Serial.println(response);
    }
    else {
      Serial.println("Error on HTTP request");   
    }
    httpClient.end(); //Free the resources
  }
}

// INIT
void setup() {

  // serial Monitor set Baud
  Serial.begin(115200);
  Serial.println();

  // Produkt Daten ermittelm  
  c_ChipModel = ESP.getChipModel();
  c_ChipRevision = ESP.getChipRevision();
  c_ProductMac = getMacAsString();
  c_LoopM = getLoopM();

  // Start SPIFFS
  initSPIFFS();

  // open myPref Preferences in RW
  prefs.begin("myPref", false);

  unsigned long long sysStart = prefs.getULong64("SystemStarts", 0);
  sysStart++;
  prefs.putULong64("SystemStarts", sysStart);

  ssid = prefs.getString("ssid", ssid);
  prefs.putString("ssid", ssid);

  pass = prefs.getString("pass", pass);
  prefs.putString("pass", pass);
   
  c_UhrH = prefs.getUInt("UhrH", d_UhrH);
  prefs.putUInt("UhrH", c_UhrH);

  c_UhrS = prefs.getUShort("UhrS", d_UhrS);
  prefs.putUShort("UhrS", c_UhrS);

  c_UhrV = prefs.getUShort("UhrV", d_UhrV);
  prefs.putUShort("UhrV", c_UhrV);

  c_SepH = prefs.getUInt("SepH", d_SepH);
  prefs.putUInt("SepH", c_SepH);

  c_SepS = prefs.getUShort("SepS", d_SepS);
  prefs.putUShort("SepS", c_SepS);

  c_SepV = prefs.getUShort("SepV", d_SepV);
  prefs.putUShort("SepV", c_SepV);

  c_SepState = prefs.getInt("SepState", d_SepState);
  prefs.putInt("SepState", c_SepState);

  c_gmtOffset = prefs.getInt("GmtOffset", d_gmtOffset);
  prefs.putInt("GmtOffset", c_gmtOffset);

  c_Hostname = prefs.getString("Hostname", d_Hostname);
  prefs.putString("Hostname", c_Hostname);

  c_ip = prefs.getUInt("StaticIP", (uint32_t)d_ip);
  prefs.putUInt("StaticIP", c_ip);

  c_softAPName = prefs.getString("softAPName", d_softAPName + "-" + c_ProductMac);
  prefs.putString("softAPName", c_softAPName);

  c_UserAgent = prefs.getString("UserAgent", d_UserAgent + "-" + c_ProductMac);
  prefs.putString("UserAgent", c_UserAgent);

  c_Product = prefs.getString("Product", c_ChipModel + "-" + c_ChipRevision + "-" + c_ProductMac);
  prefs.putString("Product", c_Product); 

  c_ProductName = prefs.getString("ProductName", d_ProductName);
  prefs.putString("ProductName", c_ProductName);    

  c_Firmware = d_Firmware;
  c_Build = d_Build;
 
  pinMode(UHR_PIN, OUTPUT);
  pixels.show();  // showtime - Init all pixels OFF

  // WiFi im Station Mode
  if(initWiFi()) {

    // DNS Name setzen
    if (!MDNS.begin(c_Hostname)) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
  
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });

    // Web Server WiFi URL
    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html", false, processor);
    });
    
    server.serveStatic("/", SPIFFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();

       // open myPref Preferences in RW
       prefs.begin("myPref", false);
  
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST Uhr H value
          if ((p->name() == "UH") && (c_UhrH != p->value().toInt())){
            c_UhrH = p->value().toInt();
            prefs.putUShort("UhrH", c_UhrH);
          }
          
          // HTTP POST Uhr S value
          if ((p->name() == "US") && (c_UhrS != p->value().toInt())){
            c_UhrS = p->value().toInt();
            prefs.putUShort("UhrS", c_UhrS);     
          }
          
          // HTTP POST Uhr V value
          if ((p->name() == "UV") && (c_UhrV != p->value().toInt())){
            c_UhrV = p->value().toInt();
            prefs.putUShort("UhrV", c_UhrV);         
          }
          
          // HTTP POST Trenner H value
          if ((p->name() == "TH") && (c_SepH != p->value().toInt())){
            c_SepH = p->value().toInt();
            prefs.putUShort("SepH", c_SepH);            
          }
          
          // HTTP POST Trenner S value
          if ((p->name() == "TS") && (c_SepS != p->value().toInt())){
            c_SepS = p->value().toInt();
            prefs.putUShort("SepS", c_SepS);              
          }
          
          // HTTP POST Trenner V value
          if ((p->name() == "TV") && (c_SepV != p->value().toInt())){
            c_SepV = p->value().toInt();
            prefs.putUShort("SepV", c_SepV);  
          }
          
          // HTTP POST Trenner Funktion
          if ((p->name() == "SEP_FNC") && (c_SepState != p->value().toInt())){
            c_SepState = p->value().toInt();
            prefs.putInt("SepState", c_SepState);
          }

          // HTTP POST Reset to Default
          if (p->name() == "RESETDEFAULT"){
            c_UhrH = d_UhrH;
            prefs.putUShort("UhrH", c_UhrH);
            c_UhrS = d_UhrS;
            prefs.putUShort("UhrS", c_UhrS);
            c_UhrV = d_UhrV;
            prefs.putUShort("UhrV", c_UhrV);                                    
            c_SepH = d_SepH;
            prefs.putUShort("SepH", c_SepH);             
            c_SepS = d_SepS;
            prefs.putUShort("SepS", c_SepS); 
            c_SepV = d_SepV;
            prefs.putUShort("SepV", c_SepV);  
            c_SepState = d_SepState;
            prefs.putInt("SepState", c_SepState);            
          }
        }
      }
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });

    server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();

       // open myPref Preferences in RW
       prefs.begin("myPref", false);
  
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == "ssid") {
            ssid = p->value().c_str();
            prefs.putString("ssid", ssid);
          }
          // HTTP POST pass value
          if (p->name() == "pass") {
            pass = p->value().c_str();
            prefs.putString("pass", pass);
          }
          // HTTP POST hostname value
          if (p->name() == "mdns") {
            c_Hostname = p->value().c_str();
            prefs.putString("Hostname", c_Hostname);
          }
        }
      }
      request->send(SPIFFS, "/wifimanager.html", "text/html", false, processor);
    });
    
    // Start the Server
    server.begin();

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
    
    // NTP Client starten
    timeClient.begin();
    // NTP Client Offset aktualisieren
    timeClient.setTimeOffset(prefs.getInt("GmtOffset"));
    timeClient.update();

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  ArduinoOTA.setHostname(d_Hostname.c_str());

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  }
 
  // WiFi im AP Mode
  else {

    Serial.println("Setting AP Mode!!!");
    WiFi.mode(WIFI_AP);
    
    // NULL sets an open Access Point
    WiFi.softAP(c_softAPName.c_str(), NULL); // Soft AP ohne Passwort

    // DNS Name setzen
    if (!MDNS.begin(d_Hostname)) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
    
    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html", false, processor);
    });
    
    // Web Server Restart URL
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/restart.html", "text/html", false, processor);
    });

    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == "ssid") {
            ssid = p->value().c_str();
            prefs.putString("ssid", ssid);
          }
          // HTTP POST pass value
          if (p->name() == "pass") {
            pass = p->value().c_str();
            prefs.putString("pass", pass);
          }
          // HTTP POST hostname value
          if (p->name() == "mdns") {
            c_Hostname = p->value().c_str();
            prefs.putString("Hostname", c_Hostname);
          }
        }
      }

      request->send(SPIFFS, "/restart.html", "text/html", false, processor);
      delay(1000);
      ESP.restart();
    });
    
    server.begin();

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
    
  }

  _FirstLoop = true;
  
  delay(100);
  
}

// RUN
void loop() {

  ArduinoOTA.handle();
 
  s = timeClient.getSeconds();
  m = timeClient.getMinutes();
  h = timeClient.getHours();

  // Jobs jede Sekunde und bei Start
  if (so != s) {
    so = s;
    MakePixels();
  }

  // Jobs jede Minute und bei Start
  if ((_FirstLoop == true) || (mo != m)) {
    mo = m;
    //putToKafka();
  }
  
  // Jobs jede Custom Minute und bei Start
  if ((_FirstLoop == true) || ((mo != m) && (m == c_LoopM))) {
    mo = m;
    //getMyInfo();
    //putToKafka();
  }

  // Jobs jede Stunde und bei Start
  if ((_FirstLoop == true) || ((ho != h) && (m == c_LoopM))) {
    ho = h;
    getMyInfo();
    putToKafka();
  }

  _FirstLoop = false;
  
}
