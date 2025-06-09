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
    : encoder_(nullptr),
      AudioPanner(Speakers::kBinaural, samplesPerBlock, sampleRate) {
  // Create the encoder for encoding to the desired ambisonic order.
  encoder_ = std::make_unique<obr::ObrImpl>(samplesPerBlock, sampleRate);
  encoder_->AddAudioElement(obr::AudioElementType::kObjectMono);

  // Resize internal buffers for API calls.
  inputBufferPlanar_ = obr::AudioBuffer(1, samplesPerBlock);
  outputBufferPlanar_ =
      obr::AudioBuffer(Speakers::kBinaural.getNumChannels(), samplesPerBlock);
}

BinauralPanner::~BinauralPanner() {}

void BinauralPanner::positionUpdated() {
  encoder_->UpdateObjectPosition(0, currPos_.azimuth, currPos_.elevation,
                                 currPos_.distance);
}

void BinauralPanner::process(juce::AudioBuffer<float>& inputBuffer,
                             juce::AudioBuffer<float>& outputBuffer) {
  outputBuffer.clear();

  // Fetch the first channel, which is the only channel to be panned
  const float* rPtr = inputBuffer.getReadPointer(0);
  for (int j = 0; j < kSamplesPerBlock_; ++j) {
    inputBufferPlanar_[0][j] = rPtr[j];
  }

  // Convert input buffer to planar vector and add spatial information.
  encoder_->Process(inputBufferPlanar_, &outputBufferPlanar_);

  // Write the processed planar output data to the intermediate buffer.
  for (int i = 0; i < Speakers::kBinaural.getNumChannels(); i++) {
    for (int j = 0; j < kSamplesPerBlock_; j++) {
      outputBuffer.setSample(i, j, outputBufferPlanar_[i][j]);
    }
  }
}