/**
 * @file uart_config.h
 * @brief UART configuration parameters
 */
#ifndef UART_CONFIG_H
#define UART_CONFIG_H

#include "driver/uart.h"
#include "driver/gpio.h"

/**
 * @brief UART configuration parameters
 */
#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     115200
#define UART_TX_PIN        GPIO_NUM_17
#define UART_RX_PIN        GPIO_NUM_18
#define UART_BUF_SIZE      (1024)

/**
 * @brief UART task configuration
 */
#define UART_TASK_STACK_SIZE   1024*10
#define UART_TASK_PRIORITY     5
#define UART_QUEUE_SIZE        10
#define UART_READ_TIMEOUT      (pdMS_TO_TICKS(1000))

/**
 * @brief Message types for UART queue
 */
typedef enum {
    UART_MSG_SEND,
    UART_MSG_RECEIVED
} uart_msg_type_t;

/**
 * @brief UART message structure for queue
 */
typedef struct {
    uart_msg_type_t msg_type;
    uint8_t data[UART_BUF_SIZE];
    size_t data_len;
} uart_queue_msg_t;

#endif /* UART_CONFIG_H */