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

#include "TitledTextBox.h"

class GainControlTextEditor : public juce::Component {
 public:
  GainControlTextEditor(const std::function<void()>& callback)
      : callback_(callback), titledTextBox_("Gain") {
    addAndMakeVisible(titledTextBox_);

    addAndMakeVisible(dBLabel_);
    dBLabel_.setText("dB", juce::dontSendNotification);
    dBLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    dBLabel_.setJustificationType(juce::Justification::right);
    dBLabel_.setColour(juce::Label::backgroundColourId,
                       juce::Colours::transparentBlack);

    titledTextBox_.setOnReturnCallback(callback_);
    titledTextBox_.setOnFocusLostCallback(callback_);
    titledTextBox_.setInputRestrictions(5, "-0123456789");
  }

  ~GainControlTextEditor() override { setLookAndFeel(nullptr); }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();

    titledTextBox_.setBounds(bounds);

    bounds.removeFromLeft(20);
    bounds.removeFromTop(bounds.proportionOfHeight(0.22f));
    bounds.removeFromRight(4);
    dBLabel_.setBounds(bounds);
  }

  void setText(const juce::String& text) { titledTextBox_.setText(text); }

  juce::String getText() { return titledTextBox_.getText(); }

 private:
  std::function<void()> callback_;
  TitledTextBox titledTextBox_;
  juce::Label dBLabel_;
};