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
#include <memory>
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
constexpr int kPreparedBlockSize = 1024;
constexpr int kSampleRate = 48000;
constexpr float kFrequency = 440.f;

// A Panner3DProcessor panning a mono element to `outputLayout`, prepared with
// kPreparedBlockSize.
struct PannerFixture {
  explicit PannerFixture(const Speakers::AudioElementSpeakerLayout outputLayout)
      : audioElement(juce::Uuid(), "Test", Speakers::kMono, 0),
        audioElementRepository(juce::ValueTree{"test"}),
        spatialLayoutRepository(
            juce::ValueTree{"audioElementSpatialLayout_test"}),
        syncClient(&spatialLayoutRepository, 0),
        routingProcessor(&spatialLayoutRepository, &syncClient, 36),
        parameterTree(routingProcessor) {
    audioElementRepository.add(audioElement);
    AudioElementSpatialLayout spatialLayout = spatialLayoutRepository.get();
    spatialLayout.setAudioElementId(audioElement.getId());
    spatialLayout.setFirstChannel(0);
    spatialLayout.setLayout(outputLayout);
    spatialLayout.setPanningEnabled(true);
    spatialLayoutRepository.update(spatialLayout);
    syncClient.setAudioElementRepositoryForTesting(audioElementRepository);

    panner = std::make_unique<Panner3DProcessor>(
        &hostProcessor, &spatialLayoutRepository, &parameterTree);
  }

  void moveTo(float x, float y) {
    panner->parameterChanged(AutoParamMetaData::xPosition, x);
    panner->parameterChanged(AutoParamMetaData::yPosition, y);
    panner->parameterChanged(AutoParamMetaData::zPosition, 0.f);
  }

  AudioElement audioElement;
  AudioElementRepository audioElementRepository;
  AudioElementSpatialLayoutRepository spatialLayoutRepository;
  TestAudioElementPluginSyncClient syncClient;
  RoutingProcessor routingProcessor;
  AudioElementParameterTree parameterTree;
  DummyHostProcessor hostProcessor;
  std::unique_ptr<Panner3DProcessor> panner;
};

struct PositionChangeMeasurement {
  // Each channel's largest sample-to-sample step while the source holds still.
  std::vector<float> steady;
  // Each channel's largest absolute sample while the source holds still.
  std::vector<float> steadyPeak;
  // Each channel's largest step across the move and the samples after it.
  std::vector<float> moving;
  // Each channel's settled gain against the input at the start and end
  // positions.
  std::vector<float> gainBefore;
  std::vector<float> gainAfter;
};

// Pans a constant sine to `outputLayout` in `hostBlockSize`-sample blocks,
// moving the source from front-left to rear-right between two blocks.
PositionChangeMeasurement measurePositionChange(
    const Speakers::AudioElementSpeakerLayout outputLayout,
    const int hostBlockSize) {
  constexpr int kSettleSamples = 8 * kPreparedBlockSize;
  // Covers the ramp, the binaural FIFO and libspatialaudio's delay line.
  constexpr int kMovingSamples = 3 * kPreparedBlockSize;

  PannerFixture fixture(outputLayout);
  fixture.moveTo(-50.f, 50.f);
  fixture.panner->prepareToPlay(kSampleRate, kPreparedBlockSize);

  const int numChannels = outputLayout.getNumChannels();
  juce::AudioBuffer<float> buffer(numChannels, hostBlockSize);
  juce::MidiBuffer midi;
  std::vector<float> lastSample(numChannels, 0.f);
  long long sampleIndex = 0;

  PositionChangeMeasurement result{std::vector<float>(numChannels, 0.f),
                                   std::vector<float>(numChannels, 0.f),
                                   std::vector<float>(numChannels, 0.f),
                                   std::vector<float>(numChannels, 0.f),
                                   std::vector<float>(numChannels, 0.f)};

  // Renders host blocks until at least `numSamples` have passed, raising
  // `maxStep` and `peak` to each channel's largest step and sample. When
  // `gain` is given, it receives each channel's correlation with the input.
  const auto render = [&](int numSamples, std::vector<float>* maxStep,
                          std::vector<float>* peak, std::vector<float>* gain) {
    std::vector<double> crossSum(numChannels, 0.0);
    double inputSum = 0.0;
    for (int rendered = 0; rendered < numSamples; rendered += hostBlockSize) {
      buffer.clear();
      for (int i = 0; i < hostBlockSize; ++i, ++sampleIndex) {
        buffer.setSample(
            0, i,
            0.5f * std::sin(2.f * juce::MathConstants<float>::pi * kFrequency *
                            sampleIndex / kSampleRate));
      }
      std::vector<float> input(buffer.getReadPointer(0),
                               buffer.getReadPointer(0) + hostBlockSize);
      fixture.panner->processBlock(buffer, midi);

      for (int ch = 0; ch < numChannels; ++ch) {
        float previous = lastSample[ch];
        for (int i = 0; i < hostBlockSize; ++i) {
          const float sample = buffer.getSample(ch, i);
          if (maxStep != nullptr) {
            (*maxStep)[ch] =
                std::max((*maxStep)[ch], std::abs(sample - previous));
          }
          if (peak != nullptr) {
            (*peak)[ch] = std::max((*peak)[ch], std::abs(sample));
          }
          crossSum[ch] += sample * input[i];
          previous = sample;
        }
        lastSample[ch] = previous;
      }
      for (const float sample : input) {
        inputSum += sample * sample;
      }
    }
    if (gain != nullptr) {
      for (int ch = 0; ch < numChannels; ++ch) {
        (*gain)[ch] = static_cast<float>(crossSum[ch] / inputSum);
      }
    }
  };

  render(kSettleSamples, nullptr, nullptr, nullptr);
  render(kPreparedBlockSize, &result.steady, &result.steadyPeak,
         &result.gainBefore);

  fixture.moveTo(50.f, -50.f);
  render(kMovingSamples, &result.moving, nullptr, nullptr);

  render(kSettleSamples, nullptr, nullptr, nullptr);
  render(kPreparedBlockSize, &result.steady, &result.steadyPeak,
         &result.gainAfter);
  return result;
}

void expectSmoothPositionChange(
    const Speakers::AudioElementSpeakerLayout outputLayout,
    const int hostBlockSize = kPreparedBlockSize) {
  const PositionChangeMeasurement m =
      measurePositionChange(outputLayout, hostBlockSize);

  EXPECT_GT(*std::max_element(m.steady.begin(), m.steady.end()), 0.f)
      << "the panner output is silent";
  float largestGainChange = 0.f;
  for (size_t ch = 0; ch < m.gainBefore.size(); ++ch) {
    largestGainChange = std::max(largestGainChange,
                                 std::abs(m.gainAfter[ch] - m.gainBefore[ch]));
  }
  EXPECT_GT(largestGainChange, 0.05f) << "the move never reached the panner";

  // A sine of peak A steps by at most A * 2 * pi * f / fs between samples, so
  // a larger steady step is a discontinuity.
  const float sineStep =
      2.f * juce::MathConstants<float>::pi * kFrequency / kSampleRate;
  for (size_t ch = 0; ch < m.steady.size(); ++ch) {
    EXPECT_LE(m.steady[ch], 1.1f * sineStep * m.steadyPeak[ch] + 1e-6f)
        << "channel " << ch << " steps by " << m.steady[ch]
        << " in steady state with peak " << m.steadyPeak[ch];
    EXPECT_LE(m.moving[ch], 2.f * m.steady[ch] + 1e-6f)
        << "channel " << ch << " steps by " << m.moving[ch] << " while moving, "
        << m.steady[ch] << " in steady state";
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

// Host blocks shorter than the prepared size do not click a 5.1 bed.
TEST(test_3D_render_Processor, short_host_blocks_are_smooth_5_1) {
  expectSmoothPositionChange(Speakers::k5Point1, 300);
}

// Host blocks longer than, and not a multiple of, the prepared size do not
// click a 5.1 bed.
TEST(test_3D_render_Processor, long_host_blocks_are_smooth_5_1) {
  expectSmoothPositionChange(Speakers::k5Point1, 1500);
}

// Host blocks shorter than the prepared size do not click in third-order
// ambisonics.
TEST(test_3D_render_Processor, short_host_blocks_are_smooth_ambisonics) {
  expectSmoothPositionChange(Speakers::kHOA3, 300);
}

// Host blocks longer than, and not a multiple of, the prepared size do not
// click in third-order ambisonics.
TEST(test_3D_render_Processor, long_host_blocks_are_smooth_ambisonics) {
  expectSmoothPositionChange(Speakers::kHOA3, 1500);
}

// Host blocks shorter than the prepared size do not click in binaural output.
TEST(test_3D_render_Processor, short_host_blocks_are_smooth_binaural) {
  expectSmoothPositionChange(Speakers::kBinaural, 300);
}

// Host blocks longer than, and not a multiple of, the prepared size do not
// click in binaural output.
TEST(test_3D_render_Processor, long_host_blocks_are_smooth_binaural) {
  expectSmoothPositionChange(Speakers::kBinaural, 1500);
}

// Binaural panning reports its one-block FIFO to the host as latency.
TEST(test_3D_render_Processor, binaural_reports_latency) {
  PannerFixture fixture(Speakers::kBinaural);
  fixture.panner->prepareToPlay(kSampleRate, kPreparedBlockSize);
  EXPECT_EQ(fixture.hostProcessor.getLatencySamples(), kPreparedBlockSize);
}

// Speaker-bed panning reports no latency to the host.
TEST(test_3D_render_Processor, speaker_bed_reports_no_latency) {
  PannerFixture fixture(Speakers::k5Point1);
  fixture.panner->prepareToPlay(kSampleRate, kPreparedBlockSize);
  EXPECT_EQ(fixture.hostProcessor.getLatencySamples(), 0);
}
