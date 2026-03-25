#include <Arduino.h>
#include "config.h"
#include "tasks/sensor_task.h"
#include "shared.h"
#include <Modulino.h>

void sensor_task(void *params) {
    ModulinoMovement movement;
    movement.begin();

    for (;;) {
        SensorData sample;
        /* get the sample from the modulino */
        movement.update();
        sample.x = movement.getX();
        sample.y = movement.getY();
        sample.z = movement.getZ();

        /* send value back to the other task */
        xQueueSend(sensor_queue, &sample, 0);
        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}
