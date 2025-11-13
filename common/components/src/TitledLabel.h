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
#ifdef _WIN32
    #include <errno.h>
#else
    #include <sys/errno.h>
#endif

#include "../components.h"
#include "TitledTextBox.h"
#include "components/src/MainEditor.h"

class OutlinelessPaddedTextEditor : public juce::TextEditor,
                                    public juce::Timer {
  juce::String title_;
  bool isFocused_ = false;
  bool caretVisible_ = false;

 public:
  OutlinelessPaddedTextEditor(juce::String title)
      : title_(title), juce::TextEditor() {}

  void focusGained(FocusChangeType cause) override {
    isFocused_ = true;
    startTimer(500);
    repaint();
  }

  void focusLost(FocusChangeType cause) override {
    isFocused_ = false;
    stopTimer();
    repaint();
  }

  void timerCallback() override {
    if (isFocused_) {
      caretVisible_ = !caretVisible_;
      repaint();
    }
  }

  void paintOverChildren(juce::Graphics& g) override {
    // Fill the background
    g.fillAll(findColour(juce::TextEditor::backgroundColourId));

    // Draw the outline and title
    auto bounds = getLocalBounds();
    auto cornerSize = 5.0f;
    int titleBuffer = 20;
    juce::Rectangle<int> boxBounds = bounds.withTrimmedTop(titleBuffer);

    // Draw the title information
    boxBounds.removeFromLeft(10);
    auto font = juce::Font("Roboto", 12.0f, juce::Font::plain);
    int titleWidth = getFont().getStringWidth(title_);

    auto titleBounds =
        boxBounds.removeFromTop(15).removeFromLeft(titleWidth + 5);
    g.setColour(findColour(juce::TextEditor::backgroundColourId));
    g.fillRect(titleBounds.toFloat());
    g.setColour(findColour(juce::TextEditor::outlineColourId));
    g.setFont(font);
    g.drawText(title_, titleBounds.removeFromTop(8),
               juce::Justification::centred);

    // Draw the text
    auto textArea = boxBounds.withTrimmedBottom(5).withTrimmedLeft(5);
    g.setColour(findColour(juce::TextEditor::textColourId));
    setFont(juce::Font("Roboto", 14.0f, juce::Font::plain));
    g.setFont(getFont());
    g.setColour(juce::Colour(221, 228, 227));
    g.drawFittedText(getText(), textArea, juce::Justification::centred, 1,
                     1.0f);
  }
};

class TitledLabel : public juce::Component {
  OutlinelessPaddedTextEditor textEditor_;
  TitledTextBoxLookAndFeel lookAndFeel_;

 public:
  TitledLabel(juce::String title) : juce::Component(), textEditor_(title) {
    setLookAndFeel(&lookAndFeel_);
    textEditor_.setJustification(juce::Justification::bottomLeft);
    textEditor_.setEnabled(
        false);  // Using text editor as label, should never be editable
  }

  ~TitledLabel() override { setLookAndFeel(nullptr); }

  void setText(juce::String text) { textEditor_.setText(text); }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();

    // Draw the text box
    addAndMakeVisible(textEditor_);
    textEditor_.setBounds(bounds);
    textEditor_.setMultiLine(false);
  }
};