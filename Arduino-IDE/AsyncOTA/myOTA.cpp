#include "myOTA.h"

MyOTAClass::MyOTAClass(){}

void MyOTAClass::begin(OTA_WEBSERVER *server, const char * username, const char * password) {
  _server = server;

  setAuth(username, password);

  _server->on("/update", HTTP_GET, [&](AsyncWebServerRequest *request){
  if(_authenticate && !request->authenticate(_username, _password)){
    return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", ELEGANT_HTML, sizeof(ELEGANT_HTML));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });


  // Logo Route
    _server->on("/ota/logo/light", HTTP_GET, [&](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse_P(200, ELEGANTOTA_LIGHT_LOGO_MIME, ELEGANTOTA_LIGHT_LOGO, sizeof(ELEGANTOTA_LIGHT_LOGO));
      #if ELEGANTOTA_LOGO_GZIPPED == 1
        response->addHeader("Content-Encoding","gzip");
      #endif
      response->addHeader("Cache-Control","public, max-age=900");
      request->send(response);
    });

    _server->on("/ota/logo/dark", HTTP_GET, [&](AsyncWebServerRequest *request){
      AsyncWebServerResponse *response = request->beginResponse_P(200, ELEGANTOTA_DARK_LOGO_MIME, ELEGANTOTA_DARK_LOGO, sizeof(ELEGANTOTA_DARK_LOGO));
      #if ELEGANTOTA_LOGO_GZIPPED == 1
        response->addHeader("Content-Encoding","gzip");
      #endif
      response->addHeader("Cache-Control","public, max-age=900");
      request->send(response);
    });


  // Metadata
    _server->on("/ota/metadata", HTTP_GET, [&](AsyncWebServerRequest *request){
      // Create JSON object using printf-like formatting
      // the object should contain the following keys:
      // tt - title
      // hid - hardware ID
      // fwv - firmware version
      // fwm - firmware mode (false = disabled, true = enabled)
      // fsm - filesystem mode (false = disabled, true = enabled)
      // lw - Logo width
      // lh - Logo height
      char json[512];
      snprintf(json, sizeof(json), "{\"tt\":\"%s\",\"hid\":\"%s\",\"fwv\":\"%s\",\"fwm\":%s,\"fsm\":%s,\"lw\":%d,\"lh\":%d}",
        _title,
        _id,
        _fw_version,
        _fw_enabled ? "true" : "false",
        _fs_enabled ? "true" : "false",
        ELEGANTOTA_LOGO_WIDTH,
        ELEGANTOTA_LOGO_HEIGHT
      );
      request->send(200, "application/json", json);
    });


    _server->on("/ota/start", HTTP_GET, [&](AsyncWebServerRequest *request) {
      if (_authenticate && !request->authenticate(_username, _password)) {
        return request->requestAuthentication();
      }

      // Get header x-ota-mode value, if present
      OTA_Mode mode = OTA_MODE_FIRMWARE;
      // Get mode from arg
      if (request->hasParam("mode")) {
        String argValue = request->getParam("mode")->value();
        if (argValue == "fs") {
          OTA_DEBUG_MSG("OTA Mode: Filesystem\n");
          mode = OTA_MODE_FILESYSTEM;
        } else {
          OTA_DEBUG_MSG("OTA Mode: Firmware\n");
          mode = OTA_MODE_FIRMWARE;
        }
      }

      // Check firmware mode
      if (mode == OTA_MODE_FIRMWARE && !_fw_enabled) {
        OTA_DEBUG_MSG("OTA Mode: Firmware not enabled\n");
        return request->send(400, "text/plain", "Firmware updates are disabled");
      }

      // Check filesystem mode
      if (mode == OTA_MODE_FILESYSTEM && !_fs_enabled) {
        OTA_DEBUG_MSG("OTA Mode: Filesystem not enabled\n");
        return request->send(400, "text/plain", "Filesystem updates are disabled");
      }

      // Get file MD5 hash from arg
      if (request->hasParam("hash")) {
        String hash = request->getParam("hash")->value();
        OTA_DEBUG_MSG(String("MD5: "+hash+"\n").c_str());
        if (!Update.setMD5(hash.c_str())) {
          OTA_DEBUG_MSG("ERROR: MD5 hash not valid\n");
          return request->send(400, "text/plain", "MD5 parameter invalid");
        }
      }

      #if UPDATE_DEBUG == 1
        // Serial output must be active to see the callback serial prints
        Serial.setDebugOutput(true);
      #endif

      // Pre-OTA update callback
      if (preUpdateCallback != NULL) preUpdateCallback();

      // Start update process
        
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, mode == OTA_MODE_FILESYSTEM ? U_SPIFFS : U_FLASH)) {
          OTA_DEBUG_MSG("Failed to start update process\n");
          // Save error to string
          StreamString str;
          Update.printError(str);
          _update_error_str = str.c_str();
          _update_error_str += "\n";
          OTA_DEBUG_MSG(_update_error_str.c_str());
        }        

      return request->send((Update.hasError()) ? 400 : 200, "text/plain", (Update.hasError()) ? _update_error_str.c_str() : "OK");
    });


    _server->on("/ota/upload", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if(_authenticate && !request->authenticate(_username, _password)){
          return request->requestAuthentication();
        }
        // Post-OTA update callback
        if (postUpdateCallback != NULL) postUpdateCallback(!Update.hasError());
        AsyncWebServerResponse *response = request->beginResponse((Update.hasError()) ? 400 : 200, "text/plain", (Update.hasError()) ? _update_error_str.c_str() : "OK");
        response->addHeader("Connection", "close");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        // Set reboot flag
        if (!Update.hasError()) {
          if (_auto_reboot) {
            _reboot_request_millis = millis();
            _reboot = true;
          }
        }
    }, [&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        //Upload handler chunks in data
        if(_authenticate){
            if(!request->authenticate(_username, _password)){
                return request->requestAuthentication();
            }
        }

        if (!index) {
          // Reset progress size on first frame
          _current_progress_size = 0;
        }

        // Write chunked data to the free sketch space
        if(len){
            if (Update.write(data, len) != len) {
                return request->send(400, "text/plain", "Failed to write chunked data to free space");
            }
            _current_progress_size += len;
            // Progress update callback
            if (progressUpdateCallback != NULL) progressUpdateCallback(_current_progress_size, request->contentLength());
        }
            
        if (final) { // if the final flag is set then this is the last frame of data
            if (!Update.end(true)) { //true to set the size to the current progress
                // Save error to string
                StreamString str;
                Update.printError(str);
                _update_error_str = str.c_str();
                _update_error_str += "\n";
                OTA_DEBUG_MSG(_update_error_str.c_str());
            }
        }else{
            return;
        }
    });

}

void MyOTAClass::setTitle(const char * title){
  strlcpy(_title, title, sizeof(_title));
}

void MyOTAClass::setID(const char * id){
  strlcpy(_id, id, sizeof(_id));
}

void MyOTAClass::setFWVersion(const char * version){
  strlcpy(_fw_version, version, sizeof(_fw_version));
}

void MyOTAClass::setAuth(const char * username, const char * password){
  if (strlen(username) > 0 && strlen(password) > 0) {
    strlcpy(_username, username, sizeof(_username));
    strlcpy(_password, password, sizeof(_password));
    _authenticate = true;
  }
}

void MyOTAClass::clearAuth(){
  _authenticate = false;
}

void MyOTAClass::setFirmwareMode(bool enable){
  _fw_enabled = enable;
}

bool MyOTAClass::checkFirmwareMode(){
  return _fw_enabled;
}

void MyOTAClass::setFilesystemMode(bool enable){
  _fs_enabled = enable;
}

bool MyOTAClass::checkFilesystemMode(){
  return _fs_enabled;
}

void MyOTAClass::setAutoReboot(bool enable){
  _auto_reboot = enable;
}

void MyOTAClass::loop() {
  // Check if 2 seconds have passed since _reboot_request_millis was set
  if (_reboot && millis() - _reboot_request_millis > 2000) {
    OTA_DEBUG_MSG("Rebooting...\n");
    )
    ESP.restart();
    _reboot = false;
  }
}

void MyOTAClass::onStart(std::function<void()> callable){
    preUpdateCallback = callable;
}

void MyOTAClass::onProgress(std::function<void(size_t current, size_t final)> callable){
    progressUpdateCallback= callable;
}

void MyOTAClass::onEnd(std::function<void(bool success)> callable){
    postUpdateCallback = callable;
}


MyOTAClass MyOTA;
