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

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <vector>

/**
 * @brief Computes the `true peak` measurement described in ITU-R BS.1770-4.
 * Uses a polyphase filter approach similar to the FFmpeg.
 *
 */

class TruePeak {
 public:
  float compute(const juce::AudioBuffer<float>& buffer);
  void reset(const double sampleRate, const juce::AudioChannelSet& channelSet);

 private:
  // ITU-R BS.1770-4 48-tap FIR filter coefficients split into 4 phases of 12
  // taps
  static constexpr int kHistorySize = 11;
  static constexpr int kTapsPerPhase = 12;
  static constexpr int kNumPhases = 4;

  // The 4 polyphase filter coefficient arrays (12 taps each)
  static constexpr float kPhase0[kTapsPerPhase] = {
      0.0017089843750f,  0.0109863281250f,  -0.0196533203125f,
      0.0332031250000f,  -0.0594482421875f, 0.1373291015625f,
      0.9721679687500f,  -0.1022949218750f, 0.0476074218750f,
      -0.0266113281250f, 0.0148925781250f,  -0.0083007812500f};

  static constexpr float kPhase1[kTapsPerPhase] = {
      -0.0291748046875f, 0.0292968750000f,  -0.0517578125000f,
      0.0891113281250f,  -0.1665039062500f, 0.4650878906250f,
      0.7797851562500f,  -0.2003173828125f, 0.1015625000000f,
      -0.0582275390625f, 0.0330810546875f,  -0.0189208984375f};

  static constexpr float kPhase2[kTapsPerPhase] = {
      -0.0189208984375f, 0.0330810546875f,  -0.0582275390625f,
      0.1015625000000f,  -0.2003173828125f, 0.7797851562500f,
      0.4650878906250f,  -0.1665039062500f, 0.0891113281250f,
      -0.0517578125000f, 0.0292968750000f,  -0.0291748046875f};

  static constexpr float kPhase3[kTapsPerPhase] = {
      -0.0083007812500f, 0.0148925781250f,  -0.0266113281250f,
      0.0476074218750f,  -0.1022949218750f, 0.9721679687500f,
      0.1373291015625f,  -0.0594482421875f, 0.0332031250000f,
      -0.0196533203125f, 0.0109863281250f,  0.0017089843750f};

  void processSingleSampleLinear(const float* data, int currentIndex);
  void processSingleSampleWithHistory(
      const float* data, int currentIndex,
      const std::array<float, kHistorySize>& history);
  void updatePeak(float o0, float o1, float o2, float o3);

  juce::AudioChannelSet channelSet_ = juce::AudioChannelSet::disabled();
  std::vector<std::array<float, kHistorySize>> channelHistory_;
  float currentTruePeak_ = -std::numeric_limits<float>::infinity();
};