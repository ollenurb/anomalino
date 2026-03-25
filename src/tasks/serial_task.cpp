#include <Arduino.h>
#include "tasks/serial_task.h"
#include "shared.h"

void serial_task(void *params) {
    SensorData data;

    for (;;) {
        /* block until we receive something from the other task */
        if (xQueueReceive(sensor_queue, &data, portMAX_DELAY) == pdTRUE) {
            Serial.print("[");
            Serial.print(data.x);
            Serial.print(", ");
            Serial.print(data.y);
            Serial.print(", ");
            Serial.print(data.z);
            Serial.println("]");
        }
    }
}
