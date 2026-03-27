#pragma once
#include <Arduino_FreeRTOS.h>

#define SENSOR_QUEUE_SIZE 20
#define INFERENCE_QUEUE_SIZE 4

typedef struct {
    float acc_magnitude;
    float rot_magnitude;
} SensorData;

/* Shared infrastructure between tasks */
extern QueueHandle_t sensor_queue;
extern QueueHandle_t inference_queue;
