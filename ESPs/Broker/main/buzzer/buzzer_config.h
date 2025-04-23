#ifndef BUZZER_CONFIG_H
#define BUZZER_CONFIG_H

#include "driver/ledc.h"

#define BUZZER_GPIO 18

#define BUZZER_DUTY 512
#define BUZZER_CHANNEL LEDC_CHANNEL_0
#define BUZZER_TIMER LEDC_TIMER_0

#endif