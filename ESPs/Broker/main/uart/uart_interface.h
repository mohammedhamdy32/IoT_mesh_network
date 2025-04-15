/**
 * @file uart_interface.h
 * @brief UART interface functions
 */
#ifndef UART_INTERFACE_H
#define UART_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "uart_config.h"

/**
 * @brief Initialize UART driver
 *
 * @return ESP_OK on success, appropriate error code otherwise
 */
esp_err_t uart_init(void);

/**
 * @brief Start the UART task
 * 
 * @return ESP_OK on success, appropriate error code otherwise
 */
esp_err_t uart_start_task(void);

/**
 * @brief Send data through UART
 *
 * @param data Buffer containing data to send
 * @param len Length of data to send
 * @return Number of bytes sent, or -1 on error
 */
int uart_send_data(const uint8_t *data, size_t len);

/**
 * @brief Get the UART queue handler
 * 
 * @return QueueHandle_t UART queue handle
 */
QueueHandle_t uart_get_queue(void);

#endif /* UART_INTERFACE_H */