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

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <vector>

#include "../processor_base/ProcessorBase.h"
#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/RoomSetupRepository.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/RepositoryCollection.h"
#include "data_structures/src/SpeakerMonitorData.h"
#include "substream_rdr/bin_rdr/BinauralRdr.h"
#include "substream_rdr/rdr_factory/Renderer.h"
#include "substream_rdr/rdr_factory/RendererFactory.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

struct AudioElementRenderer {
  // Pre-allocated buffer to be used to write the audio elements data to
  juce::AudioBuffer<float> inputData;
  juce::AudioBuffer<float> outputData;
  juce::AudioBuffer<float> outputDataBinaural;

  // First channel to pull the audio elements data from
  int firstChannel;

  const bool kIsBinaural;

  // Layout of the Audio Element.
  Speakers::AudioElementSpeakerLayout inputLayout;

  // Renderer to be used to render the audio element to the room setup
  std::unique_ptr<Renderer> renderer;
  std::unique_ptr<Renderer> rendererBinaural;

  // Constructor
  AudioElementRenderer(Speakers::AudioElementSpeakerLayout inputLayout,
                       Speakers::AudioElementSpeakerLayout playbackLayout,
                       int firstInputChannel, int samplesPerBlock,
                       int sampleRate, bool isBinaural = true);
};

//==============================================================================
class RenderProcessor final : public ProcessorBase, juce::ValueTree::Listener {
 public:
  //==============================================================================
  RenderProcessor(ProcessorBase* hostProc, RoomSetupRepository* roomSetupData,
                  AudioElementRepository* audioElementData,
                  MixPresentationRepository* mixPresData,
                  ActiveMixRepository* activeMixdata, SpeakerMonitorData& data);
  ~RenderProcessor() override;

  //==============================================================================
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  void setNonRealtime(bool isNonRealtime) noexcept override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;

  //==============================================================================
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  //==============================================================================
  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override {
    // 3 Cases for which renderers need to be reconstructed:
    // 1. The playback layout has changed.
    // 2. The active mix presentation has changed.
    // 3. The audio elements in the mix presentation have changed.
    // 4. The channel set of an audio element has changed.
    if (treeWhosePropertyHasChanged.getType() == RoomSetup::kTreeType &&
        property == RoomSetup::kSpeakerLayout) {
      initializeRenderers();
    } else if (treeWhosePropertyHasChanged.getType() ==
                   ActiveMixPresentation::kTreeType &&
               activeMixPresData_->get().getActiveMixId() != activeMixID_) {
      activeMixID_ = activeMixPresData_->get().getActiveMixId();
      initializeRenderers();
    } else if (treeWhosePropertyHasChanged.getType() ==
               MixPresentation::kTreeType) {
      initializeRenderers();
    } else if (treeWhosePropertyHasChanged.getType() ==
                   AudioElement::kTreeType &&
               property == AudioElement::kFirstChannel) {
      initializeRenderers();
    }
  }

  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override {
    if (parentTree.getType() == MixPresentation::kTreeType) {
      initializeRenderers();
    }
  }

  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override {
    if (childWhichHasBeenRemoved.getType() == MixPresentation::kTreeType) {
      initializeRenderers();
    }
  }

  //==============================================================================

  std::vector<AudioElementRenderer*> getAudioElementRenderers() {
    return audioElementRenderers_;
  }

  int getSpeakersOut() { return speakersOut_; }

 public:
  void reinitializeAfterStateRestore() { initializeRenderers(); }

 private:
  void initializeRenderers();

 private:
  void mixRenderedAudio(const bool mixFromBinaural, const int numSourceChannels,
                        juce::AudioBuffer<float>& outputBuffer);
  void updateBinauralLoudness(juce::AudioBuffer<float>& rdrdAudio);

  juce::AudioParameterFloatAttributes initParameterAttributes(
      int decimalPlaces, juce::String&& label) const {
    return juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction([decimalPlaces](float value, int unused) {
          juce::ignoreUnused(unused);
          return juce::String(value, decimalPlaces, false);
        })
        .withLabel(label);
  }

  ProcessorBase* hostProcessor_;
  RoomSetupRepository* roomSetupData_;
  AudioElementRepository* audioElementData_;
  MixPresentationRepository* mixPresData_;
  ActiveMixRepository* activeMixPresData_;
  juce::Uuid activeMixID_;
  SpeakerMonitorData& monitorData_;
  juce::SpinLock renderersLock_;
  std::vector<AudioElementRenderer*> audioElementRenderers_;
  juce::AudioBuffer<float> mixBuffer_;
  juce::AudioBuffer<float> binauralMixBuffer_;
  Speakers::AudioElementSpeakerLayout currentPlaybackLayout_;
  int currentSamplesPerBlock_;
  int currentSampleRate_ = 48000;
  int speakersOut_;
  float mixPresentationGain_ = 1.f;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RenderProcessor)
};
