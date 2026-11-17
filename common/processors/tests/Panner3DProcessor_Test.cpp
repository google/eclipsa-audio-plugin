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

#include <algorithm>
#include <cmath>
#include <vector>

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

namespace {
struct DiffsPerChannel {
  std::vector<float> steady;
  std::vector<float> moving;
};

// Pans a constant sine to `outputLayout` in 1024-sample blocks, moving the
// source from front-left to rear-right between two blocks. Returns each output
// channel's largest sample-to-sample difference while the source holds still
// at either end, and across the move and the block after it.
DiffsPerChannel measurePositionChange(
    const Speakers::AudioElementSpeakerLayout outputLayout) {
  constexpr int kBlockSize = 1024;
  constexpr int kSampleRate = 48000;
  constexpr float kFrequency = 440.f;
  constexpr int kSettleBlocks = 8;

  AudioElement audioElement(juce::Uuid(), "Test", Speakers::kMono, 0);
  AudioElementRepository audioElementRepository(juce::ValueTree{"test"});
  audioElementRepository.add(audioElement);

  AudioElementSpatialLayoutRepository spatialLayoutRepository(
      juce::ValueTree{"audioElementSpatialLayout_test"});
  AudioElementSpatialLayout spatialLayout = spatialLayoutRepository.get();
  spatialLayout.setAudioElementId(audioElement.getId());
  spatialLayout.setFirstChannel(0);
  spatialLayout.setLayout(outputLayout);
  spatialLayout.setPanningEnabled(true);
  spatialLayoutRepository.update(spatialLayout);

  TestAudioElementPluginSyncClient syncClient(&spatialLayoutRepository, 0);
  syncClient.setAudioElementRepositoryForTesting(audioElementRepository);
  RoutingProcessor routingProcessor(&spatialLayoutRepository, &syncClient, 36);
  AudioElementParameterTree parameterTree(routingProcessor);
  DummyHostProcessor hostProcessor;

  Panner3DProcessor panner(&hostProcessor, &spatialLayoutRepository,
                           &parameterTree);
  const auto moveTo = [&panner](float x, float y) {
    panner.parameterChanged(AutoParamMetaData::xPosition, x);
    panner.parameterChanged(AutoParamMetaData::yPosition, y);
    panner.parameterChanged(AutoParamMetaData::zPosition, 0.f);
  };
  moveTo(-50.f, 50.f);
  panner.prepareToPlay(kSampleRate, kBlockSize);

  const int numChannels = outputLayout.getNumChannels();
  juce::AudioBuffer<float> buffer(numChannels, kBlockSize);
  juce::MidiBuffer midi;
  std::vector<float> lastSample(numChannels, 0.f);
  long long sampleIndex = 0;

  // Renders one block and returns each channel's largest step, counting the
  // step from the previous block's last sample.
  const auto renderBlock = [&]() {
    buffer.clear();
    for (int i = 0; i < kBlockSize; ++i, ++sampleIndex) {
      buffer.setSample(0, i,
                       0.5f * std::sin(2.f * juce::MathConstants<float>::pi *
                                       kFrequency * sampleIndex / kSampleRate));
    }
    panner.processBlock(buffer, midi);

    std::vector<float> maxStep(numChannels, 0.f);
    for (int ch = 0; ch < numChannels; ++ch) {
      float previous = lastSample[ch];
      for (int i = 0; i < kBlockSize; ++i) {
        const float sample = buffer.getSample(ch, i);
        maxStep[ch] = std::max(maxStep[ch], std::abs(sample - previous));
        previous = sample;
      }
      lastSample[ch] = previous;
    }
    return maxStep;
  };

  DiffsPerChannel diffs{std::vector<float>(numChannels, 0.f),
                        std::vector<float>(numChannels, 0.f)};
  const auto accumulate = [](std::vector<float>& into,
                             const std::vector<float>& from) {
    for (size_t ch = 0; ch < into.size(); ++ch) {
      into[ch] = std::max(into[ch], from[ch]);
    }
  };

  for (int block = 0; block < kSettleBlocks; ++block) {
    renderBlock();
  }
  accumulate(diffs.steady, renderBlock());

  moveTo(50.f, -50.f);
  accumulate(diffs.moving, renderBlock());
  accumulate(diffs.moving, renderBlock());

  for (int block = 0; block < kSettleBlocks; ++block) {
    renderBlock();
  }
  accumulate(diffs.steady, renderBlock());
  return diffs;
}

void expectSmoothPositionChange(
    const Speakers::AudioElementSpeakerLayout outputLayout) {
  const DiffsPerChannel diffs = measurePositionChange(outputLayout);
  for (size_t ch = 0; ch < diffs.steady.size(); ++ch) {
    EXPECT_LE(diffs.moving[ch], 2.f * diffs.steady[ch] + 1e-6f)
        << "channel " << ch << " steps by " << diffs.moving[ch]
        << " while moving, " << diffs.steady[ch] << " in steady state";
  }
}
}  // namespace

// Moving the source does not click on a 5.1 speaker bed.
TEST(test_3D_render_Processor, position_change_is_smooth_5_1) {
  expectSmoothPositionChange(Speakers::k5Point1);
}

// Moving the source does not click on a 7.1.4 speaker bed.
TEST(test_3D_render_Processor, position_change_is_smooth_7_1_4) {
  expectSmoothPositionChange(Speakers::k7Point1Point4);
}

// Moving the source does not click in third-order ambisonics.
TEST(test_3D_render_Processor,
     position_change_is_smooth_third_order_ambisonics) {
  expectSmoothPositionChange(Speakers::kHOA3);
}

// Moving the source does not click in binaural output.
TEST(test_3D_render_Processor, position_change_is_smooth_binaural) {
  expectSmoothPositionChange(Speakers::kBinaural);
}
