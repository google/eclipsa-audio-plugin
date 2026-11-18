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

#include "BinauralPanner.h"

#include <algorithm>

BinauralPanner::BinauralPanner(const int samplesPerBlock, const int sampleRate)
    : AudioPanner(Speakers::kBinaural, samplesPerBlock, sampleRate),
      ambisonicPanner_(Speakers::kHOA3, samplesPerBlock, sampleRate),
      ambisonicBuffer_(Speakers::kHOA3.getNumChannels(), samplesPerBlock),
      fifoInputPlanar_(Speakers::kHOA3.getNumChannels(), samplesPerBlock),
      fifoOutputPlanar_(Speakers::kBinaural.getNumChannels(), samplesPerBlock),
      encoder_(std::make_unique<obr::ObrImpl>(samplesPerBlock, sampleRate)) {
  // Decode third-order ambisonics to binaural.
  encoder_->AddAudioElement(obr::AudioElementType::k3OA);
  fifoOutputPlanar_.Clear();
}

BinauralPanner::~BinauralPanner() {}

void BinauralPanner::positionUpdated() {
  ambisonicPanner_.setPolarPosition(currPos_);
}

void BinauralPanner::process(juce::AudioBuffer<float>& inputBuffer,
                             juce::AudioBuffer<float>& outputBuffer,
                             const int numSamples) {
  ambisonicPanner_.process(inputBuffer, ambisonicBuffer_, numSamples);

  int done = 0;
  while (done < numSamples) {
    const int count =
        std::min(numSamples - done, kSamplesPerBlock_ - fifoPosition_);
    for (int i = 0; i < ambisonicBuffer_.getNumChannels(); ++i) {
      const float* rPtr = ambisonicBuffer_.getReadPointer(i, done);
      for (int j = 0; j < count; ++j) {
        fifoInputPlanar_[i][fifoPosition_ + j] = rPtr[j];
      }
    }
    for (int i = 0; i < outputBuffer.getNumChannels() &&
                    i < Speakers::kBinaural.getNumChannels();
         ++i) {
      float* wPtr = outputBuffer.getWritePointer(i, done);
      for (int j = 0; j < count; ++j) {
        wPtr[j] = fifoOutputPlanar_[i][fifoPosition_ + j];
      }
    }

    done += count;
    fifoPosition_ += count;
    if (fifoPosition_ == kSamplesPerBlock_) {
      encoder_->Process(fifoInputPlanar_, &fifoOutputPlanar_);
      fifoPosition_ = 0;
    }
  }
}
