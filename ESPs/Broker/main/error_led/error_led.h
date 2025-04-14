/*
 * error_led.h
 *
 *  Created on: Nov 28, 2023
 *  Author: mohammedhamdy32
 */

#ifndef ERROR_LED_H_
#define ERROR_LED_H_


/* Error Led Info */
#define ERROR_LED_PIN    GPIO_NUM_2
#define LOGIC_HIGH          1
#define LOGIC_LOW           0


/* Functions prototypes */
void error_led_init(void);
void error_led_on(void);
void error_led_off(void);

#endif /* ERROR_LED_H_ */
