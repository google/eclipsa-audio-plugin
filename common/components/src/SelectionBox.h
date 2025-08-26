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

#include <cmath>

#include "EclipsaColours.h"

/*
  Creates a selection box object, which consists of both a title and combo box
*/
class SelectionBoxLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  SelectionBoxLookAndFeel(juce::String title) : title_(title) {
    applycolours();
  }
  SelectionBoxLookAndFeel(juce::String title, juce::Image image)
      : title_(title), image_(image) {
    applycolours();
  }

  void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override {
    auto textBounds = label.getBounds();
    textBounds = textBounds.removeFromBottom(textBounds.getHeight() - 20);
    label.setBounds(textBounds);
    label.setColour(juce::Label::textColourId,
                    findColour(juce::ComboBox::textColourId));
  }

  void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH,
                    juce::ComboBox& box) override {
    // Draw the outline of the box
    auto cornerSize = 5.0f;
    int titleBuffer = 20;
    juce::Rectangle<int> boxBounds(0, titleBuffer, width, height - titleBuffer);
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize,
                           1.0f);

    // Draw the title information
    boxBounds.removeFromLeft(10);
    // draw the border label/title
    if (title_ != "") {
      auto font = juce::Font("Roboto", 12.0f, juce::Font::plain);
      int titleWidth = font.getStringWidth(title_);

      auto titleBounds =
          boxBounds.removeFromTop(15).removeFromLeft(titleWidth + 5);

      g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
      g.fillRect(titleBounds.toFloat());
      g.setColour(box.findColour(juce::ComboBox::outlineColourId));
      g.setFont(font);
      g.drawText(title_, titleBounds.removeFromTop(8),
                 juce::Justification::centred);
    }

    juce::Rectangle<int> arrowZone(width - 30, titleBuffer, 20,
                                   height - titleBuffer);
    juce::Path path;
    path.addTriangle(arrowZone.getX() + 3.0f, arrowZone.getCentreY() - 2.0f,
                     arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f,
                     arrowZone.getRight() - 3.0f,
                     arrowZone.getCentreY() - 2.0f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.fillPath(path);

    g.setColour(box.findColour(juce::ComboBox::textColourId));
    g.setFont(box.getLookAndFeel().getComboBoxFont(box));
    auto fontHeight = juce::roundToInt(g.getCurrentFont().getHeight());
    juce::Rectangle<int> textArea(15, titleBuffer + 1, width - 30,
                                  height - 2 - titleBuffer);
    juce::String textToDraw = box.getText();
    int textWidth = g.getCurrentFont().getStringWidth(textToDraw);
    // if the text is too long, truncate it
    if (textWidth > textArea.getWidth()) {
      float ratio = static_cast<float>(textArea.getWidth()) /
                    static_cast<float>(textWidth);
      int endIndex = std::floor(ratio * (textToDraw.length() - 1)) -
                     3;  // remove an additional 3 characters for the '...'
      textToDraw = textToDraw.substring(0, endIndex);
      textToDraw += "...";
    }
    g.drawFittedText(textToDraw, textArea, juce::Justification::centredLeft, 1);
    if (image_.isValid()) {
      auto imageRect =
          juce::Rectangle<float>(textArea.getX() - fontHeight * 2.25f,
                                 textArea.getCentreY() - fontHeight * 0.75f,
                                 fontHeight * 1.25f, fontHeight * 1.25f);
      g.drawImage(image_, imageRect);
    }
  }

  void drawPopupMenuBackground(juce::Graphics& g, int width,
                               int height) override {
    g.fillAll(EclipsaColours::backgroundOffBlack);
  }

 private:
  virtual void applycolours() {
    setColour(juce::ComboBox::backgroundColourId,
              EclipsaColours::backgroundOffBlack);
    setColour(juce::ComboBox::outlineColourId, EclipsaColours::tabTextGrey);
    setColour(juce::ComboBox::textColourId, EclipsaColours::tabTextGrey);
    setColour(juce::ComboBox::buttonColourId, EclipsaColours::headingGrey);
    setColour(juce::ComboBox::focusedOutlineColourId,
              EclipsaColours::headingGrey);
    setColour(juce::ComboBox::arrowColourId, EclipsaColours::headingGrey);
  };
  juce::String title_;
  juce::Image image_;
};

class OffSelectionBoxLookAndFeel : public SelectionBoxLookAndFeel {
 public:
  OffSelectionBoxLookAndFeel(juce::String title)
      : SelectionBoxLookAndFeel(title) {}
  OffSelectionBoxLookAndFeel(juce::String title, juce::Image image)
      : SelectionBoxLookAndFeel(title, image) {}

  void onSwitch() { applycolours(); }

 private:
  void applycolours() override {
    float alpha = 0.4f;
    setColour(juce::ComboBox::backgroundColourId,
              EclipsaColours::backgroundOffBlack);
    setColour(juce::ComboBox::outlineColourId,
              EclipsaColours::tabTextGrey.withAlpha(alpha));
    setColour(juce::ComboBox::textColourId,
              EclipsaColours::tabTextGrey.withAlpha(alpha));
    setColour(juce::ComboBox::buttonColourId,
              EclipsaColours::headingGrey.withAlpha(alpha));
    setColour(juce::ComboBox::focusedOutlineColourId,
              EclipsaColours::headingGrey.withAlpha(alpha));
    setColour(juce::ComboBox::arrowColourId,
              EclipsaColours::headingGrey.withAlpha(alpha));
  };
};

class SelectionBox : public juce::Component {
  juce::String title_;
  juce::ComboBox selectionBox_;
  SelectionBoxLookAndFeel lookAndFeel_;
  OffSelectionBoxLookAndFeel offLookAndFeel_;

 public:
  SelectionBox(juce::String title)
      : lookAndFeel_(title), offLookAndFeel_(title) {
    setLookAndFeel(&lookAndFeel_);
  }
  SelectionBox(juce::String title, juce::Image icon)
      : lookAndFeel_(title, icon), offLookAndFeel_(title, icon) {
    setLookAndFeel(&lookAndFeel_);
  }

  ~SelectionBox() override {
    setLookAndFeel(nullptr);
    for (auto listener : listeners_) {
      selectionBox_.removeListener(listener);
    }
    selectionBox_.hidePopup();
  }

  void addOption(juce::String option, bool enabled = true) {
    int id = selectionBox_.getNumItems() + 1;
    selectionBox_.addItem(option, id);
    if (!enabled) {
      selectionBox_.setItemEnabled(id, false);
    }
    if (selectionBox_.getNumItems() == 1) {
      selectionBox_.setSelectedId(1);
    }
  }

  void setOption(juce::String option) { selectionBox_.setText(option); }

  void clear(
      juce::NotificationType notification = juce::sendNotificationAsync) {
    selectionBox_.clear(notification);
  }

  int getSelectedIndex() { return selectionBox_.getSelectedItemIndex(); }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();

    // Draw the combo box
    addAndMakeVisible(selectionBox_);
    selectionBox_.setBounds(bounds);
  }

  void onChange(std::function<void()> func) { selectionBox_.onChange = func; }

  void setSelectedIndex(const int& index,
                        const juce::NotificationType& notification) {
    selectionBox_.setSelectedItemIndex(index, notification);
  }

  void setTextWhenNothingSelected(juce::String text) {
    selectionBox_.setTextWhenNothingSelected(text);
  }

  void dimSelectionBox() {
    setLookAndFeel(&offLookAndFeel_);
    offLookAndFeel_.onSwitch();
  }

  void restoreLookAndFeel() { setLookAndFeel(&lookAndFeel_); }

  void setText(juce::String text) { selectionBox_.setText(text); }

  const juce::ComboBox* const getComboBox() const { return &selectionBox_; }

  void addListener(juce::ComboBox::Listener* listener) {
    selectionBox_.addListener(listener);
    listeners_.push_back(listener);
  }

  void disableIndex(const int& index) {
    selectionBox_.getChildComponent(index)->setEnabled(false);
  }
  void enableIndex(const int& index) {
    selectionBox_.getChildComponent(index)->setEnabled(true);
  }

  void setNameForComboBox(juce::String name) { selectionBox_.setName(name); }

 private:
  std::vector<juce::ComboBox::Listener*> listeners_;
};