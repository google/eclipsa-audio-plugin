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

#include "../processor_base/ProcessorBase.h"

// this struct strictly serves the purpose of improving readability
// the source channel is the channel that is being remapped
// the target channel is the channel that is being remapped to
struct RemappingITUChannelPairs {
  RemappingITUChannelPairs(const int& sourceChannel, const int& targetChannel)
      : sourceChannel(sourceChannel), targetChannel(targetChannel) {}
  RemappingITUChannelPairs(const std::pair<int, int>& channelPair)
      : sourceChannel(channelPair.first), targetChannel(channelPair.second) {}
  int sourceChannel;
  int targetChannel;

  // Equality: both members must match
  constexpr bool operator==(
      const RemappingITUChannelPairs& other) const noexcept {
    return sourceChannel == other.sourceChannel &&
           targetChannel == other.targetChannel;
  }

  // (Optional) inequality for free
  constexpr bool operator!=(
      const RemappingITUChannelPairs& other) const noexcept {
    return !(*this == other);
  }
};

using PassthroughRemapTable = std::vector<RemappingITUChannelPairs>;
class RemappingProcessor final : public ProcessorBase,
                                 juce::ValueTree::Listener {
 public:
  //==============================================================================
  // used in AudioElementPlugin
  RemappingProcessor(ProcessorBase* hostProcessor, const bool handleOutputBus);
  ~RemappingProcessor() override;

  //==============================================================================
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;

  //==============================================================================
  const juce::String getName() { return "Remapping"; }

  const PassthroughRemapTable getRemapTable() const { return remapTable_; }

 private:
  void remapBuffer(juce::AudioBuffer<float>& buffer);

  const juce::AudioBuffer<float> createBufferCopy(
      const juce::AudioBuffer<float>& buffer) const;

  PassthroughRemapTable constructRemapTable(
      const juce::Array<juce::AudioChannelSet::ChannelType>& sourceChannels,
      const juce::Array<juce::AudioChannelSet::ChannelType>& targetChannels);
  PassthroughRemapTable remapTable_;
  ProcessorBase* hostProcessor_;
  const bool kHandleOutputBus_;
};