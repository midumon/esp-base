

#include "Arduino.h"
#include "stdlib_noniso.h"
#include "elop.h"
#include "logo.h"

#ifndef OTA_DEBUG
  #define OTA_DEBUG 0
#endif

#ifndef UPDATE_DEBUG
  #define UPDATE_DEBUG 0
#endif

#if OTA_DEBUG
  #define OTA_DEBUG_MSG(x) Serial.printf("%s %s", "[myOTA] ", x)
#else
  #define OTA_DEBUG_MSG(x)
#endif


#include <functional>
#include "FS.h"
#include "Update.h"
#include "StreamString.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#define OTA_WEBSERVER AsyncWebServer
#define HARDWARE "ESP32"

enum OTA_Mode {
     OTA_MODE_FIRMWARE = 0,
     OTA_MODE_FILESYSTEM = 1
};

class MyOTAClass{
  public:
    MyOTAClass();

    void begin(OTA_WEBSERVER *server, const char * username = "", const char * password = "");

    // Set portal title
    void setTitle(const char * title);

    // Set Hardware ID
    void setID(const char * id);

    // Set Firmware Version
    void setFWVersion(const char * version);

    // Authentication
    void setAuth(const char * username, const char * password);
    void clearAuth();

    // OTA Modes
    void setFirmwareMode(bool enable);
    bool checkFirmwareMode();

    void setFilesystemMode(bool enable);
    bool checkFilesystemMode();

    void setAutoReboot(bool enable);
    void loop();

    void onStart(std::function<void()> callable);
    void onProgress(std::function<void(size_t current, size_t final)> callable);
    void onEnd(std::function<void(bool success)> callable);
    
  private:
    OTA_WEBSERVER *_server;

    bool _authenticate;
    char _title[64];
    char _id[64];
    char _fw_version[64];
    char _username[64];
    char _password[64];
    bool _fw_enabled = true;
    bool _fs_enabled = true;

    bool _auto_reboot = true;
    bool _reboot = false;
    unsigned long _reboot_request_millis = 0;

    String _update_error_str = "";
    unsigned long _current_progress_size;

    std::function<void()> preUpdateCallback = NULL;
    std::function<void(size_t current, size_t final)> progressUpdateCallback = NULL;
    std::function<void(bool success)> postUpdateCallback = NULL;
};

extern MyOTAClass MyOTA;
