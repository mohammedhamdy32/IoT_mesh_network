/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

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

#include <cmath>

#include "mfcc.h"

//#include "tensorflow/core/platform/logging.h"
#include <iostream>

tensorflow::Mfcc mfcc;

namespace tensorflow {

// const double kDefaultUpperFrequencyLimit = 4000;
// const double kDefaultLowerFrequencyLimit = 20;
// const double kFilterbankFloor = 1e-12;
// const int kDefaultFilterbankChannelCount = 40;
// const int kDefaultDCTCoefficientCount = 13;

Mfcc::Mfcc()
    : initialized_(false) {}

bool Mfcc::Initialize(int input_length, 
                      double input_sample_rate,
                      double lower_freq_limit,
                      double upper_freq_limit,
                      int filterbank_channel_count,
                      int dct_coefficient_count,
                      double kFilterbankFloor) {

  this->lower_frequency_limit_ = lower_freq_limit;
  this->upper_frequency_limit_ = upper_freq_limit;
  this->filterbank_channel_count_ = filterbank_channel_count;
  this->dct_coefficient_count_ = dct_coefficient_count;
  this->kFilterbankFloor_ = kFilterbankFloor_;
                        
  bool initialized = mel_filterbank_.Initialize(
      input_length, input_sample_rate, filterbank_channel_count_,
      lower_frequency_limit_, upper_frequency_limit_);
  initialized &=
      dct_.Initialize(filterbank_channel_count_, dct_coefficient_count_);
  initialized_ = initialized;
  return initialized;
}

void Mfcc::Compute(const std::vector<double>& spectrogram_frame,
                   std::vector<double>* output) const {
  if (!initialized_) {
    std::cout << "Mfcc not initialized.";
    return;
  }
  std::vector<double> working;
  mel_filterbank_.Compute(spectrogram_frame, &working);
  for (size_t i = 0; i < working.size(); ++i) {
    double val = working[i];
    if (val < this->kFilterbankFloor_) {
      val = this->kFilterbankFloor_;
    }
    working[i] = log(val);
  }
  dct_.Compute(working, output);
}

}  // namespace tensorflow