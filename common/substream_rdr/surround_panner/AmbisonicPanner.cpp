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

#include "AmbisonicPanner.h"

#include <algorithm>

AmbisonicPanner::AmbisonicPanner(
    const Speakers::AudioElementSpeakerLayout pannedLayout,
    const int samplesPerBlock, const int sampleRate)
    : AudioPanner(pannedLayout, samplesPerBlock, sampleRate),
      unitInputPlanar_(1, 1),
      unitOutputPlanar_(pannedLayout.getNumChannels(), 1),
      encoder_(std::make_unique<obr::AmbisonicEncoder>(
          1, (int)(std::sqrt(pannedLayout.getNumChannels()) - 1.0))),
      currentGains_(pannedLayout.getNumChannels(), 0.f),
      targetGains_(pannedLayout.getNumChannels(), 0.f),
      gainSteps_(pannedLayout.getNumChannels(), 0.f) {
  unitInputPlanar_[0][0] = 1.f;
}

AmbisonicPanner::~AmbisonicPanner() {}

void AmbisonicPanner::setPolarPosition(const ear::PolarPosition& position) {
  currPos_ = position;
  positionUpdated();
}

void AmbisonicPanner::positionUpdated() {
  // NOTE: As we pan mono only, just set the source of the first input channel.
  encoder_->SetSource(0, 1.f, currPos_.azimuth, currPos_.elevation,
                      currPos_.distance);

  // The encoder is a per-channel gain, so encoding a unit sample reads it out.
  encoder_->ProcessPlanarAudioData(unitInputPlanar_, &unitOutputPlanar_);
  for (size_t i = 0; i < targetGains_.size(); ++i) {
    targetGains_[i] = unitOutputPlanar_[i][0];
  }

  if (!hasRendered_) {
    currentGains_ = targetGains_;
    rampSamplesRemaining_ = 0;
    return;
  }

  // Ramp from wherever the gains are now, even mid-ramp, to the new target.
  for (size_t i = 0; i < targetGains_.size(); ++i) {
    gainSteps_[i] = (targetGains_[i] - currentGains_[i]) / kSamplesPerBlock_;
  }
  rampSamplesRemaining_ = kSamplesPerBlock_;
}

void AmbisonicPanner::process(juce::AudioBuffer<float>& inputBuffer,
                              juce::AudioBuffer<float>& outputBuffer,
                              const int numSamples) {
  const float* input = inputBuffer.getReadPointer(0);
  const int rampSamples = std::min(numSamples, rampSamplesRemaining_);
  const bool rampEnds = rampSamples == rampSamplesRemaining_;

  for (int i = 0; i < kPannedLayout_.getNumChannels(); ++i) {
    float* output = outputBuffer.getWritePointer(i);
    float gain = currentGains_[i];
    for (int j = 0; j < rampSamples; ++j) {
      gain += gainSteps_[i];
      output[j] = input[j] * gain;
    }
    if (rampEnds) {
      gain = targetGains_[i];
    }
    for (int j = rampSamples; j < numSamples; ++j) {
      output[j] = input[j] * gain;
    }
    currentGains_[i] = gain;
  }

  rampSamplesRemaining_ -= rampSamples;
  hasRendered_ = true;
}
