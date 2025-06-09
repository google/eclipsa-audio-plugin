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
#include <components/src/EclipsaColours.h>
#include <components/src/TextEditorControlledDial.h>
#include <data_structures/src/ParameterMetaData.h>

#include <cstddef>

#include "../AudioElementPluginProcessor.h"
#include "data_structures/src/AudioElementParameterTree.h"

class PositionSelectionLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  PositionSelectionLookAndFeel() {
    setColour(juce::Label::textColourId, EclipsaColours::headingGrey);
  }
};

class PositionSelectionScreen : public juce::Component,
                                public juce::ValueTree::Listener,
                                public juce::Timer {
 public:
  PositionSelectionScreen(
      AudioElementSpatialLayoutRepository* audioElementSpatialLayoutRepo_,
      AudioElementParameterTree& apvts);

  ~PositionSelectionScreen();

  void paint(juce::Graphics& g) override;

 private:
  std::unique_ptr<TextEditorControlledDial> createDialWithChevrons(
      const juce::String& title, const int& defaultValue = 0,
      const int& currValue = 0, const std::pair<int, int>& range = {-50, 50});

  void adjustLabelBounds(juce::Rectangle<int>& labelBounds,
                         const juce::Rectangle<int> positionBounds);

  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;

  void timerCallback() override;

  float getValue(const std::string& parameterLabel);

  void updateDialVisibility(const Elevation elevation);

  // Add a control button to return to the main screen
  AudioElementParameterTree& parameterTree_;
  PositionSelectionLookAndFeel lookAndFeel_;
  AudioElementSpatialLayoutRepository* audioElementSpatialLayoutRepo_;

  juce::Label positionLabel_;
  juce::Label spreadLabel_;
  juce::Label lfeLabel_;

  juce::OwnedArray<TextEditorControlledDial> positionDials;
  const int z_dial_index = 2;

  const std::pair<int, int> positionRange_;
};