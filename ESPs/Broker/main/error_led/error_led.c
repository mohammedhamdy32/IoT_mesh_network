/**
 * Init Error LED
*/

#include "driver/gpio.h"
#include "error_led.h"

void error_led_init(void)
{
	gpio_set_direction( ERROR_LED_PIN , GPIO_MODE_OUTPUT );
	ESP_ERROR_CHECK( gpio_set_level( ERROR_LED_PIN , LOGIC_LOW  ) ) ; /* Turn LED off */
}

void error_led_on(void)
{
	gpio_set_level( ERROR_LED_PIN , LOGIC_HIGH  ); /* Turn LED on */

}

void error_led_off(void)
{
    gpio_set_level( ERROR_LED_PIN , LOGIC_LOW  ); /* Turn LED off */
}