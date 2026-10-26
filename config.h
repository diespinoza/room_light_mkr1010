// config.h — hardware and behaviour settings for the room light.
// Everything you might need to tweak lives here.
#pragma once

#include <Adafruit_NeoPixel.h>

// ---- Strip ---------------------------------------------------------------
// Adafruit 4689: NeoPixel RGBW Neon-like Flex Strip, 60 LEDs/m, 5 V, 5500K.
#define LED_PIN            6                       // MKR digital pin → strip DATA
#define NUM_PIXELS         60                      // one 1 m strip
#define PIXEL_TYPE         (NEO_GRBW + NEO_KHZ800) // RGBW strips are GRBW order

// ---- Power / current limiter ---------------------------------------------
// The limiter estimates the strip's draw from the pixel values about to be
// shown and scales the whole frame down if it would exceed CURRENT_CAP_MA.
// Keep the cap below the supply rating minus what the MKR itself needs.
#define CURRENT_CAP_MA     350    // bench with the 1N4007 diode: strip rail sags below 4 V above this
                                  // and the white/blue dies shimmer. 3500 with a 4 A supply + 74AHCT125
#define MA_PER_DIE         20     // each R, G, B or W die at full on (SK6812 class)
#define MA_IDLE_PER_PIXEL  1      // quiescent draw of each pixel's controller

// ---- Defaults --------------------------------------------------------------
#define DEFAULT_BRIGHTNESS 128    // 0..255 master brightness at boot
#define DEFAULT_FADE_MS    400    // colour change fade time
#define FRAME_INTERVAL_MS  20     // ~50 fps for fades/effects
#define IDLE_REFRESH_MS    1000   // resend the frame this often when nothing
                                  // changes, so a glitched frame heals itself

// ---- Serial ---------------------------------------------------------------
#define SERIAL_BAUD        115200

// ---- Network ------------------------------------------------------------
#define WIFI_HOSTNAME      "roomlight"  // shows in the router's client list
#define WIFI_RETRY_MS      10000        // wait between connection attempts
#define HTTP_PORT          80
#define HTTP_CLIENT_TIMEOUT_MS 300      // max time spent reading one request

// ---- Phase 3 ---------------------------------------------------------------
#define USE_MDNS           1            // 0 if the ArduinoMDNS library is not installed
#define MDNS_SERVICE_NAME  "Room light._http"
#define SAVE_DELAY_MS      3000         // write state to flash this long after the last change
#define DEFAULT_SPEED      50           // effect speed 1..100

// ---- Status LED (RGB LED on the NINA WiFi module) --------------------------
// The MKR's own orange LED is on pin 6, shared with the strip data, so it is
// not usable. The WiFi module's RGB LED is driven over SPI instead.
#define STATUS_LED_LEVEL   24           // 0..255 brightness of the status colours
#define STATUS_LED_OK_MS   3000         // green this long after connecting, then off

// ---- Reliability -----------------------------------------------------------
#define WIFI_CONNECT_TIMEOUT_MS 15000   // give up on one attempt after this
#define WIFI_STATUS_POLL_MS     1000    // how often to ask the module "still connected?"
#define WIFI_MODULE_RESET_AFTER 3       // consecutive failed attempts before resetting the NINA
#define WATCHDOG_ENABLED        1       // hardware reset if the loop stalls > ~16 s
