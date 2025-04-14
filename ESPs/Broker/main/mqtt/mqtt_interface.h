#ifndef MQTT_INTERFACE_H
#define MQTT_INTERFACE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mqtt_client.h"

// MQTT Message Structure
typedef struct {
    char topic[64];
    char data[256];
    int qos;
    int retain;
} mqtt_message_t;

// Public API
void mqtt_init(void);  // Initialize MQTT system
esp_err_t mqtt_send_message(const char* topic, const char* data, int qos, int retain);  // Send a message via queue
esp_mqtt_client_handle_t mqtt_get_client(void);  // Get the client handle if needed

// WiFi initialization
void wifi_init(void);

#endif /* MQTT_INTERFACE_H */