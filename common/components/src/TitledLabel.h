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

#include "TitledTextBox.h"
class TitledLabel : public juce::Component {
  TitledTextBox disabledTextBox_;

 public:
  TitledLabel(juce::String title) : juce::Component(), disabledTextBox_(title) {
    disabledTextBox_.setReadOnly(
        true);  // Using text editor as label, should never be editable
  }

  ~TitledLabel() override { setLookAndFeel(nullptr); }

  void setText(juce::String text) { disabledTextBox_.setText(text); }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();

    // Draw the text box
    addAndMakeVisible(disabledTextBox_);
    disabledTextBox_.setBounds(bounds);
  }

  void reduceTitleBuffer(int amount) {
    disabledTextBox_.reduceTitleBuffer(amount);
  }
};