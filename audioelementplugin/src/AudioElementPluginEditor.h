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
#include <components/components.h>

#include "AudioElementPluginProcessor.h"
#include "components/src/SelectionBox.h"
#include "components/src/SliderButton.h"
#include "components/src/TitledTextBox.h"
#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_structures/src/SpeakerMonitorData.h"
#include "screens/PositionSelectionScreen.h"
#include "screens/RoomViewScreen.h"
#include "screens/TrackMonitorScreen.h"

class CustomLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  CustomLookAndFeel();

  void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool isMouseOverButton, bool isButtonDown) override;
};

//==============================================================================
class AudioElementPluginEditor final : public juce::AudioProcessorEditor,
                                       public AudioElementPluginListener,
                                       public juce::ValueTree::Listener {
 public:
  explicit AudioElementPluginEditor(AudioElementPluginProcessor& p);
  ~AudioElementPluginEditor() override;

  void audioElementsUpdated() override;

  //==============================================================================
  void setAudioElementSelection();

  void paint(juce::Graphics&) override;
  void resized() override;

  //==============================================================================
  void setMode();

  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override {
    if (property == AudioElementSpatialLayout::kPanningEnabled) {
      setMode();
    }
    // Update track name text box when spatial layout name changes
    if (property == AudioElementSpatialLayout::kName) {
      AudioElementSpatialLayout layout =
          audioElementSpatialLayoutRepository_->get();
      juce::String newName = layout.getName();

      // Temporarily disable callback to avoid feedback loop
      trackNameTextBox_.onTextChanged(nullptr);
      trackNameTextBox_.setText(newName);

      // Re-enable callback for manual editing
      trackNameTextBox_.onTextChanged([this]() {
        AudioElementSpatialLayout toUpdate =
            audioElementSpatialLayoutRepository_->get();
        toUpdate.setName(trackNameTextBox_.getText());
        audioElementSpatialLayoutRepository_->update(toUpdate);
      });
    }
  }
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override {
    setMode();
  }
  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override {
    setMode();
  }
  void valueTreeChildOrderChanged(
      juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex,
      int newIndex) override {
    setMode();
  }
  void valueTreeParentChanged(
      juce::ValueTree& treeWhoseParentHasChanged) override {
    setMode();
  }

 private:
  RoomLayout layout_;
  CustomLookAndFeel customLookAndFeel_;

  juce::Label titleLabel_;
  juce::Label panningControlsLabel_;
  SliderButton panningControls_;
  juce::Component* currentScreen_;

  AudioElementSpatialLayoutRepository* audioElementSpatialLayoutRepository_;
  AudioElementPluginSyncClient* syncClient_;
  SpeakerMonitorData* spkrData_;

  PositionSelectionScreen positionSelectionScreen_;
  RoomViewScreen roomViewScreen_;
  TrackMonitorScreen trackMonitorScreen_;

  TitledTextBox trackNameTextBox_;
  TitledTextBox outputModeTypeLabel_;
  SelectionBox audioElementSelectionBox_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioElementPluginEditor)
};