void getWHOAMI(){

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

    String payload = httpClient.getString();
    Serial.println(httpCode);
    Serial.println(payload);

     
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
