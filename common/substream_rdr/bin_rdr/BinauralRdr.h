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

#include "obr/renderer/obr_impl.h"
#include "substream_rdr/rdr_factory/Renderer.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class BinauralRdr : public Renderer {
 public:
  static std::unique_ptr<Renderer> createBinauralRdr(
      const Speakers::AudioElementSpeakerLayout layout, const int numSamples,
      const int sampleRate);

  ~BinauralRdr();

  void render(const juce::AudioBuffer<float>& inputBuffer,
              juce::AudioBuffer<float>& outputBuffer) override;

 private:
  BinauralRdr(const obr::AudioElementType layout,
              const Speakers::AudioElementSpeakerLayout spkrLayout,
              const int numSamples, const int sampleRate);

  int numSamplesIn_;
  obr::AudioBuffer inputBufferPlanar_, outputBufferPlanar_;
  std::unique_ptr<obr::ObrImpl> binauralRdr_;
  Speakers::AudioElementSpeakerLayout audioElementlayout_;
};

/**
 * @brief To be consistent with other rendering objects, when the input layout
 * is the same as the playback layout input channels are copied to the output.
 * E.g. rendering a stereo bed from a stereo input layout does an implicit copy,
 * this mimics that behavior.
 *
 */
class BinauralCopyRdr : public Renderer {
 public:
  void render(const juce::AudioBuffer<float>& inputBuffer,
              juce::AudioBuffer<float>& outputBuffer) override {
    for (int i = 0; i < Speakers::kBinaural.getNumChannels(); ++i) {
      outputBuffer.copyFrom(i, 0, inputBuffer, i, 0,
                            inputBuffer.getNumSamples());
    }
  }
};