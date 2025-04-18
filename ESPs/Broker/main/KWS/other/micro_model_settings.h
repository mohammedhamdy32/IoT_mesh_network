/* Copyright 2023 The TensorFlow Authors. All Rights Reserved.

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

#ifndef TENSORFLOW_LITE_MICRO_EXAMPLES_MICRO_SPEECH_MICRO_MODEL_SETTINGS_H_
#define TENSORFLOW_LITE_MICRO_EXAMPLES_MICRO_SPEECH_MICRO_MODEL_SETTINGS_H_

/************* Defines ******************/
#define I2S_NUM     I2S_NUM_1

/* Microphone pins */
#define  MICROPHONE_I2S_DOUT_PIN                 20 //15
#define  MICROPHONE_I2S_CLK_PIN                  19 //14
#define  MICROPHONE_I2S_WS_PIN                   21 //13

/* Buffer */
#define  MICROPHONE_DMA_BUFFER_SIZE                  (300) 
#define  MICROPHONE_DMA_BUFFER_COUNT                  (3) 


/* The following values are derived from values used during model training. */
/* If you change the way you preprocess the input, update all these constants. */
constexpr int kMaxAudioSampleSize = 480;
constexpr int kAudioSampleFrequency = 16000;
constexpr uint16_t g_kFeatureSize  = 40; /* The size of my spectogram */
constexpr uint16_t g_kFeatureCount = 49; /* The size of my spectogram */
constexpr int g_kFeatureElementCount = (g_kFeatureSize * g_kFeatureCount); /* spectogram total size */
constexpr int kFeatureStrideMs = 20;
constexpr int kFeatureDurationMs = 30;

/* Variables for the model's output categories. */
constexpr int kCategoryCount = 6;
constexpr const char* kCategoryLabels[kCategoryCount] = {
    "ON",
    "left",
    "right",
    "OFF",
    "silence",
    "unknown",
    
};

#endif  // TENSORFLOW_LITE_MICRO_EXAMPLES_MICRO_SPEECH_MICRO_MODEL_SETTINGS_H_
