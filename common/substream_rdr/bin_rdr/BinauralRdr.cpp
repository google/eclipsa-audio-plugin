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

#include "BinauralRdr.h"

#include "obr_impl.h"

static obr::AudioElementType asOBRLayout(
    const Speakers::AudioElementSpeakerLayout layout) {
  Speakers::AudioElementSpeakerLayout explLayout = layout.getExplBaseLayout();
  switch (explLayout) {
    case Speakers::kMono:
      return obr::AudioElementType::kLayoutMono;
    case Speakers::kStereo:
      return obr::AudioElementType::kLayoutStereo;
    case Speakers::k3Point1Point2:
      return obr::AudioElementType::kLayout3_1_2_ch;
    case Speakers::k5Point1:
      return obr::AudioElementType::kLayout5_1_0_ch;
    case Speakers::k5Point1Point2:
      return obr::AudioElementType::kLayout5_1_2_ch;
    case Speakers::k5Point1Point4:
      return obr::AudioElementType::kLayout5_1_4_ch;
    case Speakers::k7Point1:
      return obr::AudioElementType::kLayout7_1_0_ch;
    case Speakers::k7Point1Point2:
      return obr::AudioElementType::kLayout7_1_2_ch;
    case Speakers::k7Point1Point4:
      return obr::AudioElementType::kLayout7_1_4_ch;
    case Speakers::kExpl9Point1Point6:
      return obr::AudioElementType::kLayout9_1_6_ch;
    case Speakers::kHOA1:
      return obr::AudioElementType::k1OA;
    case Speakers::kHOA2:
      return obr::AudioElementType::k2OA;
    case Speakers::kHOA3:
      return obr::AudioElementType::k3OA;
    case Speakers::kHOA4:
      return obr::AudioElementType::k4OA;
    default:
      return static_cast<obr::AudioElementType>(-1);
  }
}

std::unique_ptr<Renderer> BinauralRdr::createBinauralRdr(
    const Speakers::AudioElementSpeakerLayout layout, const int numSamples,
    const int sampleRate) {
  // Input layout == output layout. No rendering to be done.
  if (layout == Speakers::kBinaural) {
    return std::unique_ptr<Renderer>(new BinauralCopyRdr());
  }

  // Check that a binaural renderer can be created for the given layout.
  obr::AudioElementType inputType = asOBRLayout(layout);
  if (inputType == static_cast<obr::AudioElementType>(-1)) {
    return nullptr;
  }

  // If numSamples < 32 don't create a binaural renderer.
  // Seems like OBRs FFT requires at least 32 samples.
  if (numSamples < 32) {
    return nullptr;
  }

  // Construct the binaural renderer.
  return std::unique_ptr<Renderer>(
      new BinauralRdr(inputType, layout, numSamples, sampleRate));
}

BinauralRdr::BinauralRdr(const obr::AudioElementType layout,
                         const Speakers::AudioElementSpeakerLayout spkrLayout,
                         const int numSamples, const int sampleRate)
    : audioElementlayout_(spkrLayout), numSamplesIn_(numSamples) {
  binauralRdr_ = std::make_unique<obr::ObrImpl>(numSamplesIn_, sampleRate);
  binauralRdr_->AddAudioElement(layout);

  // Initialize planar buffers for API calls.
  inputBufferPlanar_ = obr::AudioBuffer(
      audioElementlayout_.getExplBaseLayout().getNumChannels(), numSamplesIn_);
  outputBufferPlanar_ =
      obr::AudioBuffer(Speakers::kBinaural.getNumChannels(), numSamplesIn_);
  inputBufferPlanar_.Clear();
  outputBufferPlanar_.Clear();
}

BinauralRdr::~BinauralRdr() {}

void BinauralRdr::render(const juce::AudioBuffer<float>& inputBuffer,
                         juce::AudioBuffer<float>& outputBuffer) {
  // For non-expanded layouts, copy input buffer to planar buffer directly
  if (!audioElementlayout_.isExpandedLayout()) {
    for (int i = 0; i < audioElementlayout_.getNumChannels(); ++i) {
      const float* rPtr = inputBuffer.getReadPointer(i);
      for (int j = 0; j < numSamplesIn_; ++j) {
        inputBufferPlanar_[i][j] = rPtr[j];
      }
    }
  } else {
    // For expanded layouts, copy the input channels into the correct positions
    // in the planar buffer.
    const auto& validChannels = audioElementlayout_.getExplValidChannels();
    for (int i = 0; i < validChannels->size(); i++) {
      const float* rPtr = inputBuffer.getReadPointer(i);
      for (int j = 0; j < numSamplesIn_; ++j) {
        inputBufferPlanar_[validChannels->at(i)][j] = rPtr[j];
      }
    }
  }

  binauralRdr_->Process(inputBufferPlanar_, &outputBufferPlanar_);

  for (int i = 0; i < Speakers::kBinaural.getNumChannels(); i++) {
    for (int j = 0; j < numSamplesIn_; j++) {
      outputBuffer.setSample(i, j, outputBufferPlanar_[i][j]);
    }
  }
}