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

#include "../gain/GainProcessor.h"

#include <gtest/gtest.h>

#include "../processor_base/ProcessorBase.h"

void ensureGainsStoredandUpdated() {
  MultiChannelRepository gainRepository = juce::ValueTree("multichannelGains");

  GainProcessor gainProcessor(&gainRepository);

  // Use the actual channel count from the processor instead of hardcoded 28
  const int actualChannelCount = gainProcessor.getGainRepoInputChannels();

  // establish the test gain values that will be used to update the
  std::vector<float> test_gains(actualChannelCount, 1.2f);

  ChannelGains channelGains = gainRepository.get();
  channelGains.setGains(test_gains);

  gainRepository.update(channelGains);
  juce::Logger::outputDebugString(
      "After set update: " + gainRepository.get().toValueTree().toXmlString());

  // Validate the GainProcessor is allocating room for the expected channels
  ASSERT_GE(gainProcessor.getGainRepoInputChannels(), actualChannelCount);
  ASSERT_GE(gainProcessor.getGains().size(), actualChannelCount);

  // validate that the gains have the correct values
  for (int i = 0; i < actualChannelCount; i++) {
    ASSERT_EQ(gainProcessor.getGains()[i]->get(), test_gains[i]);
  }

  // Check if the gains are being applied correctly
  // Create an AudioBuffer to test the processBlock function
  juce::AudioBuffer<float> testDataBuffer(actualChannelCount, 24);

  for (int i = 0; i < actualChannelCount; i++) {
    for (int j = 0; j < 24; j++) {
      testDataBuffer.setSample(i, j, 0.5f);
    }
  }

  juce::MidiBuffer midiBuffer;

  // Now process the buffer
  gainProcessor.prepareToPlay(2, 24);
  gainProcessor.processBlock(testDataBuffer, midiBuffer);

  for (int i = 0; i < actualChannelCount; i++) {
    for (int j = 0; j < 24; j++) {
      ASSERT_EQ(testDataBuffer.getSample(i, j),
                0.6f);  // samples should be 0.5*1.2 = 0.6
    }
  }

  // Reset the gains
  gainProcessor.ResetGains();

  for (int i = 0; i < actualChannelCount; i++) {
    ASSERT_EQ(gainProcessor.getGains()[i]->get(), 1.f);
  }
}

TEST(test_gain_processor, test_listener) { ensureGainsStoredandUpdated(); }

void ensureMuteToggleisFunctional() {
  // initialize an empty tree
  MultiChannelRepository gainsRepository = juce::ValueTree("multichannelGains");
  // use the empty repository to create a GainProcessor
  GainProcessor gainProcessor(&gainsRepository);

  // Use the actual channel count from the processor
  const int actualChannelCount = gainProcessor.getGainRepoInputChannels();

  std::vector<float> test_gains(actualChannelCount, 1.5f);  // unmuted channels
  ChannelGains channelGains = gainsRepository.get();
  channelGains.setGains(test_gains);
  gainsRepository.update(channelGains);

  // mute channel 0 and 5 (if they exist)
  gainProcessor.toggleChannelMute(0);
  if (actualChannelCount > 5) {
    gainProcessor.toggleChannelMute(5);
  }

  // now check if gains have updated in the GainProcessor
  // only channel 0 and 5 (if it exists) should have a gain of 0.0f
  for (int i = 0; i < actualChannelCount; i++) {
    if (i == 0 || (i == 5 && actualChannelCount > 5)) {
      ASSERT_EQ(gainProcessor.getGains()[i]->get(), 0.0f);
    } else {
      ASSERT_EQ(gainProcessor.getGains()[i]->get(), test_gains[i]);
    }
  }

  // Check if the gains are being applied correctly
  // Create an AudioBuffer to test the processBlock function
  juce::AudioBuffer<float> testDataBuffer(actualChannelCount, 24);

  for (int i = 0; i < actualChannelCount; i++) {
    for (int j = 0; j < 24; j++) {
      testDataBuffer.setSample(i, j, 0.5f);
    }
  }

  juce::MidiBuffer midiBuffer;

  // Now process the buffer
  gainProcessor.prepareToPlay(2, 24);
  gainProcessor.processBlock(testDataBuffer, midiBuffer);

  for (int i = 0; i < actualChannelCount; i++) {
    for (int j = 0; j < 24; j++) {
      if (i == 0 || (i == 5 && actualChannelCount > 5)) {
        ASSERT_EQ(testDataBuffer.getSample(i, j), 0.0f);
      } else {
        ASSERT_EQ(testDataBuffer.getSample(i, j), 0.75f);  // 0.5*1.5 = 0.75
      }
    }
  }
}

TEST(test_gain_processor, test_mute_toggle) { ensureMuteToggleisFunctional(); }