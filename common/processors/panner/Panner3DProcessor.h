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

#include <memory>

#include "../processor_base/ProcessorBase.h"
#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_structures/src/AudioElementParameterTree.h"
#include "data_structures/src/ParameterMetaData.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"
#include "substream_rdr/surround_panner/AudioPanner.h"

//==============================================================================
class Panner3DProcessor final
    : public ProcessorBase,
      juce::ValueTree::Listener,
      public juce::AudioProcessorValueTreeState::Listener {
 public:
  //==============================================================================
  Panner3DProcessor(
      ProcessorBase* hostProcessor,
      AudioElementSpatialLayoutRepository* audioElementSpatialLayoutRepository,
      AudioElementParameterTree* automationParameterTree);
  ~Panner3DProcessor() override;

  //==============================================================================
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;

  //==============================================================================
  const juce::String getName() { return "Panner 3D"; }

  //==============================================================================
  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override {
    if (property == AudioElementSpatialLayout::kLayout ||
        property == AudioElementSpatialLayout::kPanningEnabled) {
      initializePanning();
    }
  }
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override {
    initializePanning();
  }
  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override {
    initializePanning();
  }
  void valueTreeChildOrderChanged(
      juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex,
      int newIndex) override {
    initializePanning();
  }
  void valueTreeParentChanged(
      juce::ValueTree& treeWhoseParentHasChanged) override {
    initializePanning();
  }

  //==============================================================================
  void parameterChanged(const juce::String& parameterID,
                        float newValue) override {
    if (parameterID == AutoParamMetaData::xPosition) {
      xPosition_ = newValue;
    } else if (parameterID == AutoParamMetaData::yPosition) {
      yPosition_ = newValue;
    } else if (parameterID == AutoParamMetaData::zPosition) {
      zPosition_ = newValue;
    }
  }

  //==============================================================================

 private:
  void initializePanning();

  ProcessorBase* hostProcessor_;
  AudioElementSpatialLayoutRepository* audioElementSpatialLayoutData_;
  AudioElementParameterTree* automationParameterTree_;
  juce::SpinLock renderLock;
  std::unique_ptr<AudioPanner> surroundPanner_;
  int samplesPerBlock_;
  int sampleRate_;
  Speakers::AudioElementSpeakerLayout inputLayout_;
  Speakers::AudioElementSpeakerLayout outputLayout_;
  juce::AudioBuffer<float> outputBuffer_;
  int xPosition_;
  int yPosition_;
  int zPosition_;

  // Position caching for optimization - avoid redundant renderer updates
  int lastSetXPosition_ = -999;
  int lastSetYPosition_ = -999;
  int lastSetZPosition_ = -999;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Panner3DProcessor)
};
