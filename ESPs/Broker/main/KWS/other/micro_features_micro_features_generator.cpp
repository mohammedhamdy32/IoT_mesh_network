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

#include "micro_features_micro_features_generator.h"

#include <cmath>
#include <cstring>

#include "lib/frontend.h"
#include "lib/frontend_util.h"
#include "micro_model_settings.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "lib/mfcc/mfcc.h"
#include "lib/mfcc/spectrogram.h"

// Configure FFT to output 16 bit fixed point.
#define FIXED_POINT 16

// From training
#define MFCC_FEATURE_MIN  (-247.0)
#define MFCC_FEATURE_MAX  (30.0)


static int window_size = 480;
static int stride = 320;
static bool magnitude_squared = true;
static int output_height;
static tflite::internal::Spectrogram spectrogram;


namespace {

FrontendState g_micro_features_state;
bool g_is_first_time = true;
bool need_resize = true;

}  // namespace

TfLiteStatus InitializeMicroFeatures(tflite::ErrorReporter* error_reporter) {
  FrontendConfig config;
  config.window.size_ms = kFeatureDurationMs;
  config.window.step_size_ms = kFeatureStrideMs;
  config.noise_reduction.smoothing_bits = 10;
  config.filterbank.num_channels = g_kFeatureSize;
  config.filterbank.lower_band_limit = 125.0;
  config.filterbank.upper_band_limit = 3500.0; // 3500 with our changes, 7500 tensorflow default
  config.noise_reduction.smoothing_bits = 10;
  config.noise_reduction.even_smoothing = 0.025;
  config.noise_reduction.odd_smoothing = 0.06;
  config.noise_reduction.min_signal_remaining = 0.05;
  config.pcan_gain_control.enable_pcan = 1;
  config.pcan_gain_control.strength = 0.95;
  config.pcan_gain_control.offset = 80.0;
  config.pcan_gain_control.gain_bits = 21;
  config.log_scale.enable_log = 1;
  config.log_scale.scale_shift = 6;
  
  if (!FrontendPopulateState(&config, &g_micro_features_state,
                             kAudioSampleFrequency)) {
    TF_LITE_REPORT_ERROR(error_reporter, "FrontendPopulateState() failed");
    return kTfLiteError;
  }

  if (mfcc.Initialize( g_micro_features_state.fft.fft_size / 2 + 1,
                       kAudioSampleFrequency,
                       config.filterbank.lower_band_limit,
                       config.filterbank.upper_band_limit,
                       config.filterbank.num_channels,
                       g_kFeatureSize,
                       1e-12
                     ) == false) 
  {
    std::cout << "Mfcc initalize failed\n";
  }
  
  g_is_first_time = true;
  return kTfLiteOk;
}

// This is not exposed in any header, and is only used for testing, to ensure
// that the state is correctly set up before generating results.
void SetMicroFeaturesNoiseEstimates(const uint32_t* estimate_presets) 
{
  for (int i = 0; i < g_micro_features_state.filterbank.num_channels; ++i) 
  {
    g_micro_features_state.noise_reduction.estimate[i] = estimate_presets[i];
  }
}

TfLiteStatus GenerateMicroFeatures(tflite::ErrorReporter* error_reporter,
                                   const int16_t* input, int input_size,
                                   int output_size, int8_t* output,
                                   size_t* num_samples_read) {
  const int16_t* frontend_input;
  static std::vector<std::vector<double>> spectrogram_output;
  static std::vector<double> spectrogram_input;
  static std::vector<double> mfcc_output;


  if (g_is_first_time) {
    // printf ("Generatemicro first \n");
    spectrogram.Initialize(window_size, stride);        
                                                        
    frontend_input = input;
    g_is_first_time = false;

    spectrogram_input.resize(input_size);


  } else 
  {
    int stride_size_samples = ((kAudioSampleFrequency * (kFeatureDurationMs - kFeatureStrideMs)) / 1000);
    // printf ("\nGeneratemicro \n");
    frontend_input = input + stride_size_samples;
    
    // Don't calculate spectrogram again for overlapping window
    input_size -= stride_size_samples;
    input += stride_size_samples;

    // Resize spectrogram_input vector also
    // Since after first time only stride length of samples are used
    if (need_resize == true) 
    {
      spectrogram_input.resize(input_size);
      need_resize = false;
    }
  }
  // printf (" input_size: %d\n", input_size);
  // for (int i = 0; i < input_size; ++i)
  // {
  //   printf ("%d,", input[i]);
  // }
  // printf ("\n");

  //printf ("INPUT \n");
  for (int i = 0; i < input_size; i++)
  {
    /* The spectogram function takes a double, so nomalize the audio */
    // Normalize according to what is done in training.
    // WAV values from [-32768, 32767] to [-1, 1]
    if (input[i] < 0)
    {
      //spectrogram_input.push_back((double)input[i] / 32768.0);
      spectrogram_input[i] = (double)input[i] / 32768.0;
    }
    else 
    {
      //spectrogram_input.push_back((double)input[i] / 32767.0);
      spectrogram_input[i] = (double)input[i] / 32767.0;
    }
    //if (i < 10)
    // printf("%f,",spectrogram_input[i]);
    // std::cout << spectrogram_input[i] << ",";
  }
  // printf ("\n\n");


  spectrogram.ComputeSquaredMagnitudeSpectrogram(spectrogram_input, &spectrogram_output);
  //spectrogram_input.clear();
  // printf ("SPECTROGRAM \n");
 
  // printf ("Spectrogram.size() = %zu\n", spectrogram_output.size());
  // printf ("Spectrogram[0].size() = %zu\n", spectrogram_output[0].size());
  // for( int k=0; k <257 ; k++ )
  //    printf("%f," ,spectrogram_output[0][k] );


  // MFCC
  mfcc.Compute(spectrogram_output[0], &mfcc_output);
  // printf ( "\nMFCC.size() = %zu\n",output_size );
  // for( int k=0; k <output_size ; k++ )
  //    printf("%f," ,mfcc_output[k] );

  // output_size if 40, coming from model_settings.h
  for (int i = 0; i < output_size; i++)
  {
    // Quantization as done in training
    int16_t quantized_value = round((255. * (mfcc_output[i] - MFCC_FEATURE_MIN)) / (MFCC_FEATURE_MAX - MFCC_FEATURE_MIN));
    if (quantized_value < 0) 
    {
      quantized_value = 0;
    }
    else if (quantized_value > 255) 
    {
      quantized_value = 255;
    }
    quantized_value -= 128;

    output[i] = quantized_value;
    // printf ("%d ", output[i]);
  }
  // printf ("\n");

 
  return kTfLiteOk;
}
