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

BinauralPanner::BinauralPanner(const int samplesPerBlock, const int sampleRate)
    : AudioPanner(Speakers::kBinaural, samplesPerBlock, sampleRate),
      ambisonicPanner_(Speakers::kHOA3, samplesPerBlock, sampleRate),
      ambisonicBuffer_(Speakers::kHOA3.getNumChannels(), samplesPerBlock),
      encoder_(nullptr) {
  // Create the renderer for decoding third-order ambisonics to binaural.
  encoder_ = std::make_unique<obr::ObrImpl>(samplesPerBlock, sampleRate);
  encoder_->AddAudioElement(obr::AudioElementType::k3OA);

  // Resize internal buffers for API calls.
  inputBufferPlanar_ =
      obr::AudioBuffer(Speakers::kHOA3.getNumChannels(), samplesPerBlock);
  outputBufferPlanar_ =
      obr::AudioBuffer(Speakers::kBinaural.getNumChannels(), samplesPerBlock);
}

BinauralPanner::~BinauralPanner() {}

void BinauralPanner::positionUpdated() {
  ambisonicPanner_.setPolarPosition(currPos_);
}

void BinauralPanner::process(juce::AudioBuffer<float>& inputBuffer,
                             juce::AudioBuffer<float>& outputBuffer) {
  outputBuffer.clear();

  ambisonicPanner_.process(inputBuffer, ambisonicBuffer_);
  for (int i = 0; i < ambisonicBuffer_.getNumChannels(); ++i) {
    const float* rPtr = ambisonicBuffer_.getReadPointer(i);
    for (int j = 0; j < kSamplesPerBlock_; ++j) {
      inputBufferPlanar_[i][j] = rPtr[j];
    }
  }

  encoder_->Process(inputBufferPlanar_, &outputBufferPlanar_);

  // Write the processed planar output data to the intermediate buffer.
  for (int i = 0; i < Speakers::kBinaural.getNumChannels(); i++) {
    for (int j = 0; j < kSamplesPerBlock_; j++) {
      outputBuffer.setSample(i, j, outputBufferPlanar_[i][j]);
    }
  }
}
