/*** Projekt
 ###
 
 Projekt für 
 
 Chip: ESP32C3 
 Board: XIAO ESP32C3
   
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
//#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

// test mit verteilten Dateien
#include "GlobalVars.h"
#include "Pixels.h"
#include "Helper.h"

/* Nicht flüchtige Variablen als prefs */
Preferences prefs;

// Create HTTPClient
WiFiClientSecure wifiClient;
HTTPClient httpClient;
    
// ### Webserver ###
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// ### NTP ###
// Create NTP-Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, c_gmtOffset, updateInterval);

// ### JSON ###
char JSONBuffer[2048];
size_t JSON_Buffer_size;
DynamicJsonDocument myInfo(2048);
DynamicJsonDocument toKafka(2048);

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

  // https://kafka-bridge.rheinturm.cloud

  //String myUrl = "https://bridge-kafka.rheinturm.cloud/topics/" + c_ProductMac;
  String myUrl = "https://bridge-kafka.rheinturm.cloud/topics/FDX";
  // JSON leeren
  toKafka.clear();
    
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status

    wifiClient.setInsecure();  // HTTPS ohne SSL Überprüfung  
     
    httpClient.begin(wifiClient, myUrl);  
    httpClient.addHeader("Content-Type", "application/vnd.kafka.json.v2+json");  
    //httpClient.addHeader("api_token", "iuergpeiugpieufperugpeiuepiueiughepiuh");

    JsonObject fdx = toKafka.to<JsonObject>();
    
    JsonArray records = fdx.createNestedArray("records");  
    JsonObject nestRecords = records.createNestedObject();
      nestRecords["key"] = c_ProductMac;
      JsonObject nestValue = nestRecords.createNestedObject("value"); 
        JsonObject nestDevice = nestValue.createNestedObject("device");
        JsonObject nestLocation = nestValue.createNestedObject("location");
          JsonObject nestCustomLocation = nestLocation.createNestedObject("custom");
          JsonObject nestAutoLocation = nestLocation.createNestedObject("ipapi");

    nestDevice["id"] = c_ProductMac;
    nestDevice["model"] = c_ChipModel;
    nestDevice["revision"] = c_ChipRevision;
    nestDevice["firmware"] = c_Firmware;
    nestDevice["SystemStarts"] = prefs.getULong64("SystemStarts");
    nestDevice["Name"] = c_ProductName;
    nestDevice["Useragent"] = c_UserAgent;
    nestDevice["HostIP"] = WiFi.localIP().toString();
    nestDevice["HostName"] = c_Hostname;
    nestDevice["SPIFFStotalBytes"] = SPIFFS.totalBytes();
    nestDevice["SPIFFSusedBytes"] = SPIFFS.usedBytes();

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
  
  BeginPixels();
  ShowPixels();

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
}