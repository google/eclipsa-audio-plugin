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
#include <juce_audio_basics/juce_audio_basics.h>

#include "AmbisonicPanner.h"
#include "AudioPanner.h"
#include "renderer/obr_impl.h"

class BinauralPanner : public AudioPanner {
 public:
  BinauralPanner(const int samplesPerBlock, const int sampleRate);

  ~BinauralPanner();

  /**
   * @brief Applies spatial information to the given input audio and writes the
   *        output buffer.
   * @pre Input buffer must have the same number of channels as the input
   * layout.
   * @pre Input and output buffers must hold at least the number of samples
   *      the panner was constructed with.
   * @param inputBuffer
   * @param outputBuffer
   * @param numSamples Samples to pan, at most the constructed block size.
   */
  void process(juce::AudioBuffer<float>& inputBuffer,
               juce::AudioBuffer<float>& outputBuffer, int numSamples) override;

  int getLatencySamples() const override { return kSamplesPerBlock_; }

 protected:
  void positionUpdated() override;

 private:
  AmbisonicPanner ambisonicPanner_;  // Ramps position changes before decode.
  juce::AudioBuffer<float> ambisonicBuffer_;
  // obr decodes whole blocks only, so samples pass through a one-block FIFO.
  obr::AudioBuffer fifoInputPlanar_, fifoOutputPlanar_;
  int fifoPosition_ = 0;
  std::unique_ptr<obr::ObrImpl> encoder_;
};
