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
#include "WiFiClientSecure.h"
#include <ESPAsyncWebServer.h>
//#include <WebSerial.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <AsyncElegantOTA.h>
#include <Adafruit_NeoPixel.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <PubSubClient.h>

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

// LED Farben definieren
int UhrR = 17;
int UhrG = 17;
int UhrB = 17;

int SeparatorR = 0;
int SeparatorG = 0;
int SeparatorB = 20;

int RestaurantR = 25; 
int RestaurantG = 0;
int RestaurantB = 25;

int FlugbefeuerungR = 12;
int FlugbefeuerungG = 0;
int FlugbefeuerungB = 0;

int FlugbefeuerungDimR = 3;
int FlugbefeuerungDimG = 0;
int FlugbefeuerungDimB = 0;

// maximale Leistung R + B + G / max. 100
// Standard USB NT 200 mA
int MaxRGB = 100;

// Pixel, Farben etc.
#define PIN           16
#define NUMPIXELS     46
#define C0        Color(0,0,0) // LED aus
#define C1        Color(UhrR,UhrG,UhrB) // 25/25/25 150 mA max for all
#define C2        Color(SeparatorR,SeparatorG,SeparatorB) // 
#define C3        Color(RestaurantR,RestaurantG,RestaurantB) // 
#define C4        Color(FlugbefeuerungR,FlugbefeuerungG,FlugbefeuerungB) // 
#define C5        Color(FlugbefeuerungDimR,FlugbefeuerungDimG,FlugbefeuerungDimB) // 
#define AN        pixels.C1
#define AUS       pixels.C0
#define SEP    pixels.C2
#define RES    pixels.C3
#define FON    pixels.C4
#define FDM    pixels.C5
#define LED(x,y)  pixels.setPixelColor(x,y)

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

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "B1-Access";
const char* password = "wlanPWD@unifi";

#pragma endregion Variablen

//MQTT Anbindung
#pragma region -- MQTT

WiFiClientSecure secClient;
PubSubClient mqttClient(secClient); 

#define CNT_TOP    "test/counter"
#define SET_TOP     "test/set" 

long lastMsg = 0;
char msg[20];
int counter = 0;

String MQTT_SERVER = "mqtt.rheinturm.cloud";
int MQTT_PORT = 443;

void receivedCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received: ");
  Serial.println(topic);

  Serial.print("payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

void mqttconnect() {
  // set Certificate Validation OFF
  secClient.setInsecure();
  
  while (!mqttClient.connected()) {
    Serial.print("MQTT connecting ...");
    /* client ID */
    String clientId = "ESPxTesting";
    String user = "turm";
    String password = "turmPWD@mqtt";
    /* connect now */
    if (mqttClient.connect(clientId.c_str(), user.c_str(), password.c_str())) {
      Serial.println("connected");
      /* subscribe topic */
      mqttClient.subscribe(SET_TOP);
    } else {
      Serial.print("failed, status code =");
      Serial.print(mqttClient.state());
      Serial.println("try again in 5 seconds");
      /* Wait 5 seconds before retrying */
      delay(5000);
    }
  }
}
#pragma endregion -- MQTT

#pragma region -- NTP Client

const char* ntpServer = "pool.ntp.org";
const int   gmtOffset_sec = 7200;
const int   daylightOffset_sec = 60000;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);


#pragma endregion ## NTP Client

// alles rund um den Webserver
#pragma region Webserver

// Set number of outputs
#define NUM_OUTPUTS  4

// Assign each GPIO to an output
int outputGPIOs[NUM_OUTPUTS] = {2, 4, 12, 14};

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Initialize WiFi
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
    //WebSerial.println("#-REC-RAW-START-#");
    //WebSerial.println(d);
    //WebSerial.println("#-REC-RAW-ENDE-#");
  }

  if((d.indexOf("::") > 0 ) && (d.length() > 3)){ 

    //WebSerial.println("#-MATCH-START-");
    //WebSerial.println(d.indexOf("::"));
    //WebSerial.println(d.length());
    //WebSerial.println("#-MATCH-END-");

  }
  else{
     //WebSerial.println("#-NOT-ALLOW-#");
  }

}

#pragma endregion Webserver

// Uhr Funktion
#pragma region -- Uhr

void MakePixels(){

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
 for(i=0;i<sizeof(oneSecond);i++)
 (s1 <=i?LED(oneSecond[i],AUS):LED(oneSecond[i],AN));

 for(i=0;i<sizeof(tenSecond);i++)
 (s10<=i?LED(tenSecond[i],AUS):LED(tenSecond[i],AN));

 for(i=0;i<sizeof(oneMinute);i++)
 (m1 <=i?LED(oneMinute[i],AUS):LED(oneMinute[i],AN));

 for(i=0;i<sizeof(tenMinute);i++)
 (m10<=i?LED(tenMinute[i],AUS):LED(tenMinute[i],AN));

 for(i=0;i<sizeof(oneHour);i++)
 (h1 <=i?LED(oneHour[i],AUS):LED(oneHour[i],AN)); 

 for(i=0;i<sizeof(tenHour);i++)
 (h10<=i?LED(tenHour[i],AUS):LED(tenHour[i],AN));

// Separator
 if(((s1==9) && (s10==5)) || ((m1==9) && (m10==5))){
  for(int i=0;i<sizeof(Separator);i++)LED(Separator[i],SEP);
 }
 else{
  for(int i=0;i<sizeof(Separator);i++)LED(Separator[i],AUS);
  }

// Restaurant
 if((h>=18) || (h<=6)){
  for(int i=0;i<sizeof(Restaurant);i++)LED(Restaurant[i],RES);
 }
 else{
  for(int i=0;i<sizeof(Restaurant);i++)LED(Restaurant[i],AUS);
  }

// Flugbefeuerung 0-59 modulo 6 [0,1,2]
// LED#s haben 2 Helligkeitsstufen dadurch kommt es zu einer Überbelndung von 1 > 2 > 3 > 1...
 if((s % 6) == 0){
  LED(Flugbefeuerung[0],FDM); 
  LED(Flugbefeuerung[1],AUS);  
  LED(Flugbefeuerung[2],FDM);
 }

 if((s % 6) == 1){
  LED(Flugbefeuerung[0],FON);
  LED(Flugbefeuerung[1],AUS);
  LED(Flugbefeuerung[2],AUS);
 }

 if((s % 6) == 2){
  LED(Flugbefeuerung[0],FDM);
  LED(Flugbefeuerung[1],FDM);
  LED(Flugbefeuerung[2],AUS);
 }

  if((s % 6) == 3){
  LED(Flugbefeuerung[0],AUS);
  LED(Flugbefeuerung[1],FON);
  LED(Flugbefeuerung[2],AUS);
 }

 if((s % 6) == 4){
  LED(Flugbefeuerung[0],AUS);
  LED(Flugbefeuerung[1],FDM);
  LED(Flugbefeuerung[2],FDM);
 }

 if((s % 6) == 5){
  LED(Flugbefeuerung[0],AUS);
  LED(Flugbefeuerung[1],AUS);
  LED(Flugbefeuerung[2],FON);
 }
 
// Spitze
LED(Flugbefeuerung[3],FON);

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
  //WebSerial.begin(&server);
  /* Attach Message Callback */
  //WebSerial.msgCallback(recvMsg);
  
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

  /* configure the MQTT server with IPaddress and port */
  mqttClient.setServer(MQTT_SERVER.c_str(), MQTT_PORT);

  /* this receivedCallback function will be invoked 
  when client received subscribed topic */
  mqttClient.setCallback(receivedCallback);

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
      //WebSerial.println("#-INFO-START-#");
      //WebSerial.print(F("IP address: "));
      //WebSerial.println(WiFi.localIP());
      //WebSerial.printf("Millis=%lu\n", millis());
      //WebSerial.printf("Free heap=[%u]\n", ESP.getFreeHeap());
      //WebSerial.print("Chip model: "); 
      //WebSerial.println(ChipModel);
      //WebSerial.print("Chip rev.: "); 
      //WebSerial.println(ChipRevision);
      //WebSerial.print("Product ID: "); 
      //WebSerial.println(ProductId);
      //WebSerial.println("#-INFO-ENDE-#");
    }

  }

// MQTT starts here

  /* if client was disconnected then try to reconnect again */
  if (!mqttClient.connected()) {
    mqttconnect();
  }
  /* this function will listen for incomming 
  subscribed topic-process-invoke receivedCallback */
  mqttClient.loop();
  /* we increase counter every 3 secs
  we count until 3 secs reached to avoid blocking program if using delay()*/
  long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;
    if (counter < 100) {
      counter++;
      snprintf (msg, 20, "%d", counter);
      /* publish the message */
      mqttClient.publish(CNT_TOP, msg);
    }else {
      counter = 0;  
    }
  }


// MQTT ENDS HERE


  delay(10);  // <- fixes some issues with WiFi stability

}