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

#include "TextEditorControlledDial.h"

TextEditorControlledDial::TextEditorControlledDial(
    const juce::String& title, const int& defaultValue, const int& currValue,
    const int& min, const int& max, juce::Image leftImage,
    juce::Image rightImage)
    : textUpdated_([this]() { textEditorChangedCallback(); }),
      parameterLabel_(title.toStdString()),
      value_(currValue),
      min_(min),
      max_(max),
      currentText_(juce::String(currValue)),
      textBox_(title),
      slider_(min, max, defaultValue, currValue),
      leftImage_(leftImage),
      rightImage_(rightImage) {
  textBox_.setOnReturnCallback(textUpdated_);
  textBox_.setOnFocusLostCallback(textUpdated_);
  textBox_.setText(currentText_);

  addAndMakeVisible(slider_);
  addAndMakeVisible(textBox_);

  if (leftImage.isValid()) {
    leftButton_.setImages(false, true, true, leftImage, 1.0f,
                          EclipsaColours::tabTextGrey, leftImage, 1.0f,
                          EclipsaColours::tabTextGrey, leftImage, 1.0f,
                          juce::Colours::whitesmoke);
    addAndMakeVisible(leftButton_);
    leftButton_.onClick = [this]() {
      int value = moveToPreviousMultipleOf5();
      textBox_.setText(juce::String(value));
      textEditorChangedCallback();
    };
  }
  if (rightImage.isValid()) {
    rightButton_.setImages(false, true, true, rightImage, 1.0f,
                           EclipsaColours::tabTextGrey, rightImage, 1.0f,
                           EclipsaColours::tabTextGrey, rightImage, 1.0f,
                           juce::Colours::whitesmoke);
    addAndMakeVisible(rightButton_);
    rightButton_.onClick = [this]() {
      int value = moveToNextMultipleOf5();
      textBox_.setText(juce::String(value));
      textEditorChangedCallback();
    };
  }
}

TextEditorControlledDial::~TextEditorControlledDial() {
  setLookAndFeel(nullptr);
}

void TextEditorControlledDial::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();

  bounds.reduce(5, 5);
  const auto textEditorControlledDialBounds = bounds;

  auto dialBounds = bounds.removeFromTop(
      textEditorControlledDialBounds.proportionOfHeight(0.5f));

  auto chevronBounds = dialBounds;

  // set the bounds of both the image buttons, if either provided image is
  // valid
  juce::Rectangle<int> leftChevronBounds;
  if (leftImage_.isValid()) {
    chevronBounds.removeFromTop(chevronBounds.proportionOfHeight(0.4f));
    chevronBounds.removeFromBottom(chevronBounds.proportionOfHeight(0.2f));
    leftChevronBounds =
        chevronBounds.removeFromLeft(dialBounds.proportionOfWidth(0.25f));
    leftButton_.setBounds(leftChevronBounds);
  }

  dialBounds.reduce(textEditorControlledDialBounds.proportionOfWidth(0.2f),
                    0.f);

  // help center the dial relative to the text box
  dialBounds.removeFromLeft(
      textEditorControlledDialBounds.proportionOfWidth(0.07f));
  dialBounds.removeFromTop(
      textEditorControlledDialBounds.proportionOfHeight(0.125f));
  adjustDialAspectRatio(dialBounds);
  slider_.setBounds(dialBounds);

  if (rightImage_.isValid() && leftImage_.isValid()) {
    int rightSeparation = dialBounds.getX() - leftChevronBounds.getRight();
    juce::Rectangle<int> rightChevronBounds(
        dialBounds.getRight() + rightSeparation, leftChevronBounds.getY(),
        leftChevronBounds.getWidth(), leftChevronBounds.getHeight());

    rightButton_.setBounds(rightChevronBounds);
  }

  auto textBoxBounds = bounds;
  textBoxBounds.reduce(textEditorControlledDialBounds.proportionOfWidth(0.2f),
                       0.f);
  textBox_.setBounds(textBoxBounds);
}

void TextEditorControlledDial::dimLookAndFeel() {
  slider_.dimLookAndFeel();
  textBox_.dimLookAndFeel();
  if (leftImage_.isValid()) {
    leftButton_.setAlpha(0.4f);
    leftButton_.setEnabled(false);
  }
  if (rightImage_.isValid()) {
    rightButton_.setAlpha(0.4f);
    rightButton_.setEnabled(false);
  }
}

void TextEditorControlledDial::resetLookAndFeel() {
  slider_.resetLookAndFeel();
  textBox_.resetLookAndFeel();
  if (leftImage_.isValid()) {
    leftButton_.setAlpha(1.0f);
    leftButton_.setEnabled(true);
  }
  if (rightImage_.isValid()) {
    rightButton_.setAlpha(1.0f);
    rightButton_.setEnabled(true);
  }
}

// triggered when automation changes the value
void TextEditorControlledDial::setValue(const float& value) {
  int newValue = static_cast<int>(value);
  textBox_.setText(juce::String(newValue));
  // update the dial visual
  slider_.setValue(newValue);
}

void TextEditorControlledDial::adjustDialAspectRatio(
    juce::Rectangle<int>& dialBounds) {
  if (dialBounds.getWidth() < dialBounds.getHeight()) {
    dialBounds.setHeight(dialBounds.getWidth());
  } else {
    dialBounds.setWidth(dialBounds.getHeight());
  }
}
void TextEditorControlledDial::textEditorChangedCallback() {
  int value = textBox_.getText().getIntValue();

  if (value == currentText_.getIntValue()) {
    return;  // no update needed
  } else {
    // the value will need to be updated
    //  clamp the value if it is out of range
    if (value > max_) {
      value = max_;
    } else if (value < min_) {
      value = min_;
    }
    currentText_ = juce::String(value);
    // update the position
    slider_.setValue(value);
    textBox_.setText(currentText_);  // ensure the appended text is displayed
    slider_.repaint();
  }

  // Finally, call the callback if needed
  if (valueUpdatedCallback_) {
    valueUpdatedCallback_(value);
  }
}

int TextEditorControlledDial::moveToNextMultipleOf5() {
  int value = textBox_.getText().getIntValue();
  // if the number is not already a multiple of 5,
  // increment it by the complement to the next multiple of 5
  if (value % 5 != 0) {
    value += 5 - (value % 5);
  } else {
    value += 5;
  }

  return value;
}
int TextEditorControlledDial::moveToPreviousMultipleOf5() {
  int value = textBox_.getText().getIntValue();
  // if the number is not already a multiple of 5,
  // decrement it by the complement to the previous multiple of 5
  if (value % 5 != 0) {
    value -= value % 5;
  } else {
    value -= 5;
  }

  return value;
}
