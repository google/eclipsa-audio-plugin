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

#include "AudioPanner.h"
#include "ambisonic_encoder.h"  // OBR Ambisonic Encoder implementation.
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class AmbisonicPanner : public AudioPanner {
 public:
  AmbisonicPanner(const Speakers::AudioElementSpeakerLayout pannedLayout,
                  const int samplesPerBlock, const int sampleRate);

  ~AmbisonicPanner();

  /**
   * @brief Applies spatial information to the given input audio and writes the
   *        output buffer.
   * @pre Input buffer must have the same number of channels as the input
   * layout.
   * @pre Input buffer must have the same number of samples as the panner was
   *      constructed with.
   * @param inputBuffer
   * @param outputBuffer
   */
  void process(juce::AudioBuffer<float>& inputBuffer,
               juce::AudioBuffer<float>& outputBuffer) override;

  void setPolarPosition(const ear::PolarPosition& position);

 protected:
  void positionUpdated() override;

 private:
  void applyPosition();

  obr::AudioBuffer inputBufferPlanar_, outputBufferPlanar_, movedBufferPlanar_;
  std::unique_ptr<obr::AmbisonicEncoder> encoder_;
  // A position set after the first block is crossfaded in by the next one.
  bool hasRendered_ = false;
  bool positionPending_ = false;
};