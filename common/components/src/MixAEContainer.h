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

#include <juce_gui_basics/juce_gui_basics.h>

#include "EclipsaColours.h"
#include "Icons.h"

class MixAEContainer : public juce::Component {
 public:
  MixAEContainer(const juce::String& title, const juce::String& desc)
      : name_(title), desc_(desc), isBinauralCheckbox_("Binaural") {
    nameLabel_.setText(name_, juce::dontSendNotification);
    nameLabel_.setColour(juce::Label::textColourId,
                         EclipsaColours::headingGrey);
    nameLabel_.setJustificationType(juce::Justification::bottomLeft);
    addAndMakeVisible(nameLabel_);

    descLabel_.setText(desc_, juce::dontSendNotification);
    descLabel_.setColour(juce::Label::textColourId,
                         EclipsaColours::tabTextGrey);
    descLabel_.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(descLabel_);

    juce::Image removeAEImage = IconStore::getInstance().getRemoveAEIcon();
    removeAEButton_.setImages(true, true, true, removeAEImage, 1.0f,
                              juce::Colours::transparentBlack, removeAEImage,
                              0.5f, juce::Colours::grey, removeAEImage, 0.8f,
                              EclipsaColours::iconWhite);
    addAndMakeVisible(removeAEButton_);

    isBinauralCheckbox_.setColour(juce::ToggleButton::textColourId,
                                  EclipsaColours::headingGrey);
    addAndMakeVisible(isBinauralCheckbox_);

    // Configure the tooltip image
    tooltipImage_.setImage(IconStore::getInstance().getTooltipIcon());
    tooltipImage_.setTooltip(
        "Binaural\n\n"
        "If checked, this audio element will be rendered binaurally during "
        "binaural "
        "playback. \n"
        "If unchecked, this audio element will be rendered as stereo during "
        "binaural playback.");

    addAndMakeVisible(tooltipImage_);
  }

  ~MixAEContainer() override {
    for (auto listener : listeners_) {
      removeAEButton_.removeListener(listener);
    }
    setLookAndFeel(nullptr);
  }
  void setBinauralChangeHandler(std::function<void(bool)> callback) {
    isBinauralCheckbox_.onClick = [callback, this]() {
      callback(isBinauralCheckbox_.getToggleState());
    };
  }
  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();
    const auto mixAEContainerBounds = bounds;

    // Set the background colour to grey
    g.setColour(EclipsaColours::inactiveGrey);

    // Draw a filled rounded rectangle
    juce::Rectangle<float> rect(bounds.toFloat());
    g.fillRect(rect);
    const float labelWidth = 0.6f;
    auto labelBounds = bounds.removeFromLeft(
        mixAEContainerBounds.proportionOfWidth(labelWidth));

    auto nameBounds = labelBounds.removeFromTop(
        mixAEContainerBounds.proportionOfHeight(0.5f));
    nameLabel_.setBounds(nameBounds);
    nameLabel_.setColour(juce::Label::textColourId,
                         EclipsaColours::headingGrey);
    descLabel_.setBounds(labelBounds);
    descLabel_.setColour(juce::Label::textColourId,
                         EclipsaColours::tabTextGrey);

    int toRemove = 6;
    bounds.reduce(toRemove, toRemove);

    const float tooltipImageWidth = 0.04f;
    const float binauralCheckboxWidth = 0.12f;

    auto tooltipImageBounds = bounds.removeFromRight(
        mixAEContainerBounds.proportionOfWidth(tooltipImageWidth));
    tooltipImage_.setBounds(tooltipImageBounds);

    // Reserve space for the checkbox and button
    auto checkboxBounds = bounds.removeFromRight(
        mixAEContainerBounds.proportionOfWidth(binauralCheckboxWidth));
    isBinauralCheckbox_.setBounds(checkboxBounds);

    auto buttonBounds = bounds;
    removeAEButton_.setBounds(buttonBounds);
  }

  const juce::ImageButton* const getDeleteButton() const {
    return &removeAEButton_;
  }

  juce::ToggleButton* getIsBinauralCheckbox() { return &isBinauralCheckbox_; }

  void setDeleteButtonListener(juce::Button::Listener* listener) {
    removeAEButton_.addListener(listener);
    listeners_.push_back(listener);
  }

  void updateName(const juce::String& name) {
    name_ = name;
    nameLabel_.setText(name_, juce::dontSendNotification);
    nameLabel_.repaint();
  }

 private:
  juce::String name_;
  juce::String desc_;

  juce::Label nameLabel_;
  juce::Label descLabel_;

  juce::ImageButton removeAEButton_;
  juce::ToggleButton isBinauralCheckbox_;
  juce::TooltipWindow tooltipWindow_;
  juce::ImageComponent tooltipImage_;

  std::vector<juce::Button::Listener*> listeners_;
};