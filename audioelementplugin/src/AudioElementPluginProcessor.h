/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License,
 * Version 2.0 (the "License");
 * you may not use this file except in
 * compliance with the License.
 * You may obtain a copy of the License at
 *
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by
 * applicable law or agreed to in writing, software
 * distributed under the
 * License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the
 * specific language governing permissions and
 * limitations under the
 * License.
 */

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <processors/processors.h>

#include <thread>
#include <vector>

#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_repository/implementation/MSPlaybackRepository.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/AudioElementCommunication.h"
#include "data_structures/src/AudioElementParameterTree.h"
#include "data_structures/src/Elevation.h"
#include "data_structures/src/ParameterMetaData.h"
#include "data_structures/src/SpeakerMonitorData.h"
#include "logger/logger.h"

struct AudioElementPluginRepositoryCollection {
  AudioElementSpatialLayoutRepository& audioElementSpatialLayoutRepository_;
  MSPlaybackRepository& msRespository_;
  SpeakerMonitorData& monitorData_;
  AmbisonicsData& ambisonicsData_;
};

class AudioElementPluginProcessor final : public ProcessorBase,
                                          juce::ValueTree::Listener {
 public:
  // Import TrackProperties from the base AudioProcessor class
  using TrackProperties = juce::AudioProcessor::TrackProperties;

  AudioElementPluginProcessor();

  static int instanceId_;  // Unique identifier for each instance of the plugin

  void releaseResources() override;
  ~AudioElementPluginProcessor() override { syncClient_.disconnectClient(); }

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  bool applyBusLayouts(const BusesLayout& layouts) override;

  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  void updateTrackProperties(const TrackProperties& properties) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override {
    return "Eclipsa Audio Element Plugin";
  }

  void setOutputChannels(int firstChannel, int totalChannels);

  AudioElementPluginSyncClient& getSyncClient() { return syncClient_; }
  AudioElementPluginRepositoryCollection getRepositories() {
    return {audioElementSpatialLayoutRepository_, msRespository_, monitorData_,
            ambisonicsData_};
  }
  AudioElementParameterTree automationParametersTreeState;

  inline static const ::juce::Identifier
      kAudioElementSpatialLayoutRepositoryStateKey{
          "audio_element_spatial_layout_repository_state"};

 private:
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  std::vector<std::unique_ptr<ProcessorBase>> audioProcessors_;
  ElevationListener elevationListener_;

  /*
  State Information
  */
  juce::ValueTree persistentState_;
  inline static const juce::Identifier kAudioElementSpatialPluginStateKey{
      "audio_element_plugin_renderer_state"};

  AudioElementSpatialLayoutRepository audioElementSpatialLayoutRepository_;

  MSPlaybackRepository msRespository_;
  inline static const ::juce::Identifier kMSPlaybackRepositoryStateKey{
      "ms_playback_repository_state"};
  /*
  Local Information
  */
  int firstOutputChannel;  // This determines the first channel to output to.
                           // Subsequent channels are output to in order up
                           // from this first channel
  int outputChannelCount;  // Replace with a real speaker layout later
  juce::AudioChannelSet lastOutputChannelSet_ = juce::AudioChannelSet::mono();
  bool allowDownSizing_ = false;

  AudioElementPluginSyncClient syncClient_;
  SpeakerMonitorData monitorData_;
  AmbisonicsData ambisonicsData_;  // intiialized in SoundFieldProcessor

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioElementPluginProcessor)
};