/**
 * @file uart_program.c
 * @brief UART driver implementation
 */
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "uart_interface.h"
#include "uart_config.h"

static const char *TAG = "UART";
static QueueHandle_t uart_queue = NULL;
static TaskHandle_t uart_task_handle = NULL;

/**
 * @brief UART task function
 * 
 * @param pvParameters Task parameters
 */
static void uart_task(void *pvParameters) 
{
    uart_queue_msg_t rx_msg;
    uint8_t rx_buffer[UART_BUF_SIZE];
    
    while (1) 
    {
        // Check for received data
        int len = uart_read_bytes(UART_PORT_NUM, rx_buffer, UART_BUF_SIZE, UART_READ_TIMEOUT);
        if (len > 0) 
        {
            // Data received, create message and send to queue
            rx_msg.msg_type = UART_MSG_RECEIVED;
            memcpy(rx_msg.data, rx_buffer, len);
            rx_msg.data_len = len;
            
            if( xQueueSendFromISR(uart_queue, &rx_msg, pdMS_TO_TICKS(10)) != pdPASS ) 
            {
                ESP_LOGE(TAG, "Failed to send received data to queue");
            } else 
            {
                ESP_LOGI(TAG, "Received %d bytes, sent to queue", len);
            }
        }
        
        // Small delay to prevent CPU hogging
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t uart_init(void) 
{
    // Configure UART parameters
    uart_config_t uart_config = 
    {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    // Create queue for UART messages
    uart_queue = xQueueCreate(UART_QUEUE_SIZE, sizeof(uart_queue_msg_t));
    if (uart_queue == NULL) 
    {
        ESP_LOGE(TAG, "Failed to create UART queue");
        return ESP_FAIL;
    }

    // Install and configure UART driver
    esp_err_t ret = uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "UART driver install failed");
        vQueueDelete(uart_queue);
        uart_queue = NULL;
        return ret;
    }

    ret = uart_param_config(UART_PORT_NUM, &uart_config);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "UART parameter config failed");
        uart_driver_delete(UART_PORT_NUM);
        vQueueDelete(uart_queue);
        uart_queue = NULL;
        return ret;
    }

    ret = uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "UART set pin failed");
        uart_driver_delete(UART_PORT_NUM);
        vQueueDelete(uart_queue);
        uart_queue = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "UART initialized successfully");
    return ESP_OK;
}

esp_err_t uart_start_task(void) 
{
    // Initialize UART
    esp_err_t ret = uart_init();
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "UART initialization failed");
        return ESP_ERR_INVALID_STATE;
    }

    if (uart_queue == NULL) 
    {
        ESP_LOGE(TAG, "UART not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Create UART task
    BaseType_t task_created = xTaskCreatePinnedToCore(
        uart_task,
        "uart_task",
        UART_TASK_STACK_SIZE,
        NULL,
        UART_TASK_PRIORITY,
        &uart_task_handle,
        0
    );

    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "UART task started");
    return ESP_OK;
}

int uart_send_data(const uint8_t *data, size_t len) {
    if (data == NULL || len == 0) {
        ESP_LOGE(TAG, "Invalid send parameters");
        return -1;
    }

    int bytes_sent = uart_write_bytes(UART_PORT_NUM, (const char *)data, len);
    if (bytes_sent < 0) {
        ESP_LOGE(TAG, "Failed to send data");
        return -1;
    }

    // Create send message for queue (optional, to inform other components)
    uart_queue_msg_t tx_msg;
    tx_msg.msg_type = UART_MSG_SEND;
    memcpy(tx_msg.data, data, len);
    tx_msg.data_len = len;
    
    if (xQueueSend(uart_queue, &tx_msg, 0) != pdPASS) {
        ESP_LOGW(TAG, "Failed to send TX event to queue");
        // Continue anyway since data was sent successfully
    }

    ESP_LOGI(TAG, "Sent %d bytes", bytes_sent);
    return bytes_sent;
}

QueueHandle_t uart_get_queue(void) {
    return uart_queue;
}
