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

#include <data_repository/data_repository.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <processors/processors.h>

#include <memory>

#include "RendererPluginSyncServer.h"
#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_repository/implementation/MixPresentationLoudnessRepository.h"
#include "data_repository/implementation/MixPresentationSoloMuteRepository.h"
#include "data_repository/implementation/RoomSetupRepository.h"
#include "data_structures/src/AudioElementCommunication.h"
#include "data_structures/src/ChannelMonitorData.h"
#include "data_structures/src/RepositoryCollection.h"
#include "processors/processor_base/ProcessorBase.h"

//==============================================================================
class RendererProcessor final : public ProcessorBase,
                                public AudioElementPluginUpdateListener,
                                public juce::ValueTree::Listener {
 public:
  //==============================================================================
  RendererProcessor();
  ~RendererProcessor() override;
  const static int instanceId_ =
      0;  // Unique identifier for each instance of the plugin

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  bool applyBusLayouts(const BusesLayout& layouts) override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  void setNonRealtime(bool isNonRealtime) noexcept override;

  //==============================================================================
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  //==============================================================================
  void checkManualOfflineStartStop();
  void valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged) override;
  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override;
  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override;

  //==============================================================================
  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;
  void reinitializeAfterStateRestore() override;

 public:
  juce::AudioProcessorEditor* getFileEditor() const;
  ProcessorBase* getFileOutputProcessor() const;

  RepositoryCollection getRepositories() {
    return {roomSetupRepository_,
            audioElementRepository_,
            multichannelgainRepository_,
            fileExportRepository_,
            mixPresentationRepository_,
            mixPresentationSoloMuteRepository_,
            mixPresentationLoudnessRepository_,
            audioElementSpatialLayoutRepository_,
            msPlaybackRepository_,
            audioElementSubscriber_,
            activeMixPresentationRepository_,
            filePlaybackRepository_};
  }

  RoomSetupRepository& getRoomSetupRepository() { return roomSetupRepository_; }
  SpeakerMonitorData& getSpeakerMonitorData() { return monitorData_; }
  ChannelMonitorData& getChannelMonitorData() { return channelMonitorData_; }

  void updateAudioElementPluginInformation(
      AudioElementSpatialLayout& audioElementSpatialLayout) override {
    audioElementSpatialLayoutRepository_.updateOrAdd(audioElementSpatialLayout);
  }

  void removeAudioElementPlugin(
      AudioElementSpatialLayout& audioElementSpatialLayout) override {
    audioElementSpatialLayoutRepository_.remove(audioElementSpatialLayout);
  }

 private:
  void updateRepositories();

  void initializeMixPresentations();

  void configureOutputBus();

  juce::ValueTree getTreeWithId(const juce::Identifier& id);

  std::vector<std::unique_ptr<ProcessorBase>> audioProcessors_;

  juce::AudioBuffer<float> processingBuffer_;

  AudioElementSubscriber audioElementSubscriber_;

  // This repository should NOT be loaded from file, instead populated
  // by AudioElementSpatialLayout connection callbacks
  MultibaseAudioElementSpatialLayoutRepository
      audioElementSpatialLayoutRepository_;

  juce::ValueTree persistentState_;
  inline static const juce::Identifier kRendererStateKey{"re_state"};

  RoomSetupRepository roomSetupRepository_;
  inline static const juce::Identifier kRoomSetupKey{"room_setup"};

  AudioElementRepository audioElementRepository_;
  inline static const juce::Identifier kAudioElementsKey{"audio_elements"};

  MultiChannelRepository multichannelgainRepository_;
  inline static const juce::Identifier kMultiChannelGainsKey{
      "multi_channel_gains"};

  RendererPluginSyncServer syncServer_;

  FileExportRepository fileExportRepository_;
  inline static const juce::Identifier kFileExportKey{"file_export"};

  MixPresentationRepository mixPresentationRepository_;
  inline static const juce::Identifier kMixPresentationsKey{
      "mix_presentations"};

  MixPresentationLoudnessRepository mixPresentationLoudnessRepository_;
  inline static const juce::Identifier kMixPresentationLoudnessKey{
      "mix_presentation_loudness"};

  MixPresentationSoloMuteRepository mixPresentationSoloMuteRepository_;
  inline static const juce::Identifier kMixPresentationSoloMuteKey{
      "mix_presentation_solo_mute"};

  MSPlaybackRepository msPlaybackRepository_;
  inline static const juce::Identifier kMSPlaybackKey{"playback_ms"};

  ActiveMixRepository activeMixPresentationRepository_;
  inline static const juce::Identifier kActiveMixKey{"active_mix"};

  FilePlaybackRepository filePlaybackRepository_;
  inline static const juce::Identifier kFilePlaybackKey{"file_playback"};

  SpeakerMonitorData monitorData_;

  ChannelMonitorData channelMonitorData_;

  juce::AudioChannelSet outputChannelSet_ = juce::AudioChannelSet::stereo();

  // Used by the debug build to prevent processing while changing
  // to non-realtime mode
  juce::SpinLock realtimeLock_;
  // Monitors if rendering in realtime mode or offline mode during debug builds
  // only
  bool isRealtime_;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RendererProcessor)
};