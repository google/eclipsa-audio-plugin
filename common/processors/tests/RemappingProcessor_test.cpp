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

#include "../remapping/RemappingProcessor.h"

#include <gtest/gtest.h>

#include "substream_rdr/substream_rdr_utils/Speakers.h"

// Dummy processor to simulate a host processor.
class DummyHostProcessor final : public ProcessorBase {
 public:
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {
    // Provide a dummy implementation
  }

  static const PassthroughRemapTable kRemapTable7Point1Point4;
  static const PassthroughRemapTable kNoRemap;
};

// this remap table was validated through manual testing
const PassthroughRemapTable DummyHostProcessor::kRemapTable7Point1Point4{
    {6, 8}, {7, 9}, {8, 10}, {9, 11}, {10, 6}, {11, 7}};
const PassthroughRemapTable DummyHostProcessor::kNoRemap{};

std::pair<bool, bool> checkRemapTable(DummyHostProcessor& hostProcessor,
                                      const juce::AudioChannelSet& busLayout) {
  // Set the buses layout for the host processor
  juce::AudioProcessor::BusesLayout busesLayout;
  busesLayout.inputBuses.add(busLayout);
  busesLayout.outputBuses.add(busLayout);
  hostProcessor.setBusesLayout(busesLayout);
  // Create a RemappingProcessor instance
  RemappingProcessor remappingProcessor(&hostProcessor, false);

  // call prepareToPlay to initialize the remap table
  remappingProcessor.prepareToPlay(44100.0, 512);

  const PassthroughRemapTable remapTable = remappingProcessor.getRemapTable();
  PassthroughRemapTable hostRemapTable;
  if (busLayout == juce::AudioChannelSet::create7point1point4()) {
    hostRemapTable = DummyHostProcessor::kRemapTable7Point1Point4;
  } else {
    hostRemapTable = DummyHostProcessor::kNoRemap;
  }

  // The remap tables may be out of order, so ensure each pair exists in the
  // other table and that they are the same size
  bool sameSize = hostRemapTable.size() == remapTable.size();
  bool allMatched = true;
  for (size_t i = 0; i < remapTable.size(); ++i) {
    if (!allMatched) {
      break;
    }
    bool matchFound = false;
    for (size_t j = 0; j < remapTable.size(); ++j) {
      // if you find a match, break
      if (remapTable[i] == hostRemapTable[j]) {
        matchFound = true;
        break;
      }
    }
    // if you did not find a match, set allMatched to false
    if (!matchFound) allMatched = false;
  }
  return {sameSize, allMatched};
}

TEST(test_remapping_processor, test_remapping_tables) {
  const std::array<juce::AudioChannelSet, 6> channelSets = {
      juce::AudioChannelSet::create7point1point4(),
      juce::AudioChannelSet::create7point1(),
      juce::AudioChannelSet::create5point1point4(),
      juce::AudioChannelSet::create5point1point2(),
      juce::AudioChannelSet::create5point1(),
      juce::AudioChannelSet::stereo()};
  for (const auto& channelSet : channelSets) {
    // Create a dummy host processor
    DummyHostProcessor hostProcessor;
    // Check the remap table for each channel set
    const auto [sameSize, allEqual] =
        checkRemapTable(hostProcessor, channelSet);

    EXPECT_TRUE(sameSize);
    EXPECT_TRUE(allEqual);
  }
}

void testBufferRemap(DummyHostProcessor& hostProcessor,
                     const bool handleOutputBus) {
  juce::AudioProcessor::BusesLayout busesLayout;
  busesLayout.inputBuses.add(juce::AudioChannelSet::create7point1point4());
  busesLayout.outputBuses.add(juce::AudioChannelSet::create7point1point4());
  hostProcessor.setBusesLayout(busesLayout);
  // Create a RemappingProcessor instance
  RemappingProcessor remappingProcessor(&hostProcessor, handleOutputBus);

  const int samplesPerBlock = 2;

  // Prepare the processor
  remappingProcessor.prepareToPlay(44100.0, samplesPerBlock);

  const PassthroughRemapTable remapTable = remappingProcessor.getRemapTable();

  // Create a buffer to test the remapping
  // since we're only testing the remapping of channels,
  // the number size of the buffer does not matter
  juce::AudioBuffer<float> buffer(Speakers::k7Point1Point4.getNumChannels(),
                                  samplesPerBlock);
  buffer.clear();

  // Fill the sourceChannels values w/ samples equal to the channel number
  // all targetChannels should remain 0
  for (const auto& channelPair : remapTable) {
    const int channel = channelPair.sourceChannel;
    const float value = static_cast<float>(channel);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
      buffer.setSample(channel, sample, value);
    }
  }

  // Process the block
  // the targetChannels should be filled with the sourceChannel values
  juce::MidiBuffer midiBuffer;
  remappingProcessor.processBlock(buffer, midiBuffer);

  // Check the output buffer for expected values
  for (const auto& channelPair : remapTable) {
    const int targetChannel = channelPair.targetChannel;
    // the values will be the same as the source channel number
    const float value = channelPair.sourceChannel;
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
      EXPECT_EQ(buffer.getSample(targetChannel, sample), value);
    }
  }
}

TEST(test_remapping_processor, test_input_buffer) {
  // Create a dummy host processor
  DummyHostProcessor hostProcessor;

  testBufferRemap(hostProcessor, false);
}

TEST(test_remapping_processor, test_output_buffer) {
  // Create a dummy host processor
  DummyHostProcessor hostProcessor;

  testBufferRemap(hostProcessor, true);
}
