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

#include "ControlKnob.h"
#include "TitledTextBox.h"

class TextEditorControlledDial : public juce::Component {
 public:
  TextEditorControlledDial(const juce::String& title, const int& defaultValue,
                           const int& currValue, const int& min, const int& max,
                           juce::Image leftImage = juce::Image(),
                           juce::Image rightImage = juce::Image());

  ~TextEditorControlledDial() override;

  void paint(juce::Graphics& g) override;

  void setText(const juce::String& text) {}

  void dimLookAndFeel();

  void resetLookAndFeel();

  void setValueUpdatedCallback(std::function<void(int)> callback) {
    valueUpdatedCallback_ = callback;
    slider_.setValueUpdatedCallback(callback);
  }

  // triggered when automation changes the value
  void setValue(const float& value);

  std::string getParameterLabel() { return parameterLabel_; }

  bool isTextBoxFocused() { return textBox_.textEditorIsFocused(); }

 private:
  void adjustDialAspectRatio(juce::Rectangle<int>& dialBounds);
  void textEditorChangedCallback();
  std::function<void()> textUpdated_;

  int moveToNextMultipleOf5();
  int moveToPreviousMultipleOf5();

  int value_;
  int min_;
  int max_;
  const std::string parameterLabel_;
  AudioElementParameterTree* parameterTree_;
  juce::ImageButton leftButton_;
  juce::Image leftImage_;
  juce::ImageButton rightButton_;
  juce::Image rightImage_;
  TitledTextBox textBox_;
  ControlKnob slider_;
  juce::String currentText_;
  std::function<void(int)> valueUpdatedCallback_;
};