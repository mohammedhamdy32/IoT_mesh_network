#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt/mqtt_interface.h"
#include "Network/Network_inteface.h"

static const char *TAG = "MAIN";


void app_main(void) 
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize WiFi
    Netwok_app_start();
    
    int counter = 0;
    char message[64];
    // Initialize MQTT (this will create the MQTT task)
    mqtt_init();
    // Create a sample message
    snprintf(message, sizeof(message), "Sensor reading: %d", counter++);
            
    // Send the message
    mqtt_send_message("test/topic", message, 0, 0);
    
    ESP_LOGI(TAG, "Application started");
}