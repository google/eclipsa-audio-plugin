/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License,
 * Version 2.0 (the "License");
 * you may not use this file except in
 * compliance with the License.
 * You may obtain a copy of the License at
 *
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by
 * applicable law or agreed to in writing, software
 * distributed under the
 * License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the
 * specific language governing permissions and
 * limitations under the
 * License.
 */

#pragma once

#include <data_structures/src/ChannelMonitorData.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "../../data_repository/implementation/MixPresentationRepository.h"
#include "../processor_base/ProcessorBase.h"

//==============================================================================
class ChannelMonitorProcessor final : public ProcessorBase {
 public:
  ChannelMonitorProcessor(ChannelMonitorData& channelMonitorData);
  ~ChannelMonitorProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  bool hasEditor() const override;

  const juce::String getName() const override;

  std::vector<float> getPrerdrLoudness() const { return loudness_; }

 private:
  ChannelMonitorData& channelMonitorData_;
  int numChannels_;
  // replace with thread safe data-struct
  std::vector<float> loudness_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelMonitorProcessor)
};
