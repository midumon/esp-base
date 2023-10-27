/*** Projekt
 ###
 
 Projekt für ESP32C3 Board
   
 Sandwich Modell mit 46 Pixeln
 AdaFruit Stripe Standard

 ###
***/

 
/*** Includes
 ###
 
 Includes und Variablen
 
 ###
***/

#include <Arduino.h>
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

// my source
#include "pixel_46_s1.h"
//#include "pixel_46_s2.h"
//#include "pixel_46_s3.h"
//#include "pixel_46_s4.h"


// Nicht flüchtige Variablen als preferences
Preferences prefs;

// Firmware und Produktinfo
String d_Firmware = "V.0.0.4-base0101";
String c_Firmware;
String c_Product;
String c_ProductMac;
String c_ChipModel;
uint8_t c_ChipRevision;
String d_ProductName = "TestDev003";
String c_ProductName;

// ### Filesystem ###
// Information SPIFFS
unsigned int totalBytes;
unsigned int usedBytes;
// Ausgabe Dir
void printDirectory(File dir, int numTabs = 3);


// ### Wifi ###
String ssid = "A";
String pass = "B";
String d_softAPName = "rheinturm";
String c_softAPName;

// statische Netzwerk Adressen
IPAddress d_ip(192, 168, 0, 200);
IPAddress c_ip;
IPAddress d_dns(192, 168, 0, 2);
IPAddress c_dns;
IPAddress d_gateway(192, 168, 0, 1);
IPAddress c_gateway;
IPAddress d_mask(255, 255, 255, 0);
IPAddress c_mask;

// HTTP/S
String d_UserAgent = "MAF-RT";
String c_UserAgent;
// Create HTTPClient
WiFiClientSecure wifiClient;
HTTPClient httpClient;
    
// ### MDNS ###
// Variablen
String d_Hostname = "rheinturm";
String c_Hostname = "";

// ### Webserver ###
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// ### NTP ###
// Variablen
const char* ntpServer = "pool.ntp.org";
const long updateInterval = 3600000;  // 60 * 60 * 1000 == 1 Stunde
int32_t d_gmtOffset = 0;
int32_t c_gmtOffset = 0;
// Create NTP-Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, c_gmtOffset, updateInterval);

// ### JSON ###
char JSONBuffer[4096];
size_t JSON_Buffer_size;
DynamicJsonDocument myInfo(2048);
DynamicJsonDocument toKafka(2048);

// Initialize SPIFFS
void initSPIFFS() {
  
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }

  // Get all information of your SPIFFS
  totalBytes = SPIFFS.totalBytes();
  usedBytes = SPIFFS.usedBytes();
  
  Serial.println("File sistem info.");
 
  Serial.print("Total space:      ");
  Serial.print(totalBytes);
  Serial.println("byte");
 
  Serial.print("Total space used: ");
  Serial.print(usedBytes);
  Serial.println("byte");
 
  Serial.println();
 
  // Open dir folder
  File dir = SPIFFS.open("/");
  // Cycle all the content
  printDirectory(dir);
  
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

    httpClient.end(); //Free the resources

  }

}

// Send to Kafka

void putToKafka(){

  // https://kafka-bridge.rheinturm.cloud

  wifiClient.setInsecure();  // HTTPS ohne SSL Überprüfung              
  String myUrl = "https://bridge-kafka.rheinturm.cloud/topics/" + c_ProductMac;
  
  // JSON leeren
  toKafka.clear();
    
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status

    HTTPClient http;   
     
    httpClient.begin(wifiClient, myUrl);  
    httpClient.addHeader("Content-Type", "application/vnd.kafka.jsonv2+json");  
    //httpClient.addHeader("api_token", "iuergpeiugpieufperugpeiuepiueiughepiuh");

    JsonObject fdx = toKafka.to<JsonObject>();
    JsonObject DeviceInfo = fdx.createNestedObject("device");
    DeviceInfo["id"] = c_ProductMac;
    DeviceInfo["model"] = c_ChipModel;
    DeviceInfo["revision"] = c_ChipRevision;
    DeviceInfo["firmware"] = c_Firmware;
    DeviceInfo["SystemStarts"] = prefs.getULong64("SystemStarts");
    DeviceInfo["Name"] = c_ProductName;
    DeviceInfo["Useragent"] = c_UserAgent;
  
    String requestBody;
    serializeJson(toKafka, requestBody);
     
    int httpResponseCode = http.POST(requestBody);
 
    if(httpResponseCode>0){
       
      String response = http.getString();                       
       
      Serial.println(httpResponseCode);   
      Serial.println(response);
    }
    else {
      Serial.println("Error on HTTP request");   
    }  
  }
  else{
    
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

  Serial.printf("IPAddress to Int is %d \n", c_ip);
  Serial.print("String to IP Address is : ");
  Serial.println(c_ip);



  

  c_softAPName = prefs.getString("softAPName", d_softAPName + "-" + c_ProductMac);
  prefs.putString("softAPName", c_softAPName);

  c_UserAgent = prefs.getString("UserAgent", d_UserAgent + "-" + c_ProductMac);
  prefs.putString("softAPName", c_UserAgent);

  c_Product = prefs.getString("Product", c_ChipModel + "-" + c_ChipRevision + "-" + c_ProductMac);
  prefs.putString("Product", c_Product); 

  c_ProductName = prefs.getString("ProductName", d_ProductName);
  prefs.putString("ProductName", c_ProductName);    
  
  pinMode(UHR_PIN, OUTPUT);
  pixels.show();  // showtime - Init all pixels OFF

  // WiFi im Station Mode
  if(initWiFi()) {

    // DNS Name setzen
    if (!MDNS.begin(d_Hostname)) {
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
          if (p->name() == "hostname") {
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

  }
 
  // WiFi im AP Mode
  else {

    Serial.println("Setting AP Mode!!!");
    WiFi.mode(WIFI_AP);
    
    // NULL sets an open Access Point
    WiFi.softAP(c_softAPName.c_str(), NULL); // Soft AP ohne Passwort

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

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
          if (p->name() == "hostname") {
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
  
  delay(100);
}

// RUN
void loop() {
 
  s = timeClient.getSeconds();
  m = timeClient.getMinutes();
  h = timeClient.getHours();

  // Jobs jede Sekunde und bei Start
  if (so != s) {
    so = s;
    MakePixels();
  }

  // Jobs jede Minute und bei Start
  if (mo != m) {
    mo = m;
    //getMyInfo();
    //putToKafka();
  }

  // Jobs jede Stunde und bei Start
  if (ho != h) {
    ho = h;
    getMyInfo();
    putToKafka();
  }

  delay(20);
}

void printDirectory(File dir, int numTabs) {
  while (true) {
 
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}
