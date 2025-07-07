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

#include "ChannelMonitorProcessor.h"

#include "data_structures/src/ChannelMonitorData.h"

ChannelMonitorProcessor::ChannelMonitorProcessor(
    ChannelMonitorData& channelMonitorData)
    : numChannels_(juce::AudioChannelSet::ambisonic(5).size()),
      channelMonitorData_(channelMonitorData),
      loudness_(std::vector<float>(numChannels_, -300.f)) {}

ChannelMonitorProcessor::~ChannelMonitorProcessor() {}

const juce::String ChannelMonitorProcessor::getName() const {
  return {"Channel Monitor"};
}

void ChannelMonitorProcessor::prepareToPlay(double sampleRate,
                                            int samplesPerBlock) {}

void ChannelMonitorProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;

  for (int i = 0; i < buffer.getNumChannels(); i++) {
    loudness_[i] =
        20.0f * std::log10(buffer.getRMSLevel(i, 0, buffer.getNumSamples()));
  }

  for (int i = buffer.getNumChannels(); i < numChannels_; i++) {
    loudness_[i] = -120.0f;
  }

  channelMonitorData_.channelLoudnesses.update(loudness_);
}

bool ChannelMonitorProcessor::hasEditor() const {
  return false;  // (change this to false if you choose to not supply an editor)
}
