/* ESP8266 Alle Funktionen...

- WiFi Portal
- NTPClient
- Externe IP ermitteln
- Zeitzone ermitteln
- MQTT Client

*/

#pragma region Includes

#include <Arduino.h>
#include <Time.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <ArduinoJSON.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPUpdateServer.h>

Preferences preferences;

unsigned long current_millis = 0;
unsigned long last_mqtt_millis = 0;
unsigned long last_loop_millis = 0;
unsigned long last_pixel_millis = 0;

String tmpString;

const char* host = "Rheinturm";

WebServer httpServer(80);
HTTPUpdateServer httpUpdater;

#pragma endregion Includes

#pragma region -- Firmware and other fixed values

uint8_t baseMac[6];
char baseMacChr;

// Firmware
String Firmware = "V.0.0.3-dev003";
String Product;
String ProductId;
String ProductMac;
String ChipModel;
uint8_t ChipRevision;

const char *ProduktName = "TestDev003";

bool checkExternalIP; // if true check external IP

#pragma endregion -- Firmware

// NTP Client 
#pragma region -- NTP Client

const char* ntpServer = "pool.ntp.org";
int32_t DefaultgmtOffset_sec = 0;
int32_t gmtOffset_sec = 0;
int32_t daylightOffset_sec = 60000;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);


#pragma endregion ## NTP Client

#pragma region -- WiFi
// Initialize WiFi

const char* ssid = "B1-Access";
const char* password = "wlanPWD@unifi";

void initWiFi() {
  WiFi.mode(WIFI_AP_STA);
 // WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.waitForConnectResult()!= WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.print('.');
    delay(500);
  }
  Serial.print(' ');
  Serial.println(WiFi.localIP());
}

#pragma endregion -- WiFi

#pragma region Test-Variablen



#pragma endregion Test-Variablen

// Uhr Funktion
#pragma region -- Uhr

int i,s,m,h,so,mo,ho;
int s10,s1; //TENSecond ONESecond 
int m10,m1; //TENMinute ONESecond
int h10,h1; //Hour ..

// Pixel
#ifdef MH_MINI_DEFAULT
#define UHR_PIN        16          // default PIN für alle IoT Türme
#endif

#ifdef SEEED_ESP32C2_DEFAULT
#define UHR_PIN        D10          // default PIN für alle IoT Türme
#endif

#define NUMPIXELS  46          // Anzahl LED Sandwich = 46 3D = 54

// LED Farben definieren
// HSV...
// h == Farbe HUE 0 - 65536
// s == Sättigung SAT 0- 255 
// v == Helligkeit VAL 0 - 255

// default LED Werte
uint16_t MaxB = 125; // maximale Helligkeit

uint16_t offH = 0; 
uint8_t offS = 0;
uint8_t offV = 0;
// Farbe der LED's der Uhr
uint16_t DefaultUhrH = 0; 
uint8_t DefaultUhrS = 50;
uint8_t DefaultUhrV = 25;
uint16_t UhrH; 
uint8_t UhrS;
uint8_t UhrV;
// Farbe der LED's der Trenner
uint16_t DefaultSepH = 1500;
uint8_t DefaultSepS = 255;
uint8_t DefaultSepV = 25;
uint16_t SepH;
uint8_t SepS;
uint8_t SepV;

int DefaultSepState = 1; // 0 nie an 1 falsh 2 immer an
int SepState;

// 46 LED Adressen [0,...,46] mit Zuordnung zu ihrer Funktion
byte oneSecond[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};     //09
byte tenSecond[] = {10, 11, 12, 13, 14};            //15,16
byte oneMinute[] = {17, 18, 19, 20, 21, 22, 23, 24, 25}; //26
byte tenMinute[] = {27, 28, 29, 30, 31};            //32,33
byte oneHour[] = {34, 35, 36, 37, 38, 39, 40, 41, 42}; //43
byte tenHour[] = {44, 45};
byte Separator[] = {9, 15, 16, 26, 32, 33, 43};

Adafruit_NeoPixel pixels(NUMPIXELS, UHR_PIN, NEO_GRB + NEO_KHZ800);
#define LED(x,h,s,v) pixels.setPixelColor(x,pixels.ColorHSV(h,s,v))

void MakePixels(){

  // Sekunden
  s=timeClient.getSeconds();
  s1 = s % 10; 
  s10 = s / 10;

  // Minuten
  m=timeClient.getMinutes();
  m1 = m % 10; 
  m10 = m / 10;

  // Stunden
  h=timeClient.getHours();
  h1 = h % 10;
  h10 = h / 10;

  // Set Pixels AN/AUS
  for(i=0;i<sizeof(oneSecond);i++)
  (s1 <=i?LED(oneSecond[i],offH,offS,offV):LED(oneSecond[i],UhrH,UhrS,UhrV));

  for(i=0;i<sizeof(tenSecond);i++)
  (s10<=i?LED(tenSecond[i],offH,offS,offV):LED(tenSecond[i],UhrH,UhrS,UhrV));

  for(i=0;i<sizeof(oneMinute);i++)
  (m1 <=i?LED(oneMinute[i],offH,offS,offV):LED(oneMinute[i],UhrH,UhrS,UhrV));

  for(i=0;i<sizeof(tenMinute);i++)
  (m10<=i?LED(tenMinute[i],offH,offS,offV):LED(tenMinute[i],UhrH,UhrS,UhrV));

  for(i=0;i<sizeof(oneHour);i++)
  (h1 <=i?LED(oneHour[i],offH,offS,offV):LED(oneHour[i],UhrH,UhrS,UhrV));

  for(i=0;i<sizeof(tenHour);i++)
  (h10<=i?LED(tenHour[i],offH,offS,offV):LED(tenHour[i],UhrH,UhrS,UhrV));

  // Separator
  if(((s1==9) && (s10==5) && SepState==1) || ((m1==9) && (m10==5) && SepState==1) || SepState==2){
  for(int i=0;i<sizeof(Separator);i++)LED(Separator[i],SepH,SepS,SepV);
  }
  else{
    for(int i=0;i<sizeof(Separator);i++)LED(Separator[i],offH,offS,offV);
  }

  delay(20);
  pixels.show(); // showtime
 
}

#pragma endregion -- Uhr

#pragma region -- MQTT Client

// https://github.com/tuielectronics/ESP8266-mqtt-over-websocket/blob/master/ESP8266-mqtt-over-websocket.ino

String HttpUserAgent;
const char *MQTT_HOST = "mqtt.rheinturm.cloud";
const int MQTT_PORT = 8883;
//const int MQTT_PORT = 443;

const char *MQTT_CLIENT_ID;
const char *MQTT_USER = "turm"; // leave blank if no credentials used
const char *MQTT_PASS = "turmPWD@mqtt"; // leave blank if no credentials used
const char *MQTT_PATH = "/ws";
unsigned long long MSG_COUNT = 0;

char MQTT_Buffer[4096];
size_t MQTT_Buffer_size;
DynamicJsonDocument DJD_tdx(4096);
DynamicJsonDocument DJD_fdx(4096);
DynamicJsonDocument DJD_idx(4096);

String StringTopicTDX;
String StringTopicFDX;
String StringTopicIDX;
const char *MQTT_tdx;
const char *MQTT_fdx;
const char *MQTT_idx;

WiFiClientSecure SecureClient;
PubSubClient MQTTClient(SecureClient);

//WiFiClient Client;
//WebSocketsClient wsClient(Client, );
//PubSubClient MQTTClient(wsClient);


void mqtt_connect(){

  while (!MQTTClient.connected()) {
    SecureClient.setInsecure();
    if (MQTTClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      MQTTClient.subscribe(MQTT_tdx);
    } 
  }

}

void recMqttCallback(const char* topic, byte* payload, unsigned int length) {
 
  String tmpString;

  auto error = deserializeJson(DJD_tdx, (const byte*)payload, length);

  if (error) {
    Serial.print(F("MQTT Reponse deserializeJson failed with code: "));
    Serial.println(error.c_str());
    return;
  }
  else {

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i=0;i<length;i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();

    // open myPref Preferences in RW 
    preferences.begin("myPref", false);

    MSG_COUNT++;
    DJD_fdx["MSG_COUNT"] = MSG_COUNT;

    if(!DJD_tdx["device"]["custom"]["name"].isNull()){
      String tmpString = DJD_tdx["device"]["custom"]["name"];
      preferences.putString("CustomName", tmpString);
      DJD_fdx["device"]["custom"]["name"] = preferences.getString("CustomName");
    }

    if(!DJD_tdx["function"]["uhr"]["h"].isNull()){
      preferences.putUInt("UhrH", DJD_tdx["function"]["uhr"]["h"]);
      UhrH = DJD_tdx["function"]["uhr"]["h"];
      DJD_fdx["device"]["preferences"]["uhr"]["h"] = preferences.getUInt("UhrH");
    }

    if(!DJD_tdx["function"]["uhr"]["s"].isNull()){
      preferences.putUShort("UhrS", DJD_tdx["function"]["uhr"]["s"]); 
      UhrS = DJD_tdx["function"]["uhr"]["s"];
      DJD_fdx["device"]["preferences"]["uhr"]["s"] = preferences.getUShort("UhrS");
    }

    if(!DJD_tdx["function"]["uhr"]["v"].isNull()){
      preferences.putUShort("UhrV", DJD_tdx["function"]["uhr"]["v"]);
      UhrV = DJD_tdx["function"]["uhr"]["v"];
      DJD_fdx["device"]["preferences"]["uhr"]["v"] = preferences.getUShort("UhrV");
    }

    if(!DJD_tdx["function"]["sep"]["h"].isNull()){
      preferences.putUInt("SepH", DJD_tdx["function"]["sep"]["h"]);
      SepH = DJD_tdx["function"]["sep"]["h"];
      DJD_fdx["device"]["preferences"]["sep"]["h"] = preferences.getUInt("SepH");
    }

    if(!DJD_tdx["function"]["sep"]["s"].isNull()){
      preferences.putUShort("SepS", DJD_tdx["function"]["sep"]["s"]); 
      SepS = DJD_tdx["function"]["sep"]["s"];
      DJD_fdx["device"]["preferences"]["sep"]["s"] = preferences.getUShort("SepS");
    }

    if(!DJD_tdx["function"]["sep"]["v"].isNull()){
      preferences.putUShort("SepV", DJD_tdx["function"]["sep"]["v"]);
      SepV = DJD_tdx["function"]["sep"]["v"];
      DJD_fdx["device"]["preferences"]["sep"]["v"] = preferences.getUShort("SepV");
    }

    if(!DJD_tdx["function"]["sep"]["state"].isNull()){
      preferences.putUInt("SepState", DJD_tdx["function"]["sep"]["state"]);
      SepState = DJD_tdx["function"]["sep"]["state"];
      DJD_fdx["device"]["preferences"]["sep"]["state"] = preferences.getUInt("SepState");
    }

    if(!DJD_tdx["function"]["ntp"]["offset"].isNull()){

      gmtOffset_sec = DJD_tdx["function"]["ntp"]["offset"];
      preferences.putInt("GmtOffset", gmtOffset_sec);

      Serial.print("New Offset: ");
      Serial.println(gmtOffset_sec);
      Serial.print("Persistent Offset: ");
      Serial.println(preferences.getInt("GmtOffset"));

      DJD_fdx["device"]["ntpoffset"] = preferences.getInt("GmtOffset");
      
      timeClient.setTimeOffset(preferences.getInt("GmtOffset"));
      timeClient.forceUpdate();
    }

    if(!DJD_tdx["function"]["device"]["restart"].isNull()){

      ESP.restart();

    }

    if(!DJD_tdx["function"]["device"]["reset"].isNull()){

      preferences.clear();
      ESP.restart();

    }

    // open myPref Preferences in RW 
    preferences.end();

  }
}

#pragma endregion ## MQTT Client

#pragma region -- external IP

void getMyInfo(){

  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status

    HTTPClient httpClient;

    // "https://api.ipify.org/?format=json" //Specify the URL as JSON
    // "https://api.ipify.org/" //Specify the URL as TEXT

    // ipinfo.io
    // Token 634c5b93c04f6a
    // https://ipinfo.io?token=634c5b93c04f6a

    // https://ipapi.co/json

    String CheckIpUrlState = "none";              
    String CheckIpUrl = "https://ipapi.co/json";

    httpClient.begin(CheckIpUrl); //Specify the URL
    int httpCode = httpClient.GET();                        //Make the request

    if (httpCode > 0) { //Check for the returning code

      String payload = httpClient.getString();
      Serial.println(httpCode);
      Serial.println(payload);

      DJD_idx.clear();

      auto error = deserializeJson(DJD_idx, (const byte*)payload.c_str(), strlen(payload.c_str()));

      if (error) {
        Serial.print(F("IPAPI Reponse deserializeJson failed with code: "));
        Serial.println(error.c_str());

        CheckIpUrlState = error.c_str();

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

        CheckIpUrlState = "OK";

        // open myPref Preferences in RW 
        preferences.begin("myPref", false);

        // behelf Der UTC Offset wird vom node Red errechnet und übert tdx an des Device übermittelt.
        // dazurch kann man den GEO Data Provider tauschen ohne die Firmware zu ändern

        if(!DJD_idx["utc_offset"].isNull()){

          String tmp = DJD_idx["utc_offset"];

          if(tmp.length() == 5){

            int offset = tmp.substring(1,3).toInt()*3600 + tmp.substring(3,5).toInt();

            String offsetString = tmp.substring(0,1) + offset;

            DJD_fdx["device"]["ntpoffset"] = offsetString;

            gmtOffset_sec = offsetString.toInt();
            preferences.putInt("GmtOffset", gmtOffset_sec);

            timeClient.setTimeOffset(preferences.getInt("GmtOffset"));
            timeClient.forceUpdate();

          }
          else{
            DJD_fdx["device"]["utcoffset"] = "noValid";
          }
        }
        else{
          DJD_fdx["device"]["utcoffset"] = "";
        }

        // open myPref Preferences in RW 
        preferences.end();

        // empfangene GEO Infos an den Server senden
        // Device ID ergänzen
        DJD_idx["device"]["id"] = ProductMac;
        DJD_idx["device"]["ipcheckurl"] = CheckIpUrl;
        DJD_idx["device"]["ipcheckstate"] = CheckIpUrlState;

        DJD_idx.shrinkToFit();

        MQTT_Buffer_size = serializeJson(DJD_idx, MQTT_Buffer);
        MQTTClient.publish(MQTT_idx, MQTT_Buffer, MQTT_Buffer_size);

      }

      DJD_fdx["device"]["ipcheckurl"] = CheckIpUrl;
      DJD_fdx["device"]["ipcheckstate"] = CheckIpUrlState;

    }
    else {
      Serial.println("Error on HTTP request");
    }

    httpClient.end(); //Free the resources

  }

}

#pragma endregion

#pragma region -- Hilfsfunktionen

String getMacAsString() {

    uint8_t baseMac[6];
    // Get MAC address for WiFi station
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    char baseMacChr[18] = {0};
    sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);

    return String(baseMacChr);
}

#pragma endregion -- Hilfsfunktionen

void setup() {

  // serial Monitor set Baud
  Serial.begin(115200);
  Serial.println();

  pinMode(UHR_PIN, OUTPUT);
  pixels.show(); // showtime - Init all pixels OFF

  // open myPref Preferences in RW 
  preferences.begin("myPref", false);

  unsigned long long sysStart = preferences.getULong64("SystemStarts", 0);
  sysStart++;
  preferences.putULong64("SystemStarts", sysStart);

  UhrH = preferences.getUInt("UhrH", DefaultUhrH);
  preferences.putUInt("UhrH", UhrH);

  UhrS = preferences.getUShort("UhrS", DefaultUhrS);
  preferences.putUShort("UhrS", UhrS);

  UhrV = preferences.getUShort("UhrV", DefaultUhrV);
  preferences.putUShort("UhrV", UhrV);

  SepH = preferences.getUInt("SepH", DefaultSepH);
  preferences.putUInt("SepH", SepH);

  SepS = preferences.getUShort("SepS", DefaultSepS);
  preferences.putUShort("SepS", SepS);

  SepV = preferences.getUShort("SepV", DefaultSepV);
  preferences.putUShort("SepV", SepV);

  SepState = preferences.getInt("SepState", DefaultSepState);
  preferences.putInt("SepState", SepState);

  Serial.print("Old Offset: ");
  Serial.println(gmtOffset_sec);
  Serial.print("Old Persistent Offset: ");
  Serial.println(preferences.getInt("GmtOffset"));

  delay(100);

  gmtOffset_sec = preferences.getInt("GmtOffset", DefaultgmtOffset_sec);
  preferences.putInt("GmtOffset", gmtOffset_sec);

  Serial.print("New Offset: ");
  Serial.println(gmtOffset_sec);
  Serial.print("New Persistent Offset: ");
  Serial.println(preferences.getInt("GmtOffset"));

  delay(100);

  // Produkt Daten ermittelm  
  ChipModel = ESP.getChipModel();
  ChipRevision = ESP.getChipRevision();
  ProductMac = getMacAsString();

  Product = ChipModel + "-" + ChipRevision + "-" + ProductMac;
  HttpUserAgent = "MAF-RT/" + ProductMac;
 
  MQTT_CLIENT_ID = ProductMac.c_str();

  StringTopicFDX = "Px/" + ProductMac + "/fdx";
  MQTT_fdx = StringTopicFDX.c_str();
 
  StringTopicTDX = "Px/" + ProductMac + "/tdx";
  MQTT_tdx = StringTopicTDX.c_str();

  StringTopicIDX = "Px/" + ProductMac + "/idx";
  MQTT_idx = StringTopicIDX.c_str();

  Serial.print("Product ID: "); Serial.println(ProductId);
  Serial.print("Product: "); Serial.println(Product);
  Serial.print("UserAgent: "); Serial.println(HttpUserAgent);

  JsonObject fdx = DJD_fdx.to<JsonObject>();
  JsonObject DeviceInfo = fdx.createNestedObject("device");
  DeviceInfo["id"] = ProductMac;
  DeviceInfo["model"] = ChipModel;
  DeviceInfo["revision"] = ChipRevision;
  DeviceInfo["firmware"] = Firmware;
  DeviceInfo["SystemStarts"] = preferences.getULong64("SystemStarts");
  DeviceInfo["Name"] = ProduktName;
  DeviceInfo["MSG_COUNT"] = MSG_COUNT;
  DeviceInfo["Useragent"] = HttpUserAgent;

  preferences.end(); 

  initWiFi();
 
  timeClient.begin();
  timeClient.setTimeOffset(gmtOffset_sec);
  timeClient.forceUpdate();

  SecureClient.setInsecure();
  MQTTClient.setServer(MQTT_HOST, MQTT_PORT);
  MQTTClient.setBufferSize(4096);
  MQTTClient.setCallback(recMqttCallback);

  mqtt_connect();

  getMyInfo();

  Serial.println("Setup ends now.");

  so=99;
  mo=99;
  ho=99;

  if (MDNS.begin(host)) {
    Serial.println("mDNS responder started");
  }

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
}

void loop() {

  s=timeClient.getSeconds();
  m=timeClient.getMinutes();
  h=timeClient.getHours();

  current_millis = millis();
  if(so!=s){
    so=s; 
    last_pixel_millis = current_millis;
    MakePixels();
  }

  if (!MQTTClient.connected()){
    mqtt_connect();
  }
  else{
   MQTTClient.loop();
  }

  current_millis = millis();
  if (current_millis - last_mqtt_millis > 5600) {

    last_mqtt_millis = current_millis;

    MQTT_Buffer_size = serializeJson(DJD_fdx, MQTT_Buffer);
    MQTTClient.publish(MQTT_fdx, MQTT_Buffer, MQTT_Buffer_size);

    }

  current_millis = millis();
  last_loop_millis = current_millis;

  delay(100);

}