#include <WiFiS3.h>
#include "util.h"
#include "config.h"
#include "tasks/sensor_task.h"
#include "tasks/serial_task.h"
#include <Modulino.h>
#include "shared.h"

QueueHandle_t sensor_queue;

/* initialize serial and wait for the port to open */
void init_serial(uint32_t baudrate) {
    Serial.begin(baudrate);
    while (!Serial);
}

/* initialize FreeRTOS queues and tasks */
void init_freertos() {
    sensor_queue = xQueueCreate(SENSOR_QUEUE_SIZE, sizeof(SensorData));
    if (sensor_queue == NULL) {
        Serial.println("Queue creation failed!");
        while (1);
    }

    xTaskCreate(sensor_task, "SensorTask", 256, NULL, 1, NULL);
    xTaskCreate(serial_task, "SerialTask", 256, NULL, 1, NULL);

    vTaskStartScheduler();
}

/* setup the board */
void setup() {
    init_serial(BAUDRATE);
    Modulino.begin();
    init_freertos();
}

void loop() { }
