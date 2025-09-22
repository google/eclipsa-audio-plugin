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

AmbisonicPanner::AmbisonicPanner(
    const Speakers::AudioElementSpeakerLayout pannedLayout,
    const int samplesPerBlock, const int sampleRate)
    : AudioPanner(pannedLayout, samplesPerBlock, sampleRate),
      inputBufferPlanar_(1, kSamplesPerBlock_),
      outputBufferPlanar_(pannedLayout.getNumChannels(), kSamplesPerBlock_),
      encoder_(std::make_unique<obr::AmbisonicEncoder>(
          1, (int)(std::sqrt(pannedLayout.getNumChannels()) - 1.0))) {
  // Create the encoder for encoding to the desired ambisonic order.
  int numSamplesOutPlanar = pannedLayout.getNumChannels() * kSamplesPerBlock_;
}

AmbisonicPanner::~AmbisonicPanner() {}

void AmbisonicPanner::positionUpdated() {
  // NOTE: As we pan mono only, just set the source of the first input channel.
  encoder_->SetSource(0, 1.f, currPos_.azimuth, currPos_.elevation,
                      currPos_.distance);
}

void AmbisonicPanner::process(juce::AudioBuffer<float>& inputBuffer,
                              juce::AudioBuffer<float>& outputBuffer) {
  outputBuffer.clear();

  // Fetch the data for the first channel, since it's the only channel we will
  // pan
  const float* rPtr = inputBuffer.getReadPointer(0);
  for (int j = 0; j < kSamplesPerBlock_; ++j) {
    inputBufferPlanar_[0][j] = rPtr[j];
  }

  // Convert input buffer to planar vector and add spatial information.
  encoder_->ProcessPlanarAudioData(inputBufferPlanar_, &outputBufferPlanar_);

  // Write the processed planar output data to the intermediate buffer.
  for (int i = 0; i < kPannedLayout_.getNumChannels(); i++) {
    for (int j = 0; j < kSamplesPerBlock_; j++) {
      outputBuffer.setSample(i, j, outputBufferPlanar_[i][j]);
    }
  }
}