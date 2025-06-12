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

#include "../processor_base/ProcessorBase.h"
#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_structures/src/AudioElementPluginSyncClient.h"
#include "data_structures/src/AudioElementSpatialLayout.h"

//==============================================================================
class RoutingProcessor final : public ProcessorBase,
                               juce::ValueTree::Listener,
                               AudioElementPluginListener {
 public:
  //==============================================================================
  RoutingProcessor(
      AudioElementSpatialLayoutRepository* AudioElementSpatialLayoutRepository,
      AudioElementPluginSyncClient* syncClient, int totalChannelCount);
  ~RoutingProcessor() override;

  //==============================================================================
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;

  //==============================================================================
  const juce::String getName() { return "Audio Element Plugin Router"; }

  //==============================================================================
  void audioElementsUpdated() { initializeRouting(); }

  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override {
    initializeRouting();
  }
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override {
    initializeRouting();
  }
  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override {
    initializeRouting();
  }
  void valueTreeChildOrderChanged(
      juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex,
      int newIndex) override {
    initializeRouting();
  }
  void valueTreeParentChanged(
      juce::ValueTree& treeWhoseParentHasChanged) override {
    initializeRouting();
  }

  //==============================================================================

 private:
  void initializeRouting();

  AudioElementSpatialLayoutRepository* audioElementSpatialLayoutData_;
  AudioElementPluginSyncClient* syncClient_;

  // Playback information required for shifting the audio channels
  std::atomic_int firstChannel_;
  std::atomic_int totalChannels_;
  const int totalChannelCount_;
  juce::AudioBuffer<float> copyBuffer_;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingProcessor)
};
