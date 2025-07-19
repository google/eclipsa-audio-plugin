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

#include "BedToBedRdr.h"

#include "BedToBedRdrMats.h"
#include "logger/logger.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

std::unique_ptr<Renderer> BedToBedRdr::createBedToBedRdr(
    const Speakers::AudioElementSpeakerLayout inputLayout,
    const Speakers::AudioElementSpeakerLayout playbackLayout) {
  // Lookup b2b conversion matrix.
  const float* rdrMat =
      getMatrixFromLayouts(inputLayout.getExplBaseLayout(), playbackLayout);

  // If a conversion matrix exists the playback layout is renderable from the
  // input layout.
  if (rdrMat) {
    return std::unique_ptr<Renderer>(
        new BedToBedRdr(rdrMat, inputLayout, playbackLayout));
  } else {
    LOG_ERROR(0,
              "Failed to create BedToBedRdr: No valid conversion matrix found. "
              "\n Returning nullptr.");
    return nullptr;
  }
}

BedToBedRdr::BedToBedRdr(
    const float* renderMatrix,
    const Speakers::AudioElementSpeakerLayout inputLayout,
    const Speakers::AudioElementSpeakerLayout playbackLayout)
    : kRenderMatrix_(renderMatrix),
      kInputLayout_(inputLayout),
      kChannelMap_(kInputLayout_.getExplValidChannels()),
      kNumChIn_(inputLayout.getExplBaseLayout().getNumChannels()),
      kNumChOut_(playbackLayout.getNumChannels()) {}

void BedToBedRdr::render(const FBuffer& srcBuffer, FBuffer& outBuffer) {
  prepInterBuff(srcBuffer.getNumSamples());

  if (kChannelMap_) {
    // Copy source channels to their mapped channels in the intermediate buffer.
    int srcIdx = 0;
    for (const int destIdx : kChannelMap_.value()) {
      interBuffer_.copyFrom(destIdx, 0, srcBuffer.getReadPointer(srcIdx),
                            srcBuffer.getNumSamples());
      ++srcIdx;
    }
    // Only when the expanded layout's base layout is NOT the same as the
    // playback layout should rendering be done.
    if (kRenderMatrix_) {
      renderITU(interBuffer_, outBuffer);
    }
  } else {
    renderITU(srcBuffer, outBuffer);
  }
}

inline void BedToBedRdr::prepInterBuff(const int numSamples) {
  // Allocate a new buffer if necessary.
  if (interBuffer_.getNumChannels() !=
          kInputLayout_.getExplBaseLayout().getNumChannels() ||
      interBuffer_.getNumSamples() != numSamples) {
    interBuffer_.setSize(kInputLayout_.getExplBaseLayout().getNumChannels(),
                         numSamples);
  }
  interBuffer_.clear();
}

inline void BedToBedRdr::renderITU(const FBuffer& srcBuffer,
                                   FBuffer& outBuffer) const {
  // To generate an output channel:
  // Traverse col. applying gains to source channels, summing to outBuffer.
  for (int outChIdx = 0; outChIdx < kNumChOut_; ++outChIdx) {
    for (int inChIdx = 0; inChIdx < kNumChIn_; ++inChIdx) {
      float gainToApply = kRenderMatrix_[inChIdx * kNumChOut_ + outChIdx];
      outBuffer.addFrom(outChIdx, 0, srcBuffer.getReadPointer(inChIdx),
                        outBuffer.getNumSamples(), gainToApply);
    }
  }
}