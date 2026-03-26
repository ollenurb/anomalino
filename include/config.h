#pragma once

#define BAUDRATE 9600                       /* serial port baudrate */

/* Tasks settings */
#define SENSOR_PERIOD_MS 10                 /* sampling period for the sensor */
#define SENSOR_CALIBRATION_SAMPLES  100     /* number of samples averaged to compute bias */
#define SENSOR_CALIBRATION_DELAY_MS 10      /* delay between calibration samples (ms) */
