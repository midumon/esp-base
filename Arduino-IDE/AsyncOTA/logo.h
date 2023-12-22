#ifndef elegantota_logo_h
#define elegantota_logo_h

#include <Arduino.h>

#define ELEGANTOTA_LOGO_GZIPPED      1          // If the logo is gzipped compressed | 1 = true and 0 = false
#define ELEGANTOTA_LOGO_WIDTH        220        // Size in px
#define ELEGANTOTA_LOGO_HEIGHT       60         // Size in px

// Light Logo ( displayed when user is on the light color scheme )
#define ELEGANTOTA_LIGHT_LOGO_MIME         "image/svg+xml"    // MIME type of the image you are using
extern const uint8_t ELEGANTOTA_LIGHT_LOGO[1281];

// Dark Logo
#define ELEGANTOTA_DARK_LOGO_MIME         "image/svg+xml"    // MIME type of the image you are using
extern const uint8_t ELEGANTOTA_DARK_LOGO[1271];



#endif