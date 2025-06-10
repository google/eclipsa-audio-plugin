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

#include "RemappingProcessor.h"

#include "../../rendererplugin/src/RendererProcessor.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "logger/logger.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

RemappingProcessor::RemappingProcessor(ProcessorBase* hostProcessor,
                                       const bool handleOutputBus)
    : hostProcessor_(hostProcessor), kHandleOutputBus_(handleOutputBus) {}
RemappingProcessor::~RemappingProcessor() {}

void RemappingProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  // determine remap table
  juce::AudioChannelSet channelSet;
  if (kHandleOutputBus_) {
    channelSet = hostProcessor_->getBusesLayout().getMainOutputChannelSet();
  } else {
    channelSet = hostProcessor_->getBusesLayout().getMainInputChannelSet();
  }

  Speakers::AudioElementSpeakerLayout channelLayout(channelSet);

  juce::Array<juce::AudioChannelSet::ChannelType> expectedChannels =
      channelLayout.getITUChannelOrdering();

  juce::Array<juce::AudioChannelSet::ChannelType> busChannels =
      channelSet.getChannelTypes();

  // if the channel layout is mono, or if the bus channels are the same as the
  // expected channels, then
  // we do not need to remap anything
  // ensure the remapTable_ is empty
  if (expectedChannels == busChannels || channelLayout == Speakers::kMono ||
      expectedChannels.isEmpty()) {
    remapTable_.clear();
  } else if (kHandleOutputBus_) {
    // if the output bus is being handled, then we need to remap the channels
    // to the bus layout
    remapTable_ = constructRemapTable(expectedChannels, busChannels);
  } else {
    remapTable_ = constructRemapTable(busChannels, expectedChannels);
  }
}

void RemappingProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer&) {
  if (!remapTable_.empty()) {
    remapBuffer(buffer);
  }
}

const juce::AudioBuffer<float> RemappingProcessor::createBufferCopy(
    const juce::AudioBuffer<float>& buffer) const {
  // Create a copy of the input buffer
  juce::AudioBuffer<float> copy(buffer.getNumChannels(),
                                buffer.getNumSamples());
  for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
    copy.copyFrom(channel, 0, buffer, channel, 0, buffer.getNumSamples());
  }
  return copy;
}

// converts from protools to the standard layout
void RemappingProcessor::remapBuffer(juce::AudioBuffer<float>& buffer) {
  // Create a copy of the input buffer
  juce::AudioBuffer<float> originalBuffer = createBufferCopy(buffer);

  // iterate through the remap table
  for (const auto& remap : remapTable_) {
    buffer.copyFrom(remap.targetChannel, 0, originalBuffer, remap.sourceChannel,
                    0, originalBuffer.getNumSamples());
  }
}

PassthroughRemapTable RemappingProcessor::constructRemapTable(
    const juce::Array<juce::AudioChannelSet::ChannelType>& sourceChannels,
    const juce::Array<juce::AudioChannelSet::ChannelType>& targetChannels) {
  // Create a passthrough remap table
  PassthroughRemapTable remapTable;
  for (int targetChannel = 0; targetChannel < targetChannels.size();
       ++targetChannel) {
    if (targetChannels[targetChannel] == sourceChannels[targetChannel]) {
      continue;
    }
    int sourceChannel = sourceChannels.indexOf(targetChannels[targetChannel]);
    // if the source channel is not found, then there is a mismatch between the
    // DAW channel layout and the expected ITU channel layout
    if (sourceChannel == -1) {
      LOG_ERROR(RendererProcessor::instanceId_,
                "RemappingProcessor, constructRemapTable: Could not find "
                "source channel for target channel: " +
                    std::to_string(targetChannel));
      continue;
    }
    jassert(sourceChannel != -1);
    remapTable.emplace_back(sourceChannel, targetChannel);
  }

  return remapTable;
}
