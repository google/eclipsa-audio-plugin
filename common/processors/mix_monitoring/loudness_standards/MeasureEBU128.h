/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <logger/logger.h>

#include "EBU128LoudnessMeter.h"

class MeasureEBU128 {
 public:
  // EBU128 loudness statistics.
  struct LoudnessStats {
    float loudnessMomentary, loudnessShortTerm, loudnessIntegrated;
    float loudnessRange, loudnessTruePeak, loudnessDigitalPeak;
  };

  /**
   * @brief Create a loudness measurement object for a given sample rate and
   * rendering layout.
   * NOTE:
   * Filter coefficients are hard-coded from ITU 1770 for a sample rate of
   * 48kHz, so calculations at other sample rates are currently expected to be
   * inaccurate.
   * @param sampleRate
   * @param chData Playback layout for which to measure loudness.
   */
  MeasureEBU128(
      const double sampleRate,
      const juce::AudioChannelSet& channelSet = juce::AudioChannelSet::mono());

  // Calculate EBU128 loudness statistics.
  LoudnessStats measureLoudness(const juce::AudioChannelSet& currPlaybackLayout,
                                const juce::AudioBuffer<float>& buffer);

  /**
   * @brief Reset internal measurements.
   *
   */
  void reset(const juce::AudioChannelSet& currPlaybackLayout,
             const juce::AudioBuffer<float>& buffer);

  /**
   * @brief Calculate the true sample peak level for the current buffer of
   * samples. ITU 1770.
   *
   * @param buffer Filtered and squared samples.
   * @return float True peak level for the current buffer.
   */
  float calculateTruePeakLevel(const juce::AudioBuffer<float>& buffer);

  float calculateDigitalPeak(const juce::AudioBuffer<float>& buffer);

  struct LPF {
    juce::dsp::AudioBlock<float> block;
    juce::dsp::ProcessorDuplicator<juce::dsp::FIR::Filter<float>,
                                   juce::dsp::FIR::Coefficients<float>>
        filter;
  };
  // Playback information
  const double kSampleRate_;
  juce::AudioChannelSet playbackLayout_;

  // Library for calculating loudness and range values
  Ebu128LoudnessMeter loudnessMeter_;

  // Resamplers for true peak calculation.
  int upsampleRatio_ = 4;  // Upsampling ratio for true peak calculation.
  juce::AudioBuffer<float> upsampledBuffer_;  // Larger buffer to upsample into.
  std::vector<juce::Interpolators::Lagrange> perChannelResamplers_;
  LPF lpf_;

  // Internal copy of calculated loudness statistics to return when
  // loudnesses' are queried between measurement periods.
  LoudnessStats loudnessStats_{-std::numeric_limits<float>::infinity(),
                               -std::numeric_limits<float>::infinity(),
                               -std::numeric_limits<float>::infinity(),
                               -std::numeric_limits<float>::infinity(),
                               -std::numeric_limits<float>::infinity(),
                               -std::numeric_limits<float>::infinity()};
};
