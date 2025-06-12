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

#include <gtest/gtest.h>

#include "data_repository/implementation/AudioElementRepository.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/AudioElementPluginSyncClient.h"
#include "data_structures/src/AudioElementSpatialLayout.h"
#include "processors/processor_base/ProcessorBase.h"
#include "processors/routing/RoutingProcessor.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class TestAudioElementPluginSyncClient : public AudioElementPluginSyncClient {
 public:
  TestAudioElementPluginSyncClient(
      AudioElementSpatialLayoutRepository* audioElementSpatialLayoutRepository,
      int port)
      : AudioElementPluginSyncClient(audioElementSpatialLayoutRepository,
                                     port) {}

  void setAudioElementRepositoryForTesting(
      AudioElementRepository& audioElementRepository) {
    rendererAudioElements_ = audioElementRepository;
  }
};

TEST(test_routing_processor, test_no_shift) {
  // Create an audio element repository
  // Create an audio element
  AudioElement audioElement(juce::Uuid(), "Test", Speakers::kStereo, 0);
  AudioElementRepository audio_element_repository(juce::ValueTree{"test"});
  audio_element_repository.add(audioElement);

  // Create a AudioElementSpatialLayout repository
  AudioElementSpatialLayoutRepository audioElementSpatialLayout_repository(
      juce::ValueTree{"audioElementSpatialLayout_test"});
  AudioElementSpatialLayout val = audioElementSpatialLayout_repository.get();
  val.setAudioElementId(audioElement.getId());
  val.setFirstChannel(0);
  val.setLayout(audioElement.getChannelConfig());
  val.setName("TestAudioElementSpatialLayout");
  audioElementSpatialLayout_repository.update(val);

  // Attach it to a sync processor
  TestAudioElementPluginSyncClient sync_client(
      &audioElementSpatialLayout_repository, 0);
  sync_client.setAudioElementRepositoryForTesting(audio_element_repository);

  // Create an instance of the routing processor
  RoutingProcessor routing_processor(&audioElementSpatialLayout_repository,
                                     &sync_client, 36);

  // Create an instance of the audio buffer with 10 channels, filling the first
  // two with 1's and the rest with 0's
  juce::AudioBuffer<float> audio_buffer(10, 10);
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      if (i < 2) {
        audio_buffer.setSample(i, j, 1.0f);
      } else {
        audio_buffer.setSample(i, j, 0.0f);
      }
    }
  }

  // Process the audio buffer
  juce::MidiBuffer midi_buffer;
  routing_processor.setChannelLayoutOfBus(false, 0,
                                          juce::AudioChannelSet::ambisonic(5));
  routing_processor.prepareToPlay(10, 10);
  routing_processor.processBlock(audio_buffer, midi_buffer);

  // Verify nothing has changed
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      if (i < 2) {
        ASSERT_EQ(audio_buffer.getSample(i, j), 1.0f);
      } else {
        ASSERT_EQ(audio_buffer.getSample(i, j), 0.0f);
      }
    }
  }
};

TEST(test_routing_processor, test_partial_shift) {
  // Create an audio element repository
  // Create an audio element with a first channel at 1
  AudioElement audioElement(juce::Uuid(), "Test", Speakers::kStereo, 1);
  AudioElementRepository audio_element_repository(juce::ValueTree{"test"});
  audio_element_repository.add(audioElement);

  // Create a AudioElementSpatialLayout repository
  AudioElementSpatialLayoutRepository audioElementSpatialLayout_repository(
      juce::ValueTree{"audioElementSpatialLayout_test"});
  AudioElementSpatialLayout val = audioElementSpatialLayout_repository.get();
  val.setAudioElementId(audioElement.getId());
  val.setFirstChannel(0);
  val.setLayout(audioElement.getChannelConfig());
  val.setName("TestAudioElementSpatialLayout");
  audioElementSpatialLayout_repository.update(val);

  // Attach it to a sync processor
  TestAudioElementPluginSyncClient sync_client(
      &audioElementSpatialLayout_repository, 0);
  sync_client.setAudioElementRepositoryForTesting(audio_element_repository);

  // Create an instance of the routing processor
  RoutingProcessor routing_processor(&audioElementSpatialLayout_repository,
                                     &sync_client, 36);

  // Create an instance of the audio buffer with 10 channels, filling the first
  // two with 1's and 2's and the rest with 0's
  juce::AudioBuffer<float> audio_buffer(10, 10);
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      if (i < 2) {
        audio_buffer.setSample(i, j, i + 1);
      } else {
        audio_buffer.setSample(i, j, 0.0f);
      }
    }
  }

  // Process the audio buffer
  routing_processor.setChannelLayoutOfBus(false, 0,
                                          juce::AudioChannelSet::ambisonic(5));
  routing_processor.prepareToPlay(10, 10);
  juce::MidiBuffer midi_buffer;
  routing_processor.processBlock(audio_buffer, midi_buffer);

  // Verify the 1's and 2's have shifted one channel higher
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      if (i == 0) {
        ASSERT_EQ(audio_buffer.getSample(i, j), 0.0f);
      } else if (i < 3) {
        ASSERT_EQ(audio_buffer.getSample(i, j), i);
      } else {
        ASSERT_EQ(audio_buffer.getSample(i, j), 0.0f);
      }
    }
  }
}

TEST(test_routing_processor, test_full_shift) {
  // Create an audio element repository
  // Create an audio element
  AudioElement audioElement(juce::Uuid(), "Test", Speakers::kStereo, 2);
  AudioElementRepository audio_element_repository(juce::ValueTree{"test"});
  audio_element_repository.add(audioElement);

  // Create a AudioElementSpatialLayout repository
  AudioElementSpatialLayoutRepository audioElementSpatialLayout_repository(
      juce::ValueTree{"audioElementSpatialLayout_test"});
  AudioElementSpatialLayout val = audioElementSpatialLayout_repository.get();
  val.setAudioElementId(audioElement.getId());
  val.setFirstChannel(0);
  val.setLayout(audioElement.getChannelConfig());
  val.setName("TestaudioElementSpatialLayout");
  audioElementSpatialLayout_repository.update(val);

  // Attach it to a sync processor
  TestAudioElementPluginSyncClient sync_client(
      &audioElementSpatialLayout_repository, 0);
  sync_client.setAudioElementRepositoryForTesting(audio_element_repository);

  // Create an instance of the routing processor
  RoutingProcessor routing_processor(&audioElementSpatialLayout_repository,
                                     &sync_client, 36);

  // Create an instance of the audio buffer with 10 channels, filling the first
  // two with 1's and the rest with 0's
  juce::AudioBuffer<float> audio_buffer(10, 10);
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      if (i < 2) {
        audio_buffer.setSample(i, j, 1.0f);
      } else {
        audio_buffer.setSample(i, j, 0.0f);
      }
    }
  }

  // Process the audio buffer
  routing_processor.setChannelLayoutOfBus(false, 0,
                                          juce::AudioChannelSet::ambisonic(5));
  routing_processor.prepareToPlay(10, 10);
  juce::MidiBuffer midi_buffer;
  routing_processor.processBlock(audio_buffer, midi_buffer);

  // Verify everything has been shifted 2 channels
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      if (i < 2) {
        ASSERT_EQ(audio_buffer.getSample(i, j), 0.0f);
      } else if (i < 4) {
        ASSERT_EQ(audio_buffer.getSample(i, j), 1.0f);
      } else {
        ASSERT_EQ(audio_buffer.getSample(i, j), 0.0f);
      }
    }
  }
}