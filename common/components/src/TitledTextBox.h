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
#include "components/src/MainEditor.h"

/*
  Creates a titled text box object
*/
class TitledTextBoxLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  TitledTextBoxLookAndFeel() {
    setColour(juce::TextEditor::backgroundColourId,
              EclipsaColours::backgroundOffBlack);
    setColour(juce::TextEditor::outlineColourId, EclipsaColours::tabTextGrey);
    setColour(juce::TextEditor::textColourId, EclipsaColours::headingGrey);
    setColour(juce::TextEditor::highlightColourId, EclipsaColours::headingGrey);
  }

  // Use custom paint instead
  void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                             juce::TextEditor& textEditor) override {}
};

class DimmedTitledTextBoxLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  DimmedTitledTextBoxLookAndFeel() {
    float alpha = 0.4f;
    setColour(juce::TextEditor::backgroundColourId,
              EclipsaColours::backgroundOffBlack);
    setColour(juce::TextEditor::outlineColourId,
              EclipsaColours::tabTextGrey.withAlpha(alpha));
    setColour(juce::TextEditor::textColourId,
              EclipsaColours::headingGrey.withAlpha(alpha));
    setColour(juce::TextEditor::highlightColourId,
              EclipsaColours::headingGrey.withAlpha(alpha));
  }

  // Use custom paint instead
  void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                             juce::TextEditor& textEditor) override {}
};

class PaddedTextEditor : public juce::TextEditor, public juce::Timer {
  juce::String title_;
  bool isFocused_ = false;
  bool caretVisible_ = false;

 public:
  PaddedTextEditor(juce::String title) : title_(title), juce::TextEditor() {}

  ~PaddedTextEditor() {
    onFocusLost = nullptr;
    onReturnKey = nullptr;
  }

  void setTitle(juce::String title) { title_ = title; }

  void focusGained(FocusChangeType cause) override {
    isFocused_ = true;
    startTimer(500);
    repaint();
  }

  void focusLost(FocusChangeType cause) override {
    isFocused_ = false;
    stopTimer();
    repaint();
    if (onFocusLost) {
      onFocusLost();
    }
  }

  void timerCallback() override {
    if (isFocused_ && isAccessible() && !isReadOnly()) {
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
    g.setColour(findColour(juce::TextEditor::outlineColourId));
    g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize,
                           1.0f);

    // Draw the title information
    boxBounds.removeFromLeft(10);
    auto font = juce::Font("Roboto", 12.0f, juce::Font::plain);
    int titleWidth = font.getStringWidth(title_);

    auto titleBounds =
        boxBounds.removeFromTop(15).removeFromLeft(titleWidth + 5);
    g.setColour(findColour(juce::TextEditor::backgroundColourId));
    g.fillRect(titleBounds.toFloat());
    g.setColour(findColour(juce::TextEditor::outlineColourId));
    g.setFont(font);
    g.drawText(title_, titleBounds.removeFromTop(8),
               juce::Justification::centred);

    // Draw the text
    auto textArea =
        getLocalBounds().withTrimmedTop(titleBuffer).withTrimmedLeft(15);
    g.setColour(findColour(juce::TextEditor::textColourId));
    setFont(juce::Font("Roboto", 14.0f, juce::Font::plain));
    g.setFont(getFont());
    g.setColour(juce::Colour(221, 228, 227));
    g.drawFittedText(getText(), textArea, juce::Justification::centredLeft, 1,
                     1.0f);

    // Draw the caret
    if (isFocused_ && caretVisible_) {
      juce::Rectangle<int> caret = textArea.withWidth(2);
      caret.setHeight(getFont().getHeight() + 2);
      getText().substring(0, getCaretPosition());
      float pos =
          getFont().getStringWidth(getText().substring(0, getCaretPosition()));

      caret.setX(pos + 15);
      caret.setY(caret.getY() + (textArea.getHeight() / 2) -
                 (caret.getHeight() / 2));
      g.fillRect(caret);
    }
  }
};

class TitledTextBox : public juce::Component {
  PaddedTextEditor textEditor_;
  TitledTextBoxLookAndFeel lookAndFeel_;
  DimmedTitledTextBoxLookAndFeel dimmedLookAndFeel_;

 public:
  TitledTextBox(juce::String title) : juce::Component(), textEditor_(title) {
    setLookAndFeel(&lookAndFeel_);
    textEditor_.setJustification(juce::Justification::bottomLeft);
  }

  ~TitledTextBox() override {
    setLookAndFeel(nullptr);
    setOnReturnCallback(nullptr);
    setOnFocusLostCallback(nullptr);
  }

  void setText(juce::String text) { textEditor_.setText(text); }

  juce::String getText() { return textEditor_.getText(); }

  void setTitle(juce::String title) { textEditor_.setTitle(title); }

  const juce::TextEditor* const getTextEditor() const { return &textEditor_; }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();

    // Draw the text box
    addAndMakeVisible(textEditor_);
    textEditor_.setBounds(bounds);
    textEditor_.setMultiLine(false);
  }

  void onTextChanged(std::function<void()> callback) {
    textEditor_.onTextChange = callback;
  }

  void setOnReturnCallback(std::function<void()> callback) {
    textEditor_.onReturnKey = callback;
  }

  void setOnFocusLostCallback(std::function<void()> callback) {
    textEditor_.onFocusLost = callback;
  }

  void setOnEscapeKeyCallback(std::function<void()> callback) {
    textEditor_.onEscapeKey = callback;
  }

  void setInputRestrictions(const int& maxLength,
                            const juce::String& allowedCharacters) {
    textEditor_.setInputRestrictions(maxLength, allowedCharacters);
  }

  void dimLookAndFeel() { setLookAndFeel(&dimmedLookAndFeel_); }

  void resetLookAndFeel() { setLookAndFeel(&lookAndFeel_); }

  bool textEditorIsFocused() { return textEditor_.hasKeyboardFocus(true); }

  void setReadOnly(const bool isReadOnly) {
    textEditor_.setAccessible(!isReadOnly);
    textEditor_.setReadOnly(isReadOnly);
    textEditor_.setCaretVisible(isReadOnly);
    textEditor_.stopTimer();
  }
};
