#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt/mqtt_interface.h"
#include "mqtt/mqtt_config.h"
#include "Network/Network_inteface.h"
#include "KWS/keyword_spotting_interface.h"

#include "uart/uart_interface.h"

static const char *TAG = "MAIN";

char message[64];
char uart_message[64];

extern uint8_t is_new_data;
extern uint8_t g_recvData[30];
extern uint8_t g_recvTopic[30];
extern uint8_t g_recvDataLen;
extern uint8_t g_recvTopicLen;

void app_main(void) 
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize WiFi
    Netwok_app_start();

    keyword_spotting_app_start();
    
    // Initialize MQTT (this will create the MQTT task)
    mqtt_init();
    
    /* Start UART task */
    uart_start_task();
    
    // Create a sample message
    // int counter = 0;
    // snprintf(message, sizeof(message), "Sensor reading: %d", counter++);        
    // Send the message
    mqtt_send_message( MQTT_DATA_TOPIC , message , 0 , 0 );
    mqtt_send_message( MQTT_ESP_CONTROL_TOPIC , "on" , 0 , 0 );
    
    ESP_LOGI(TAG, "Application started");

    // Example of sending data
    const char *msg = "Hello from node 1\r\n";
    int status = uart_send_data((const uint8_t *)msg, strlen(msg));
    if (status != -1) 
        ESP_LOGI(TAG, "Message sent correctly");
    else 
        ESP_LOGE(TAG, "Error sending message");
    

    uart_queue_msg_t rx_msg;
    while (1) 
    {
        if( xQueueReceive(uart_get_queue(), &rx_msg, pdMS_TO_TICKS(5000)) == pdPASS ) 
        {
            if (rx_msg.msg_type == UART_MSG_RECEIVED) 
            {
                // Null-terminate for printing as string (if appropriate)
                rx_msg.data[rx_msg.data_len] = '\0';
                ESP_LOGI(TAG, "Received message: %s", (char *)rx_msg.data);
                
                /* Send message to mqtt server */
                mqtt_send_message( MQTT_DATA_TOPIC , (char *)rx_msg.data , 0 , 0 );
            }
        }
        if( is_new_data == 1 )
        {
            is_new_data = 0;
            int status = uart_send_data( (uint8_t *)g_recvData , g_recvDataLen );
            if (status != -1) 
                ESP_LOGI(TAG, "Uart sent correctly");
            else 
                ESP_LOGE(TAG, "Uart sending message");   
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }    

}


