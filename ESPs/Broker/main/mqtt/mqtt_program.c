#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_system.h"

#include "mqtt_config.h"
#include "mqtt_interface.h"

static const char *TAG = "MQTT_MODULE";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static QueueHandle_t mqtt_queue = NULL;
static TaskHandle_t mqtt_task_handle = NULL;

// MQTT event handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) 
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) 
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_publish(client, MQTT_DEFAULT_TOPIC, "Connected from ESP32-S3", 0, 1, 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:" );
            break;
    }
}

// Initialize MQTT Client
static esp_mqtt_client_handle_t mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri  = MQTT_BROKER_URI,
        .broker.address.port = MQTT_BROKER_PORT,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    
    return client;
}

// MQTT Task Function
static void mqtt_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Start MQTT task" );
    mqtt_message_t msg;
    
    // Wait for WiFi connection before starting MQTT
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    
    // Initialize MQTT client
    mqtt_client = mqtt_app_start();
    
    // Send an initial test message
    esp_mqtt_client_publish(mqtt_client, MQTT_QOS_0_TOPIC, "Send test message from ESP32-S3", 0, 0, 0);
    
    // Main task loop - process messages from queue
    while(1) 
    {
        if(xQueueReceive(mqtt_queue, &msg, portMAX_DELAY) == pdTRUE) 
        {
            ESP_LOGI(TAG, "Sending message to topic %s: %s", msg.topic, msg.data);
            uint8_t status = esp_mqtt_client_publish(mqtt_client, msg.topic, msg.data, strlen(msg.data), msg.qos, msg.retain);
            if(status == -1)
                ESP_LOGE(TAG, "Sending message to topic %s: %s Fails", msg.topic, msg.data);
        }
    }
}

// Initialize MQTT system
void mqtt_init(void)
{
    // Create message queue
    mqtt_queue = xQueueCreate(MQTT_QUEUE_SIZE, sizeof(mqtt_message_t));
    
    if (mqtt_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT queue");
        return;
    }
    
    // Create MQTT task
    xTaskCreate(mqtt_task, "mqtt_task", MQTT_TASK_STACK_SIZE, NULL, MQTT_TASK_PRIORITY, &mqtt_task_handle);
    
    ESP_LOGI(TAG, "MQTT system initialized");
}

// Send MQTT message via queue
esp_err_t mqtt_send_message(const char* topic, const char* data, int qos, int retain)
{
    if (mqtt_queue == NULL) {
        ESP_LOGE(TAG, "MQTT queue not initialized");
        return ESP_FAIL;
    }
    
    mqtt_message_t msg;
    
    // Copy data to message struct
    strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
    msg.topic[sizeof(msg.topic) - 1] = '\0';
    
    strncpy(msg.data, data, sizeof(msg.data) - 1);
    msg.data[sizeof(msg.data) - 1] = '\0';
    
    msg.qos = qos;
    msg.retain = retain;
    
    // Send to queue
    if (xQueueSend(mqtt_queue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to queue MQTT message");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

// Get the MQTT client handle
esp_mqtt_client_handle_t mqtt_get_client(void)
{
    return mqtt_client;
}