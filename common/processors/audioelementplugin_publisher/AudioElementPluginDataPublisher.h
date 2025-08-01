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
#include "MessagingThread.h"
#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_structures/src/AudioElementCommunication.h"
#include "data_structures/src/AudioElementParameterTree.h"
#include "data_structures/src/ParameterMetaData.h"
#include "data_structures/src/RealtimeDataType.h"
#include "data_structures/src/SpeakerMonitorData.h"

//==============================================================================
class AudioElementPluginDataPublisher final
    : public ProcessorBase,
      juce::ValueTree::Listener,
      public juce::AudioProcessorValueTreeState::Listener {
 public:
  //==============================================================================
  AudioElementPluginDataPublisher(
      AudioElementSpatialLayoutRepository* audioElementSpatialLayoutRepository,
      AudioElementParameterTree* automationParameterTree);
  ~AudioElementPluginDataPublisher() override;

  //==============================================================================
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;

  //==============================================================================
  const juce::String getName() { return "Audio Element Plugin Data Publisher"; }

  //==============================================================================
  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override {
    updateData();
  }
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override {
    updateData();
  }
  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override {
    updateData();
  }
  void valueTreeChildOrderChanged(
      juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex,
      int newIndex) override {
    updateData();
  }
  void valueTreeParentChanged(
      juce::ValueTree& treeWhoseParentHasChanged) override {
    updateData();
  }

  //==============================================================================
  void parameterChanged(const juce::String& parameterID,
                        float newValue) override {
    // Guaranteed to be called before processBlock so we don't need to lock
    if (parameterID == AutoParamMetaData::xPosition) {
      localData_.x = newValue;
    } else if (parameterID == AutoParamMetaData::yPosition) {
      localData_.y = newValue;
    } else if (parameterID == AutoParamMetaData::zPosition) {
      localData_.z = newValue;
    }
    updateData();
  }

  //==============================================================================

 private:
  void updateData();

  RealtimeDataType<float> avgLoudness_;
  AudioElementSpatialLayoutRepository* audioElementSpatialLayoutData_;
  AudioElementParameterTree* automationParameterTree_;
  std::unique_ptr<MessagingThread> messagingThread_;
  AudioElementUpdateData localData_;
  int channels_;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioElementPluginDataPublisher)
};
