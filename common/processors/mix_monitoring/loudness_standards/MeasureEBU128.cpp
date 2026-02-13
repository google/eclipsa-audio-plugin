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

#include "MeasureEBU128.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <logger/logger.h>

#include <limits>

#include "TruePeak.h"

MeasureEBU128::MeasureEBU128(const double sampleRate,
                             const juce::AudioChannelSet& channelSet)
    : loudnessMeter_(), kSampleRate_(sampleRate), playbackLayout_(channelSet) {
  reset(playbackLayout_, juce::AudioBuffer<float>());
}

MeasureEBU128::LoudnessStats MeasureEBU128::measureLoudness(
    const juce::AudioChannelSet& currPlaybackLayout,
    const juce::AudioBuffer<float>& buffer) {
  // If the playback layout has changed, reconfigure and reset internal loudness
  // stats.
  if (buffer.getNumChannels() != currPlaybackLayout.size() ||
      playbackLayout_ != currPlaybackLayout) {
    reset(currPlaybackLayout, buffer);
    LOG_INFO(
        0, "measureLoudness: Mismatch between provided layout and buffer size");
  }

  // Update max permitted true peak level.
  loudnessStats_.loudnessTruePeak =
      std::max(loudnessStats_.loudnessTruePeak, calculateTruePeakLevel(buffer));

  // Update the max digital peak level.
  loudnessStats_.loudnessDigitalPeak = std::max(
      loudnessStats_.loudnessDigitalPeak, calculateDigitalPeak(buffer));

  // Update the LUF based loudness stats
  loudnessMeter_.processBlock(buffer);
  loudnessStats_.loudnessMomentary = loudnessMeter_.getMomentaryLoudness();
  loudnessStats_.loudnessShortTerm = loudnessMeter_.getShortTermLoudness();
  loudnessStats_.loudnessIntegrated = loudnessMeter_.getIntegratedLoudness();
  loudnessStats_.loudnessRange = loudnessMeter_.getLoudnessRange();

  return loudnessStats_;
}

void MeasureEBU128::reset(const juce::AudioChannelSet& currPlaybackLayout,
                          const juce::AudioBuffer<float>& buffer) {
  playbackLayout_ = currPlaybackLayout;
  loudnessMeter_.prepareToPlay(kSampleRate_, playbackLayout_.size(),
                               buffer.getNumSamples(), 1);

  // Initialize true peak meter
  truePeakMeter_.reset(kSampleRate_, currPlaybackLayout);

  loudnessStats_ = {-std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity()};
}

// ITU-R BS.1770-4 True Peak using polyphase FIR filter.
float MeasureEBU128::calculateTruePeakLevel(
    const juce::AudioBuffer<float>& buffer) {
  return truePeakMeter_.compute(buffer);
}

// digital_peak specifies the the digital (sampled) peak of the audio signal.
// cited section 3.7.4 of the IAMF spec.
float MeasureEBU128::calculateDigitalPeak(
    const juce::AudioBuffer<float>& buffer) {
  float digitalPeak = -70.0f;  // Set silence at -70dBFS
  // iterate through each channel
  for (int i = 0; i < buffer.getNumChannels(); ++i) {
    const float* channelData = buffer.getReadPointer(i);
    // iterate through samples in channel data
    for (int j = 0; j < buffer.getNumSamples(); ++j) {
      // compare both positive and negative extremes of the digital signal
      digitalPeak = std::max(digitalPeak, std::abs(channelData[j]));
    }
  }
  float digitalPeakdB = 20.f * std::log10(digitalPeak);
  // boiler plate check for unreasonable values
  if (digitalPeakdB > 15.f) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  return digitalPeakdB;
}