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

#include "PositionSelectionScreen.h"

#include <memory>

#include "components/src/Icons.h"
#include "data_structures/src/AudioElementParameterTree.h"
#include "data_structures/src/ParameterMetaData.h"

std::unique_ptr<TextEditorControlledDial>
PositionSelectionScreen::createDialWithChevrons(
    const juce::String& title, const int& defaultValue, const int& currValue,
    const std::pair<int, int>& range) {
  std::unique_ptr<TextEditorControlledDial> dial =
      std::make_unique<TextEditorControlledDial>(
          title, defaultValue, currValue, range.first, range.second,
          IconStore::getInstance().getLeftChevronIcon(),
          IconStore::getInstance().getRightChevronIcon());
  dial->setTitle(title);
  addAndMakeVisible(dial.get());
  dial.get()->setValueUpdatedCallback([this, title](int newVal) {
    parameterTree_.getParameterAsValue(title).setValue(newVal);
  });
  return dial;
}

PositionSelectionScreen::PositionSelectionScreen(
    AudioElementSpatialLayoutRepository* audioElementSpatialLayoutRepo,
    AudioElementParameterTree& apvts)
    : audioElementSpatialLayoutRepo_(audioElementSpatialLayoutRepo),
      parameterTree_(apvts) {
  audioElementSpatialLayoutRepo_->registerListener(this);
  setLookAndFeel(&lookAndFeel_);

  startTimerHz(10);

  positionLabel_.setText("Position", juce::dontSendNotification);
  positionLabel_.setColour(juce::Label::textColourId,
                           EclipsaColours::headingGrey);
  positionLabel_.setJustificationType(juce::Justification::centredLeft);
  positionLabel_.setFont(juce::Font(18.0f));

  addAndMakeVisible(positionLabel_);

  positionDials.add(createDialWithChevrons(AutoParamMetaData::xPosition, 0,
                                           parameterTree_.getXPosition(),
                                           AutoParamMetaData::positionRange_));
  positionDials.add(createDialWithChevrons(AutoParamMetaData::yPosition, 0,
                                           parameterTree_.getYPosition(),
                                           AutoParamMetaData::positionRange_));
  positionDials.add(createDialWithChevrons(AutoParamMetaData::zPosition, 0,
                                           parameterTree_.getZPosition(),
                                           AutoParamMetaData::positionRange_));

  // Disable the Z-position control if elevation is not 'None' or 'Flat'.
  updateDialVisibility(static_cast<Elevation>(
      audioElementSpatialLayoutRepo_->get().getElevation()));

  // temporarily hide the rotation and size dials
  for (int i = 3; i < positionDials.size(); i++) {
    positionDials[i]->setVisible(false);
  }
}

PositionSelectionScreen::~PositionSelectionScreen() {
  audioElementSpatialLayoutRepo_->deregisterListener(this);
  // remove all GUI listeners
  setLookAndFeel(nullptr);
}

void PositionSelectionScreen::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();

  bounds.removeFromTop(30);

  // Set bounds for the "Position" label in the top-left corner
  auto labelBounds = bounds.removeFromTop(20);
  positionLabel_.setBounds(labelBounds);

  // Add padding below the label
  bounds.removeFromTop(5);

  // The width for the dials area (75% of available width)
  const float dialsWidth = bounds.proportionOfWidth(0.75f);
  const float dialWidth = bounds.proportionOfWidth(
      0.22f);  // Each dial takes 22% of the total width
  const float paddingWidth =
      bounds.proportionOfWidth(0.03f);  // Padding between dials (3%)

  // The area for the dials within the left 75% of the available width
  auto dialsArea = bounds.removeFromLeft(dialsWidth);

  // Set bounds for each dial, including the 3% gap between them
  for (int i = 0; i < positionDials.size(); i++) {
    auto dialBounds = dialsArea.removeFromLeft(dialWidth);
    dialBounds.setHeight(dialBounds.getWidth());
    dialBounds.reduce(1.f, 1.f);
    positionDials[i]->setBounds(dialBounds);

    // Add padding between dials, except after the last dial
    if (i < positionDials.size() - 1) {
      dialsArea.removeFromLeft(paddingWidth);
    }
  }
}

void PositionSelectionScreen::adjustLabelBounds(
    juce::Rectangle<int>& labelBounds,
    const juce::Rectangle<int> positionBounds) {
  // Set the label to a fixed height (20% of the position bounds height)
  labelBounds.setHeight(positionBounds.proportionOfHeight(0.2f));
}

void PositionSelectionScreen::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  // disable the z-position control if elevation is not none or flat
  if (property == AudioElementSpatialLayout::kElevation) {
    updateDialVisibility(static_cast<Elevation>(
        audioElementSpatialLayoutRepo_->get().getElevation()));
  }
}

void PositionSelectionScreen::timerCallback() {
  for (auto& dial : positionDials) {
    if (dial->isTextBoxFocused()) {
      continue;
    } else {
      float newValue = getValue(dial->getParameterLabel());
      dial->setValue(newValue);
    }
  }
}

float PositionSelectionScreen::getValue(const std::string& parameterLabel) {
  if (parameterLabel == AutoParamMetaData::xPosition) {
    return parameterTree_.getXPosition();
  } else if (parameterLabel == AutoParamMetaData::yPosition) {
    return parameterTree_.getYPosition();
  } else if (parameterLabel == AutoParamMetaData::zPosition) {
    return parameterTree_.getZPosition();
  } else {
    return 0.f;
  }
}

void PositionSelectionScreen::updateDialVisibility(const Elevation elevation) {
  if (audioElementSpatialLayoutRepo_->get().getElevation() !=
          AudioElementSpatialLayout::Elevation::kNone &&
      audioElementSpatialLayoutRepo_->get().getElevation() !=
          AudioElementSpatialLayout::Elevation::kFlat) {
    positionDials[z_dial_index]->setEnabled(false);
    positionDials[z_dial_index]->dimLookAndFeel();
  } else {
    positionDials[z_dial_index]->setEnabled(true);
    positionDials[z_dial_index]->resetLookAndFeel();
  }
}
