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
#include <sys/errno.h>

#include "../components.h"
#include "components/src/EclipsaColours.h"
#include "components/src/MainEditor.h"

/*
  Creates a selection button object, which consists of a blue button and a
  set of selection options
*/
class SelectionButtonLookAndFeel : public juce::LookAndFeel_V4 {
  juce::String title_;

 public:
  SelectionButtonLookAndFeel(juce::String title) : title_(title) {
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(128, 213, 212));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(128, 213, 212));
    setColour(juce::ComboBox::textColourId, EclipsaColours::buttonTextColour);
    setColour(juce::ComboBox::arrowColourId, EclipsaColours::buttonTextColour);
    setColour(juce::ComboBox::focusedOutlineColourId, juce::Colours::white);
  }

  void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override {
    auto textBounds = label.getBounds();
    textBounds = textBounds.removeFromBottom(textBounds.getHeight() - 20);
    label.setBounds(textBounds);
  }

  void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH,
                    juce::ComboBox& box) override {
    juce::Rectangle<int> boxBounds(0, 0, width, height);
    float cornerSize = boxBounds.getHeight() / 2.0f;
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

    g.setColour(box.findColour(juce::ComboBox::textColourId));
    g.setFont(box.getLookAndFeel().getComboBoxFont(box));
    juce::Rectangle<int> textArea(1, 1, width - 30, height - 2);
    g.drawFittedText(title_, textArea, juce::Justification::centred, 1);
  }

  void drawPopupMenuBackground(juce::Graphics& g, int width,
                               int height) override {
    g.fillAll(juce::Colour(26, 33, 33));
  }
};

class SelectionButton : public juce::Component, juce::ComboBox::Listener {
  juce::ComboBox selectionBox_;
  SelectionButtonLookAndFeel lookAndFeel_;
  juce::ComboBox::Listener* listener_;
  bool isEnabled_;

 public:
  SelectionButton(juce::String title, juce::ComboBox::Listener* listener)
      : juce::Component(),
        lookAndFeel_(title),
        listener_(listener),
        isEnabled_(true) {
    setLookAndFeel(&lookAndFeel_);
    selectionBox_.addListener(this);
  }

  ~SelectionButton() override { setLookAndFeel(nullptr); }

  void addOption(juce::String option) {
    selectionBox_.addItem(option, selectionBox_.getNumItems() + 1);
    selectionBox_.setSelectedId(0);
  }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();

    // Draw the combo box
    addAndMakeVisible(selectionBox_);
    selectionBox_.setBounds(bounds);
  }

  void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) {
    if (selectionBox_.getSelectedId() != 0) {
      listener_->comboBoxChanged(comboBoxThatHasChanged);
      selectionBox_.setSelectedId(0);
    }
  };

  void disable() {
    if (!isEnabled_) return;
    isEnabled_ = false;
    setEnabled(false);
    lookAndFeel_.setColour(juce::ComboBox::backgroundColourId,
                           juce::Colour(40, 45, 46));
    lookAndFeel_.setColour(juce::ComboBox::textColourId,
                           juce::Colour(109, 114, 115));
    lookAndFeelChanged();
  }

  void enable() {
    if (isEnabled_) return;
    isEnabled_ = true;
    setEnabled(true);
    lookAndFeel_.setColour(juce::ComboBox::backgroundColourId,
                           juce::Colour(128, 213, 212));
    lookAndFeel_.setColour(juce::ComboBox::textColourId,
                           juce::Colour(0, 55, 55));
    lookAndFeelChanged();
  }

  void clear(
      juce::NotificationType notification = juce::sendNotificationAsync) {
    selectionBox_.clear(notification);
  }

  void addItemList(const juce::StringArray& items, int startIndex) {
    selectionBox_.addItemList(items, startIndex);
  }

  void setItemEnabled(int itemId, bool shouldBeEnabled) {
    selectionBox_.setItemEnabled(itemId, shouldBeEnabled);
  }

  bool isItemEnabled(int itemId) const {
    return selectionBox_.isItemEnabled(itemId);
  }
};
