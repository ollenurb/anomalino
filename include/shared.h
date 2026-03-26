#pragma once
#include <Arduino_FreeRTOS.h>
#define SENSOR_QUEUE_SIZE 10

typedef struct {
    float x ;
    float y;
    float z;
    float pitch;
    float roll;
    float yaw;
} SensorData;

/* Shared infrastructure between tasks */
extern QueueHandle_t sensor_queue;
