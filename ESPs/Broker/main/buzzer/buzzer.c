#include "buzzer.h"
#include "buzzer_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"

void
buzzer_init (void)
{
  ledc_timer_config_t timer_conf = { .speed_mode = LEDC_LOW_SPEED_MODE,
                                     .duty_resolution = LEDC_TIMER_10_BIT,
                                     .timer_num = BUZZER_TIMER,
                                     .freq_hz = 1000,
                                     .clk_cfg = LEDC_AUTO_CLK };
  ledc_timer_config (&timer_conf);

  ledc_channel_config_t channel_conf = { .gpio_num = BUZZER_GPIO,
                                         .speed_mode = LEDC_LOW_SPEED_MODE,
                                         .channel = BUZZER_CHANNEL,
                                         .intr_type = LEDC_INTR_DISABLE,
                                         .timer_sel = BUZZER_TIMER,
                                         .duty = 0,
                                         .hpoint = 0 };

  ledc_channel_config (&channel_conf);
}

static void
buzzer_tone (uint32_t freq, uint32_t duration_ms)
{
  ledc_set_freq (LEDC_LOW_SPEED_MODE, BUZZER_TIMER, freq);

  ledc_set_duty (LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, BUZZER_DUTY);
  ledc_update_duty (LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);

  vTaskDelay (pdMS_TO_TICKS (duration_ms));

  ledc_set_duty (LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
  ledc_update_duty (LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);

  vTaskDelay (pdMS_TO_TICKS (30)); // Small pause between tones
}

void
buzzer_feedback_on (void)
{
  // 🎵 Vivaldi - Spring (Short adaptation)
  buzzer_tone (1318, 80); // E6

  buzzer_tone (1396, 80); // F#6

  buzzer_tone (1567, 100); // G6

  vTaskDelay (pdMS_TO_TICKS (40));

  buzzer_tone (1760, 120); // A6 (Joyful accent)
}

void
buzzer_feedback_off (void)
{
  // Beethoven - 5th Symphony (Dramatic knock)

  buzzer_tone (784, 100); // G5
  vTaskDelay (pdMS_TO_TICKS (50));

  buzzer_tone (784, 100); // G5
  vTaskDelay (pdMS_TO_TICKS (50));

  buzzer_tone (784, 100); // G5
  vTaskDelay (pdMS_TO_TICKS (70));

  buzzer_tone (622, 200); // D#5 (Drop for drama)
}
