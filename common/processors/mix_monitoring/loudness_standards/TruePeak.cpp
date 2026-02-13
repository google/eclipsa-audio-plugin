// Copyright 2025 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "TruePeak.h"

void TruePeak::reset(const double sampleRate,
                     const juce::AudioChannelSet& channelSet) {
  channelSet_ = channelSet;
  // Initialize history arrays for each channel
  channelHistory_.clear();
  channelHistory_.resize(channelSet.size());
  for (auto& history : channelHistory_) {
    history.fill(0.0f);
  }
  currentTruePeak_ = -std::numeric_limits<float>::infinity();
}

float TruePeak::compute(const juce::AudioBuffer<float>& buffer) {
  if (channelSet_.isDisabled()) {
    return -1000;
  }
  if (buffer.getNumChannels() != channelSet_.size()) {
    return -1000;
  }

  currentTruePeak_ = -std::numeric_limits<float>::infinity();

  // Process each channel
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    // Skip LFE channel
    if (channelSet_.getTypeOfChannel(ch) == juce::AudioChannelSet::LFE) {
      continue;
    }

    const float* inputData = buffer.getReadPointer(ch);
    int numSamples = buffer.getNumSamples();
    auto& history = channelHistory_[ch];

    // Phase 1: Process the first 11 samples (requires history)
    int overlapCount = std::min(numSamples, kHistorySize);
    for (int i = 0; i < overlapCount; ++i) {
      processSingleSampleWithHistory(inputData, i, history);
    }

    // Phase 2: Process the rest of the block
    for (int i = kHistorySize; i < numSamples; ++i) {
      processSingleSampleLinear(inputData, i);
    }

    // Phase 3: Save the last 11 samples for the next block's history
    if (numSamples >= kHistorySize) {
      for (int i = 0; i < kHistorySize; ++i) {
        history[i] = inputData[numSamples - kHistorySize + i];
      }
    } else {
      // Edge case: Block size is smaller than 11
      // Shift history left and append new samples
      int shift = kHistorySize - numSamples;
      for (int i = 0; i < shift; ++i) {
        history[i] = history[i + numSamples];
      }
      for (int i = 0; i < numSamples; ++i) {
        history[shift + i] = inputData[i];
      }
    }
  }

  // Convert to dB and sanitize outputs
  if (currentTruePeak_ > 0.0f) {
    float truePeakdB = 20.0f * std::log10(currentTruePeak_);
    if (truePeakdB > 15.0f) {
      return std::numeric_limits<float>::quiet_NaN();
    }
    return truePeakdB;
  }

  return -std::numeric_limits<float>::infinity();
}

void TruePeak::processSingleSampleLinear(const float* data, int currentIndex) {
  // Calculate the 4 upsampled points directly from the linear buffer
  float out0 = 0.0f, out1 = 0.0f, out2 = 0.0f, out3 = 0.0f;

  for (int m = 0; m < kTapsPerPhase; ++m) {
    float sample = data[currentIndex - m];
    out0 += kPhase0[m] * sample;
    out1 += kPhase1[m] * sample;
    out2 += kPhase2[m] * sample;
    out3 += kPhase3[m] * sample;
  }

  updatePeak(out0, out1, out2, out3);
}

void TruePeak::processSingleSampleWithHistory(
    const float* data, int currentIndex,
    const std::array<float, kHistorySize>& history) {
  // Calculate the 4 upsampled points using history when necessary
  float out0 = 0.0f, out1 = 0.0f, out2 = 0.0f, out3 = 0.0f;

  for (int m = 0; m < kTapsPerPhase; ++m) {
    float sample;
    int index = currentIndex - m;
    if (index >= 0) {
      sample = data[index];
    } else {
      // Read from history array
      sample = history[kHistorySize + index];
    }

    out0 += kPhase0[m] * sample;
    out1 += kPhase1[m] * sample;
    out2 += kPhase2[m] * sample;
    out3 += kPhase3[m] * sample;
  }

  updatePeak(out0, out1, out2, out3);
}

void TruePeak::updatePeak(float o0, float o1, float o2, float o3) {
  currentTruePeak_ = std::max({currentTruePeak_, std::abs(o0), std::abs(o1),
                               std::abs(o2), std::abs(o3)});
}