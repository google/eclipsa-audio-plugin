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

#include "processors/panner/Panner3DProcessor.h"

#include <gtest/gtest.h>

#include "../audioelementplugin/src/AudioElementPluginProcessor.h"
#include "data_repository/implementation/AudioElementRepository.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/AudioElementPluginSyncClient.h"
#include "data_structures/src/AudioElementSpatialLayout.h"
#include "processors/processor_base/ProcessorBase.h"
#include "processors/routing/RoutingProcessor.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

// Dummy processor to simulate a host processor.
class DummyHostProcessor final : public ProcessorBase {
 public:
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {
    // Provide a dummy implementation
  }
};

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

TEST(test_3D_render_Processor, test_simple_pan) {
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

  // Create a routing processor
  RoutingProcessor routing_processor(&audioElementSpatialLayout_repository,
                                     &sync_client, 36);

  // Create a parameter tree
  AudioElementParameterTree parameter_tree(routing_processor);

  DummyHostProcessor baseProcessor;

  // Create an instance of the 3d Panner
  Panner3DProcessor processor(
      &baseProcessor, &audioElementSpatialLayout_repository, &parameter_tree);

  // Create an instance of the audio buffer with 2 channels, filling the first
  // two with 1's and the rest with 0's
  juce::AudioBuffer<float> audio_buffer(2, 10);
  for (int ch = 0; ch < audio_buffer.getNumChannels(); ++ch) {
    for (int i = 0; i < 10; i++) {
      if (i < 2) {
        audio_buffer.setSample(ch, i, 1.0f);
      } else {
        audio_buffer.setSample(ch, i, 0.0f);
      }
    }
  }

  // Process the audio buffer
  juce::MidiBuffer midi_buffer;
  processor.setChannelLayoutOfBus(false, 0,
                                  juce::AudioChannelSet::ambisonic(5));
  processor.prepareToPlay(10, 10);
  processor.processBlock(audio_buffer, midi_buffer);
};
