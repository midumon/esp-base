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
#include <WiFiClientSecure.h>
#include <HttpClient.h>
#include <ArduinoJSON.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

Preferences preferences;


unsigned long current_millis = 0;
unsigned long last_mqtt_millis = 0;
unsigned long last_loop_millis = 0;
unsigned long last_pixel_millis = 0;

String tmpString;

#pragma endregion Includes

#pragma region -- Firmware and other fixed values

uint8_t baseMac[6];
char baseMacChr;

// Firmware
String Firmware = "V.0.0.1-P3600";
String Product;
String ProductId;
String ProductMac;
String ChipModel;
uint8_t ChipRevision;

const char *ProduktName = "P3600";

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
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
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
#define PIN        16          // default PIN für alle IoT Türme
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
byte oneSecond[] = {45, 44, 43, 42, 41, 40, 39, 38, 37};
byte tenSecond[] = {35, 34, 33, 32, 31};
byte oneMinute[] = {28, 27, 26, 25, 24, 23, 22, 21, 20};
byte tenMinute[] = {18, 17, 16, 15, 14};
byte oneHour[] = {11, 10, 9, 8, 7, 6, 5, 4, 3};
byte tenHour[] = {1, 0};
byte Separator[] = {36, 30, 29, 19, 13, 12, 2};

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);
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
const char *MQTT_CLIENT_ID;
const char *MQTT_USER = "turm"; // leave blank if no credentials used
const char *MQTT_PASS = "turmPWD@mqtt"; // leave blank if no credentials used

unsigned long long MSG_COUNT = 0;

char MQTT_Buffer[4096];
size_t MQTT_Buffer_size;
DynamicJsonDocument MQTT_in(4096);
DynamicJsonDocument MQTT_out(4096);

String SubTopicString;
const char *MQTT_SUB_TOPIC;
String PubTopicString;
const char *MQTT_PUB_TOPIC;

WiFiClientSecure SecureClient;
PubSubClient MQTTClient(SecureClient);

void mqtt_connect()
{
    while (!MQTTClient.connected()) {
    Serial.println("MQTT connecting");
    if (MQTTClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {

      MQTTClient.subscribe(MQTT_SUB_TOPIC);

    } 
  }  
}

void recMqttCallback(const char* topic, byte* payload, unsigned int length) {
 
  String tmpString;

  auto error = deserializeJson(MQTT_in, (const byte*)payload, length);

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
    MQTT_out["loop"]["MSG_COUNT"] = MSG_COUNT;

    if(!MQTT_in["device"]["custom"]["name"].isNull()){
      String tmpString = MQTT_in["device"]["custom"]["name"];
      preferences.putString("CustomName", tmpString);
      MQTT_out["device"]["custom"]["name"] = preferences.getString("CustomName");
    }

    if(!MQTT_in["function"]["uhr"]["h"].isNull()){
      preferences.putUInt("UhrH", MQTT_in["function"]["uhr"]["h"]);
      UhrH = MQTT_in["function"]["uhr"]["h"];
      MQTT_out["device"]["preferences"]["uhr"]["h"] = preferences.getUInt("UhrH");
    }

    if(!MQTT_in["function"]["uhr"]["s"].isNull()){
      preferences.putUShort("UhrS", MQTT_in["function"]["uhr"]["s"]); 
      UhrS = MQTT_in["function"]["uhr"]["s"];
      MQTT_out["device"]["preferences"]["uhr"]["s"] = preferences.getUShort("UhrS");
    }

    if(!MQTT_in["function"]["uhr"]["v"].isNull()){
      preferences.putUShort("UhrV", MQTT_in["function"]["uhr"]["v"]);
      UhrV = MQTT_in["function"]["uhr"]["v"];
      MQTT_out["device"]["preferences"]["uhr"]["v"] = preferences.getUShort("UhrV");
    }

    if(!MQTT_in["function"]["sep"]["h"].isNull()){
      preferences.putUInt("SepH", MQTT_in["function"]["sep"]["h"]);
      SepH = MQTT_in["function"]["sep"]["h"];
      MQTT_out["device"]["preferences"]["sep"]["h"] = preferences.getUInt("SepH");
    }

    if(!MQTT_in["function"]["sep"]["s"].isNull()){
      preferences.putUShort("SepS", MQTT_in["function"]["sep"]["s"]); 
      SepS = MQTT_in["function"]["sep"]["s"];
      MQTT_out["device"]["preferences"]["sep"]["s"] = preferences.getUShort("SepS");
    }

    if(!MQTT_in["function"]["sep"]["v"].isNull()){
      preferences.putUShort("SepV", MQTT_in["function"]["sep"]["v"]);
      SepV = MQTT_in["function"]["sep"]["v"];
      MQTT_out["device"]["preferences"]["sep"]["v"] = preferences.getUShort("SepV");
    }

    if(!MQTT_in["function"]["sep"]["state"].isNull()){
      preferences.putUInt("SepState", MQTT_in["function"]["sep"]["state"]);
      SepState = MQTT_in["function"]["sep"]["state"];
      MQTT_out["device"]["preferences"]["sep"]["state"] = preferences.getUInt("SepState");
    }

    if(!MQTT_in["function"]["ntp"]["offset"].isNull()){

      gmtOffset_sec = MQTT_in["function"]["ntp"]["offset"];
      preferences.putInt("GmtOffset", gmtOffset_sec);

      Serial.print("New Offset: ");
      Serial.println(gmtOffset_sec);
      Serial.print("Persistent Offset: ");
      Serial.println(preferences.getInt("GmtOffset"));

      MQTT_out["device"]["ntpoffset"] = preferences.getInt("GmtOffset");
      
      MQTT_out["device"]["preferences"]["ntp"]["offset"] = preferences.getInt("GmtOffset");
      timeClient.setTimeOffset(preferences.getInt("GmtOffset"));
      timeClient.forceUpdate();
    }

    if(!MQTT_in["function"]["device"]["restart"].isNull()){

      ESP.restart();

    }

    if(!MQTT_in["function"]["device"]["reset"].isNull()){

      preferences.clear();
      ESP.restart();

    }

    // open myPref Preferences in RW 
    preferences.end();

  }
}

#pragma endregion ## MQTT Client

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

  pinMode(16, OUTPUT);
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

  PubTopicString = "Px/" + ProductMac + "/fdx";
  MQTT_PUB_TOPIC = PubTopicString.c_str();
 
  SubTopicString = "Px/" + ProductMac + "/tdx";
  MQTT_SUB_TOPIC = SubTopicString.c_str();

  Serial.print("Product ID: "); Serial.println(ProductId);
  Serial.print("Product: "); Serial.println(Product);
  Serial.print("UserAgent: "); Serial.println(HttpUserAgent);

  JsonObject root = MQTT_out.to<JsonObject>();
  JsonObject DeviceInfo = root.createNestedObject("device");
  DeviceInfo["id"] = ProductMac;
  DeviceInfo["model"] = ChipModel;
  DeviceInfo["revision"] = ChipRevision;
  DeviceInfo["firmware"] = Firmware;
  DeviceInfo["SystemStarts"] = preferences.getULong64("SystemStarts");
  DeviceInfo["Name"] = ProduktName;

  JsonObject LoopInfo = root.createNestedObject("loop");
  LoopInfo["MSG_COUNT"] = MSG_COUNT;
  LoopInfo["Useragent"] = HttpUserAgent;

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

  Serial.println("Setup ends now.");

  so=99;
  mo=99;
  ho=99;

}

void loop() {

  s=timeClient.getSeconds();
  m=timeClient.getMinutes();
  h=timeClient.getHours();

  current_millis = millis();
  if(so!=s){
    so=s; 

    Serial.print("Pixel Time Begin: ");
    Serial.println(current_millis - last_pixel_millis);
    last_pixel_millis = current_millis;

    MakePixels();

    Serial.print("Pixel Time End: ");
    Serial.println(millis() - last_pixel_millis);

  }

  if (!MQTTClient.connected()){
  mqtt_connect();
  }
  else{
   MQTTClient.loop();
  }

  current_millis = millis();
  if (current_millis - last_mqtt_millis > 5600) {

    //Serial.print("MQTT Client Time: ");
    //Serial.println(current_millis - last_mqtt_millis);
    last_mqtt_millis = current_millis;

    MQTT_Buffer_size = serializeJson(MQTT_out, MQTT_Buffer);
    MQTTClient.publish(MQTT_PUB_TOPIC, MQTT_Buffer, MQTT_Buffer_size);

    //Serial.print("MQTT Buffer Length: ");
    //Serial.println(MQTT_Buffer_size);
  
    }

  current_millis = millis();
  //Serial.print("MQTT Client Time: ");
  //Serial.println(current_millis - last_loop_millis);
  last_loop_millis = current_millis;

  delay(100);

}