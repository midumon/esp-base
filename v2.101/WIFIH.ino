// Initialize WiFi


// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

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
