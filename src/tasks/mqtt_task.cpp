#include <Arduino.h>
#include <WiFiS3.h>
#include <MQTT.h>
#include "config.h"
#include "tasks/mqtt_task.h"
#include "shared.h"

/*
 * confidence score is computed using a sigmoid function centered at the
 * ANOMALY_THRESHOLD
 */
static inline float confidence_from_mse(float mse) {
    return 1.0f / (1.0f + expf(-(mse - ANOMALY_THRESHOLD)));
}

/* connect the client to the broker, retry if it fails */
static void ensure_connected(MQTTClient &client) {
    while (!client.connected()) {
        Serial.println("[mqtt] Connecting to broker...");
        if (client.connect("arduino-anomaly-detector")) {
            Serial.println("[mqtt] Connected");
        } else {
            Serial.println("[mqtt] Connection failed, retrying");
            vTaskDelay(pdMS_TO_TICKS(MQTT_RETRY_PERIOD));
        }
    }
}

/* build a compact json payload */
static int build_payload(
    char *buf,
    size_t buf_len,
    unsigned long timestamp_ms,
    float mse,
    float confidence
) {
    return snprintf(
        buf,
        buf_len,
        "{\"timestamp\":%lu,\"confidence\":%.4f}",
        timestamp_ms,
        confidence
    );
}

void mqtt_task(void *params) {
    WiFiClient wifi;
    MQTTClient client;

    client.begin(MQTT_ADDRESS, MQTT_PORT, wifi);
    ensure_connected(client);

    char payload[128];

    for (;;) {
        /* keep the mqtt connection alive */
        client.loop();

        float mse;
        /* wait for a message, if it doesn't arrive just skip the iteration */
        if (xQueueReceive(inference_queue, &mse, pdMS_TO_TICKS(100)) != pdTRUE) continue;

        /* check if the received mse is an anomaly */
        if (mse <= ANOMALY_THRESHOLD) continue;

        /* reconnect if the broker dropped */
        if (!client.connected()) ensure_connected(client);

        /* prepare the payload and send it */
        float confidence = confidence_from_mse(mse);
        uint32_t timestamp_ms = millis();
        int len = build_payload(payload, sizeof(payload), timestamp_ms, mse, confidence);

        if (len <= 0 || len >= (int)sizeof(payload)) {
            Serial.println("[mqtt] Payload formatting error, skipping.");
            continue;
        }

        if (client.publish(MQTT_TOPIC, payload)) {
            Serial.print("[mqtt] Published: ");
            Serial.println(payload);
        } else {
            Serial.println("[mqtt] Publish failed.");
        }
    }
}
