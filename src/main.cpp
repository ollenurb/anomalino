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

/* initialize wifi by connecting to it. hangs indefinitely if it cannot connect */
void init_wifi(const char* ssid, const char* pass, const uint32_t timeout_ms) {
    /* check for the WiFi module */
    if (WiFi.status() == WL_NO_MODULE) {
      Serial.println("FATAL: Unable to communicate with WiFi module");
      HANG_FOREVER;
    }

    std::string firmware_version = WiFi.firmwareVersion();
    if (firmware_version < WIFI_FIRMWARE_LATEST_VERSION) {
      Serial.println("WARN: Firmware needs upgrading");
    }

    // attempt to connect to WiFi network:
    Serial.print("Connecting to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

    // poll every 500ms after 15 seconds stop trying and hang indefinitely
    uint32_t start = millis();

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeout_ms) {
            Serial.println("FATAL: WiFi connection timed out");
            HANG_FOREVER;
        }
        Serial.print(".");
        delay(500);
    }
    Serial.println("Successfully connected");
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
    init_wifi(SSID_SECRET, PASS_SECRET, WIFI_TIMEOUT_MS);
    print_network_info();
    Modulino.begin();
    init_freertos();
}

void loop() { }
