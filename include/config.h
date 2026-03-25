#pragma once

/* Serial port section */
#define BAUDRATE 9600
/* WiFi section */
#define WIFI_TIMEOUT_MS 15000

#ifndef SSID_SECRET
  #error "SSID not defined — set WIFI_SSID env variable before building"
#endif
#ifndef PASS_SECRET
  #error "PASSWORD not defined — set WIFI_PASSWORD env variable before building"
#endif

/* Tasks settings */
#define SENSOR_PERIOD_MS 250
