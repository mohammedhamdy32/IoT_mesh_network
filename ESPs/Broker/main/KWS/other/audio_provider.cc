/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "audio_provider.h"

#include <cstdlib>
#include <cstring>

// FreeRTOS.h must be included before some of the following dependencies.
// Solves b/150260343.
// clang-format off
#include "freertos/FreeRTOS.h"
// clang-format on

#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "ringbuf.h"
#include "micro_model_settings.h"
#include <soc/i2s_reg.h>

using namespace std;

// for c2 and c3, I2S support was added from IDF v4.4 onwards
#define NO_I2S_SUPPORT CONFIG_IDF_TARGET_ESP32C2 || (CONFIG_IDF_TARGET_ESP32C3  && (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 4, 0)))

static const char* TAG = "TF_LITE_AUDIO_PROVIDER";

/* Capture audio task handler */
TaskHandle_t g_capture_audio_task_handler;

/* ringbuffer to hold the incoming audio data */
ringbuf_t* g_KWS_audio_capture_buffer;

volatile int32_t g_latest_audio_timestamp = 0;
/* model requires 20ms new data from g_audio_capture_buffer and 10ms old data
 * each time , storing old data in the histrory buffer , {
 * history_samples_to_keep = 10 * 16 } */
constexpr int32_t history_samples_to_keep =
    ((kFeatureDurationMs - kFeatureStrideMs) *
     (kAudioSampleFrequency / 1000));
/* new samples to get each time from ringbuffer, { new_samples_to_get =  20 * 16
 * } */
constexpr int32_t new_samples_to_get =
    (kFeatureStrideMs * (kAudioSampleFrequency / 1000));

const int32_t kAudioCaptureBufferSizeKWS = 40000;
const int32_t kAudioCaptureBufferSizeVS  = 5000;
const int32_t i2s_bytes_to_read = 3200 ;

namespace {
int16_t g_audio_output_buffer_KWS[kMaxAudioSampleSize];
bool g_is_audio_initialized = false;
int16_t g_history_buffer[history_samples_to_keep];

uint8_t g_i2s_read_buffer32[i2s_bytes_to_read]  ;
uint8_t g_i2s_read_buffer16[i2s_bytes_to_read/2];

}  // namespace


#if NO_I2S_SUPPORT
  // nothing to be done here
#else
static void i2s_init(void) 
{
  /* Set I2S configuration */
  i2s_config_t i2s_config = {

      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX) , /* use i2s master, mean esp will genrate the clock, and RX mode to recive the audio data */
      .sample_rate = kAudioSampleFrequency,  /* Sampling rate */ 
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, /* sph0645 should use 32bit sample, but inmp441 can use 16bit or 32bit */
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  /* use only left channel */
      .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_I2S , /* Use I2S Philips standard  */
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, /* configare intrrupt flage to level 1, means lower priority */
      /* What is dma_buf_len and dma_buf_count : https://youtu.be/ejyt-kWmys8?si=HaH6Jtu8VxTwzFjP */
      .dma_buf_count = MICROPHONE_DMA_BUFFER_COUNT , /* Number of DMA buffers */
      .dma_buf_len = MICROPHONE_DMA_BUFFER_SIZE , /* The length of each DMA buffer, when buffer become bigger, the number of intrrupts become smaller per second (MAx:1024Byte) */
      .use_apll = false,    
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0,
  };

  
  
#if CONFIG_IDF_TARGET_ESP32S3
  i2s_pin_config_t pin_config = {
      .bck_io_num = MICROPHONE_I2S_CLK_PIN,    // IIS_SCLK
      .ws_io_num = MICROPHONE_I2S_WS_PIN,     // IIS_LCLK
      .data_out_num = I2S_PIN_NO_CHANGE,  // IIS_DSIN
      .data_in_num = MICROPHONE_I2S_DOUT_PIN,   // IIS_DOUT
  };
  i2s_config.bits_per_sample = (i2s_bits_per_sample_t) 32;
#else
  /* Configare I2S pins */
  i2s_pin_config_t pin_config = {
      .bck_io_num =   MICROPHONE_I2S_CLK_PIN,  /* I2S - LRCLK - left right clock */
      .ws_io_num  =   MICROPHONE_I2S_WS_PIN,   /* I2S - LRCLK - left right clock */
      .data_out_num = I2S_PIN_NO_CHANGE ,      /* Will not be used */
      .data_in_num =  MICROPHONE_I2S_DOUT_PIN, /* I2S - Serial data */
  };
#endif

  /* Install i2S configration */
  ESP_ERROR_CHECK( i2s_driver_install( I2S_NUM , &i2s_config, 0, NULL) );

  /* sph0645 uses a nonstandard I2S interface, so we should enable Philips mode to work  */
  /* If you use inmp441, don't write this lines */ 
  /* For esp32 */
  // REG_SET_BIT(I2S_TIMING_REG(I2S_NUM), BIT(1));
  // REG_SET_BIT( I2S_CONF_REG(I2S_NUM)  , I2S_RX_MSB_SHIFT );
  /* For esp32-s3 */
  REG_SET_BIT(I2S_TX_TIMING_REG(I2S_NUM), BIT(1));
  REG_SET_BIT( I2S_RX_CONF_REG(I2S_NUM)  , I2S_RX_MSB_SHIFT );


  // REG_SET_BIT(I2S_TIMING_REG(I2S_NUM), BIT(9));

   /* Set I2S pins */
  ESP_ERROR_CHECK( i2s_set_pin( I2S_NUM , &pin_config) );
  
  /* Clear DMA buffer and fill them with zeros */
  ESP_ERROR_CHECK( i2s_zero_dma_buffer( I2S_NUM ) );

#endif
}


static void CaptureSamples(void* arg) 
{
 
#if NO_I2S_SUPPORT
  ESP_LOGE(TAG, "i2s support not available on C3 chip for IDF < 4.4.0");
#else
  size_t bytes_read = i2s_bytes_to_read;
  i2s_init();

  while (1) 
  {
    i2s_read( I2S_NUM, (void *)g_i2s_read_buffer32, i2s_bytes_to_read , &bytes_read, pdMS_TO_TICKS(100));

    if (bytes_read <= 0) /* Means it didn't read any audio data */
    {
      ESP_LOGE(TAG, "Error in I2S read : %d", bytes_read);
    } else /* It reads some audio data only */
    {
      if (bytes_read < i2s_bytes_to_read) /* It reads only a some data, not all the data */ 
      {
        ESP_LOGW(TAG, "Partial I2S read");
      }


      /* Rescale the 32bit data to 16bit */
      for (int i = 0; i < bytes_read / 4; ++i)
      {
        ( (int16_t *) g_i2s_read_buffer16 )[i] = ( ((int32_t *) g_i2s_read_buffer32)[i] >> 15 )&(0xFFFF) ;
      }
                                                                                            /*
                                                                                             32bit samples : 4045734399,4046553599,4026073599,4077781503

                                                                                             10 : is high volume, but not good, At high volume, it overlap
                                                                                             11 : is like 10, but less overlap     
                                                                                             12 : is better with less overlap, and medium voice volume  
                                                                                             13 : is very bad
                                                                                             14 : is good but low voice 
                                                                                             15 : idea without any lose
                                                                                             */
     
      bytes_read = bytes_read / 2; /* Divide the size of read bytes by 2, because we use 32bit sample */

      /* Write bytes read by i2s into a KWS ring buffer */
      int kws_bytes_written = rb_write(g_KWS_audio_capture_buffer, (uint8_t*)g_i2s_read_buffer16, bytes_read, pdMS_TO_TICKS(100));
      

      
      /* Check if the bytes written correctly or not for KWS */
      if( kws_bytes_written < bytes_read && kws_bytes_written > 0 ) /* If the buffer is about to full, it will not write the whole array in it */
      {
        ESP_LOGI(TAG, "KWS : Could only write %d bytes out of %d", kws_bytes_written, bytes_read);
      }else if ( kws_bytes_written <= 0 ) /* The ring buffer is full, so it will not write any data */
      {
        ESP_LOGE(TAG, "KWS : Could Not Write in Ring Buffer: %d ", kws_bytes_written);
      }


      /* Update the timestamp (in ms) to let the model know that new data has arrived */
      g_latest_audio_timestamp = g_latest_audio_timestamp + ((1000 * (kws_bytes_written / 2)) / kAudioSampleFrequency);
    }
  

  }/*while loop bracket*/

#endif
  vTaskDelete(NULL);
}


TfLiteStatus InitAudioRecording() 
{
  /* Initalize the ringbuffers */
  g_KWS_audio_capture_buffer = rb_init("tf_ringbuffer", kAudioCaptureBufferSizeKWS);

  /* Check ringbuffer intalizing */
  if (!g_KWS_audio_capture_buffer) 
  {
    ESP_LOGE(TAG, "Error creating KWS ring buffer");
    return kTfLiteError;
  }

  /* create CaptureSamples Task which will get the i2s_data from mic and fill it
   * in the ring buffer */

  /* Create the task in CORE 1 */
  xTaskCreatePinnedToCore( CaptureSamples , "CaptureSamples" , 1024*4 , NULL, 10, &g_capture_audio_task_handler , 1);
  while (!g_latest_audio_timestamp) 
  {
    vTaskDelay(1); // one tick delay to avoid watchdog
  }

  return kTfLiteOk;
}

/***
 * This function gets audio samples for KWS task, which fills it's ring buffer
*/
TfLiteStatus GetAudioSamples_KWS(int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) 
{
  /* If it's is the first time, Init the microphone */
  if (!g_is_audio_initialized) 
  {
    TfLiteStatus init_status = InitAudioRecording();
    if (init_status != kTfLiteOk) {
      return init_status;
    }
    g_is_audio_initialized = true;
  }

  /* copy 160 samples (320 bytes) into output_buff from history */
  memcpy((void*)(g_audio_output_buffer_KWS), (void*)(g_history_buffer),
         history_samples_to_keep * sizeof(int16_t));

  /* copy 320 samples (640 bytes) from rb at ( int16_t*(g_audio_output_buffer_KWS) +
   * 160 ), first 160 samples (320 bytes) will be from history */
  int bytes_read = rb_read(g_KWS_audio_capture_buffer,
              ((uint8_t*)(g_audio_output_buffer_KWS + history_samples_to_keep)),
              new_samples_to_get * sizeof(int16_t), pdMS_TO_TICKS(200));
  
  /* Check reading */
  if (bytes_read < 0) 
  {
    ESP_LOGE(TAG, " Model Could not read data from Ring Buffer");
  }
  else if (bytes_read < new_samples_to_get * sizeof(int16_t)) 
  {
    ESP_LOGD(TAG, "RB FILLED RIGHT NOW IS %d",
             rb_filled(g_KWS_audio_capture_buffer));
    ESP_LOGD(TAG, " Partial Read of Data by Model ");
    ESP_LOGV(TAG, " Could only read %d bytes when required %d bytes ",
             bytes_read, (int) (new_samples_to_get * sizeof(int16_t)));
  }

  /* copy 320 bytes from output_buff into history */
  memcpy((void*)(g_history_buffer),
         (void*)(g_audio_output_buffer_KWS + new_samples_to_get),
         history_samples_to_keep * sizeof(int16_t));

  *audio_samples_size = kMaxAudioSampleSize;
  *audio_samples = g_audio_output_buffer_KWS;
  return kTfLiteOk;
}


int32_t LatestAudioTimestamp() 
{
   return g_latest_audio_timestamp; 
}

