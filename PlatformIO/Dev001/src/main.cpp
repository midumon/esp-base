/*
  Basis
  MIt OTA Webseite

*/

// alle Ahängigkeiten
#pragma region Includes

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <AsyncElegantOTA.h>
#include <Adafruit_NeoPixel.h>
#include <TimeLib.h>
#include <NTPClient.h>

#pragma endregion Includes

// allgemien Variablen deklarieren
#pragma region Variablen

uint64_t ProductId = 0;
String ChipModel;
uint8_t ChipRevision;

// Ausgabe im Webterminal steuern 0==NIX 99=ALL
uint8_t WEB_DEBUG_LVL= 1; //Standard ist 1 Ausgabe Infoblock zur vollen Minute

struct tm timeinfo;

int i,s,m,h,so,mo,ho;
int s10,s1; //TENSecond ONESecond 
int m10,m1; //TENMinute ONESecond
int h10,h1; //Hour ..

// Pixel
#define PIN        22          // default PIN für alle IoT Türme
#define NUMPIXELS  46          // Anzahl LED Sandwich = 46 3D = 54

// LED Farben definieren
// HSV...
// Farbe HUE 0 - 65536
// Sättigung SAT 0- 255 
// Helligkeit VAL 0 - 255

uint16_t MaxB = 125;

uint16_t OffH = 0; 
uint8_t OffS = 0;
uint8_t OffV = 0;

uint16_t UhrH = 0; 
uint8_t UhrS = 0;
uint8_t UhrV = 25;

uint16_t SepH = 1500;
uint8_t SepS = 255;
uint8_t SepV = 25;
int SepState = 1; // 0 nie an 1 falshing 2 immer an

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define LED(x,h,s,v) pixels.setPixelColor(x,pixels.ColorHSV(h,s,v))

//54 LED Adressen [0,...,53] mit Zuordnung zu ihrer Funktion
byte oneSecond[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};     //09
byte tenSecond[] = {10, 11, 12, 13, 14};            //15,16
byte oneMinute[] = {17, 18, 19, 20, 21, 22, 23, 24, 25}; //26
byte tenMinute[] = {27, 28, 29, 30, 31};            //32,33
byte oneHour[] = {34, 35, 36, 37, 38, 39, 40, 41, 42}; //43
byte tenHour[] = {44, 45};
byte Separator[] = {9, 15, 16, 26, 32, 33, 43};
byte Restaurant[] = {46, 47, 48, 49};
byte Flugbefeuerung[] = {50, 51, 52, 53};


#pragma endregion Variablen

// NTP Client 
#pragma region -- NTP Client

const char* ntpServer = "pool.ntp.org";
const int   gmtOffset_sec = 7200;
const int   daylightOffset_sec = 60000;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);


#pragma endregion ## NTP Client


#pragma region -- Filesystem

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

#pragma endregion -- Filsesystem

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

#pragma region -- Variablen über HTML Console ändern

void writeFromWebConsole(String data){

// SepState


  if(data.startsWith("SepState::")){

    // SepState auwerten 0-9
    WebSerial.print("New SepState : ");
    WebSerial.print(data.substring(10,11));
    SepState = data.substring(10,11).toInt();

}

}

#pragma endregion

// alles rund um den Webserver
#pragma region -- Webserver

// Set number of outputs
#define NUM_OUTPUTS  4

// Assign each GPIO to an output
int outputGPIOs[NUM_OUTPUTS] = {2, 4, 12, 14};

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


String getOutputStates(){
  JSONVar myArray;
  for (int i =0; i<NUM_OUTPUTS; i++){
    myArray["gpios"][i]["output"] = String(outputGPIOs[i]);
    myArray["gpios"][i]["state"] = String(digitalRead(outputGPIOs[i]));
  }
  String jsonString = JSON.stringify(myArray);
  return jsonString;
}

void notifyClients(String state) {
  ws.textAll(state);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "states") == 0) {
      notifyClients(getOutputStates());
    }
    else{
      int gpio = atoi((char*)data);
      digitalWrite(gpio, !digitalRead(gpio));
      notifyClients(getOutputStates());
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len){
  
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }

  // RAW Daten ab LVL 4 ausgeben
  if(WEB_DEBUG_LVL > 4){
    WebSerial.println("#-REC-RAW-START-#");
    WebSerial.println(d);
    WebSerial.println("#-REC-RAW-ENDE-#");
  }

    if((d.indexOf("::") > 0 ) && (d.length() > 3)){ 

    WebSerial.println("#-MATCH-START-");
    WebSerial.println(d.indexOf("::"));
    WebSerial.println(d.length());
    WebSerial.println("#-MATCH-END-");

    // hie alle Variablen beschreibbar machen
    // overwrite Vars
    writeFromWebConsole(d);

  }
  else{
     WebSerial.println("#-NOT-ALLOW-#");
  }

}

#pragma endregion -- Webserver

// Uhr Funktion
#pragma region -- Uhr

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
 (s1 <=i?LED(oneSecond[i],OffH,OffS,OffV):LED(oneSecond[i],UhrH,UhrS,UhrV));

 for(i=0;i<sizeof(tenSecond);i++)
 (s10<=i?LED(tenSecond[i],OffH,OffS,OffV):LED(tenSecond[i],UhrH,UhrS,UhrV));

 for(i=0;i<sizeof(oneMinute);i++)
 (m1 <=i?LED(oneMinute[i],OffH,OffS,OffV):LED(oneMinute[i],UhrH,UhrS,UhrV));

 for(i=0;i<sizeof(tenMinute);i++)
 (m10<=i?LED(tenMinute[i],OffH,OffS,OffV):LED(tenMinute[i],UhrH,UhrS,UhrV));

 for(i=0;i<sizeof(oneHour);i++)
 (h1 <=i?LED(oneHour[i],OffH,OffS,OffV):LED(oneHour[i],UhrH,UhrS,UhrV));

 for(i=0;i<sizeof(tenHour);i++)
 (h10<=i?LED(tenHour[i],OffH,OffS,OffV):LED(tenHour[i],UhrH,UhrS,UhrV));

// Separator
 if((((s1==9) && (s10==5)) || ((m1==9) && (m10==5)) && SepState==1) || SepState==2 ){
  //Serial.println("Falsh Separator...");
  for(int i=0;i<sizeof(Separator);i++)LED(Separator[i],SepH,SepS,SepV);
 }
 else{
  for(int i=0;i<sizeof(Separator);i++)LED(Separator[i],OffH,OffS,OffV);
  }

 pixels.show(); // showtime
 
}

#pragma endregion -- Uhr


void setup(void) {
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Set GPIOs as outputs
  for (int i =0; i<NUM_OUTPUTS; i++){
    pinMode(outputGPIOs[i], OUTPUT);
  }

  initSPIFFS();
  initWiFi();
  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html",false);
  });

  server.serveStatic("/", SPIFFS, "/");

  // Start ElegantOTA
  AsyncElegantOTA.begin(&server);

  // WebSerial is accessible at "<IP Address>/webserial" in browser
  WebSerial.begin(&server);
  /* Attach Message Callback */
  WebSerial.msgCallback(recvMsg);
  
  // Start server
  server.begin();

  Serial.println("HTTP server started");

  // Produkt Daten ermittelm
  ProductId = ESP.getEfuseMac();
  ChipModel = ESP.getChipModel();
  ChipRevision = ESP.getChipRevision();

	Serial.print("Chip model: "); 
  Serial.println(ChipModel);
  Serial.print("Chip rev.: "); 
  Serial.println(ChipRevision);
  Serial.print("Product ID: "); 
  Serial.println(ProductId);

  timeClient.begin();
  timeClient.forceUpdate();

  so=99;
  mo=99;
  ho=99;

}


void loop(void) {

  ws.cleanupClients();

  s=timeClient.getSeconds();
  m=timeClient.getMinutes();
  h=timeClient.getHours();

  if(so!=s){
    so=s; 
    MakePixels();
  }

  // jede Minute einmalig ausführen
  if(mo!=m){
    mo=m; 
    // Info Print auf dem WebTerminal wen Web-Debug-Level > 0
    if(WEB_DEBUG_LVL > 0){
      WebSerial.println("#-INFO-START-#");
      WebSerial.print(F("IP address: "));
      WebSerial.println(WiFi.localIP());
      WebSerial.printf("Millis=%lu\n", millis());
      WebSerial.printf("Free heap=[%u]\n", ESP.getFreeHeap());
      WebSerial.print("Chip model: "); 
      WebSerial.println(ChipModel);
      WebSerial.print("Chip rev.: "); 
      WebSerial.println(ChipRevision);
      WebSerial.print("Product ID: "); 
      WebSerial.println(ProductId);
      WebSerial.println("#-INFO-ENDE-#");
    }

  }

  delay(20);  // <- fixes some issues with WiFi stability

}