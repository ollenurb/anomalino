#include <Arduino.h>
#include "config.h"
#include "tasks/sensor_task.h"
#include "shared.h"
#include <Modulino.h>


typedef struct SensorSample {
    float x;
    float y;
    float z;
    float roll;
    float pitch;
    float yaw;
} SensorSample;

/**
 * Collect CALIBRATION_SAMPLES readings at rest and average them to produce
 * per-axis bias offsets.  The IMU should be kept still during this phase.
 * On return, the six offset fields of `bias` are populated; all others are 0.
 */
static void calibrate_sensor(ModulinoMovement &movement, SensorSample &bias) {
    Serial.println("[calibration] Keep the sensor still...");

    double sum_x = 0, sum_y = 0, sum_z = 0;
    double sum_pitch = 0, sum_roll = 0, sum_yaw = 0;

    for (int i = 0; i < SENSOR_CALIBRATION_SAMPLES; i++) {
        movement.update();
        sum_x += movement.getX();
        sum_y += movement.getY();
        sum_z += movement.getZ();
        sum_pitch += movement.getPitch();
        sum_roll += movement.getRoll();
        sum_yaw += movement.getYaw();

        delay(SENSOR_CALIBRATION_DELAY_MS);
    }

    bias.x = sum_x / SENSOR_CALIBRATION_SAMPLES;
    bias.y = sum_y / SENSOR_CALIBRATION_SAMPLES;
    bias.z = sum_z / SENSOR_CALIBRATION_SAMPLES;
    bias.pitch = sum_pitch / SENSOR_CALIBRATION_SAMPLES;
    bias.roll = sum_roll / SENSOR_CALIBRATION_SAMPLES;
    bias.yaw = sum_yaw / SENSOR_CALIBRATION_SAMPLES;

    Serial.println("[calibration] Done.");
    Serial.print("[calibration] bias  x=");
    Serial.print(bias.x);
    Serial.print("  y=");
    Serial.print(bias.y);
    Serial.print("  z=");
    Serial.print(bias.z);
    Serial.print("  pitch=");
    Serial.print(bias.pitch);
    Serial.print("  roll=");
    Serial.print(bias.roll);
    Serial.print("  yaw=");
    Serial.println(bias.yaw);
}

void sensor_task(void *params) {
    /* setup the movement sensor and calibrate it */
    ModulinoMovement movement;
    SensorSample bias;
    movement.begin();
    calibrate_sensor(movement, bias);

    for (;;) {
        SensorSample sample;
        /* get the raw sample from the modulino */
        movement.update();
        sample.x = (movement.getX() - bias.x);
        sample.y = movement.getY() - bias.y;
        sample.z = movement.getZ() - bias.z;
        sample.pitch = movement.getPitch() - bias.pitch;
        sample.roll = movement.getRoll() - bias.roll;
        sample.yaw = movement.getYaw() - bias.yaw;
        /* compute acceleration magnitude and rotational magnitude */
        SensorData message;
        message.acc_magnitude = sqrtf(sample.x * sample.x + sample.y * sample.y + sample.z * sample.z);
        message.rot_magnitude = sqrtf(sample.pitch * sample.pitch + sample.roll * sample.roll + sample.yaw * sample.yaw);
        /* send message value to the inference task */
        xQueueSend(sensor_queue, &message, 0);
        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}
