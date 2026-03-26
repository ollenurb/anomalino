#pragma once

/* WiFi secrets are filled by platform.io */
#ifndef SSID_SECRET
  #error "SSID not defined — set WIFI_SSID env variable before building"
#endif
#ifndef PASS_SECRET
  #error "PASSWORD not defined — set WIFI_PASSWORD env variable before building"
#endif

#define BAUDRATE 9600                       /* serial port baudrate */
#define WIFI_TIMEOUT_MS 15000               /* total timeout for wifi connection */

/* Tasks settings */
#define SENSOR_PERIOD_MS 50                 /* sampling period for the sensor */
#define SENSOR_CALIBRATION_SAMPLES  100     /* number of samples averaged to compute bias */
#define SENSOR_CALIBRATION_DELAY_MS 10      /* delay between calibration samples (ms) */
