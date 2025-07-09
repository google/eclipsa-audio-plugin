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

#include "../channel_monitor/ChannelMonitorProcessor.h"

#include <gtest/gtest.h>

#include "data_structures/src/ChannelMonitorData.h"
#include "data_structures/src/LanguageCodeMetaData.h"
#include "data_structures/src/MixPresentation.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

TEST(test_channelmonitor_processor, test_getPrerdrLoudness) {
  ChannelMonitorData channelMonitorData;

  MixPresentationRepository mixPresentationRepository_ =
      juce::ValueTree("mixPresentation");

  MixPresentationSoloMuteRepository mixPresentationSoloMuteRepository_ =
      juce::ValueTree("mixPresentationSoloMute");

  // temporary hard-code for testing purposes
  juce::Uuid presentationUuid = juce::Uuid();
  MixPresentation presentation(presentationUuid, "English Mix", 1,
                               LanguageData::MixLanguages::English, {});

  ChannelMonitorProcessor channelMonitorProcessor(
      channelMonitorData, &mixPresentationRepository_,
      &mixPresentationSoloMuteRepository_);

  // Check if the gains are being applied correctly
  // Create an AudioBuffer to test the processBlock function
  int numSamples = 24;
  juce::AudioBuffer<float> testDataBuffer(28, numSamples);

  for (int i = 0; i < 28; i++) {
    for (int j = 0; j < numSamples; j++) {
      testDataBuffer.setSample(i, j, 0.5f);
    }
  }

  juce::MidiBuffer midiBuffer;

  // Now process the buffer
  channelMonitorProcessor.prepareToPlay(2, numSamples);
  channelMonitorProcessor.processBlock(testDataBuffer, midiBuffer);

  std::vector<float> channelLoudnessesRead;
  channelMonitorData.channelLoudnesses.read(channelLoudnessesRead);

  for (int i = 0; i < 28; i++) {
    // Channels with value 0.5 have a rough dB value of -6
    ASSERT_NEAR(channelLoudnessesRead[i], -6.0f, 0.1);
  }
}