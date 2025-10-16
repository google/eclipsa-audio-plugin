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

#include "../components.h"

class TitledTextBox : public juce::Component {
  juce::Label titleLabel_;
  juce::TextEditor textEditor_;
  juce::Colour outlineColour_;
  int titleBuffer_;

 public:
  TitledTextBox(juce::String title)
      : juce::Component(), titleLabel_(title), textEditor_(), titleBuffer_(20) {
    // Set the outline colour to the enabled colour state
    resetLookAndFeel();

    // Configure the title label
    auto font = juce::Font("Roboto", 12.0f, juce::Font::plain);
    titleLabel_.setFont(font);
    titleLabel_.setColour(juce::Label::backgroundColourId,
                          EclipsaColours::backgroundOffBlack);

    // Configure the text editor
    textEditor_.setJustification(juce::Justification::topLeft);
    titleLabel_.setText(title, juce::NotificationType::dontSendNotification);

    textEditor_.setColour(juce::TextEditor::backgroundColourId,
                          EclipsaColours::backgroundOffBlack);
    textEditor_.setColour(juce::TextEditor::outlineColourId,
                          EclipsaColours::backgroundOffBlack);
    textEditor_.setColour(juce::TextEditor::focusedOutlineColourId,
                          EclipsaColours::backgroundOffBlack);
    textEditor_.setColour(juce::TextEditor::textColourId,
                          EclipsaColours::headingGrey);
    textEditor_.setFont(juce::Font("Roboto", 14.0f, juce::Font::plain));
  }

  ~TitledTextBox() override {
    setOnReturnCallback(nullptr);
    setOnFocusLostCallback(nullptr);
  }

  void setText(juce::String text) { textEditor_.setText(text); }

  juce::String getText() { return textEditor_.getText(); }

  void setTitle(juce::String title) { textEditor_.setTitle(title); }

  const juce::TextEditor* const getTextEditor() const { return &textEditor_; }

  void paint(juce::Graphics& g) override {
    // This component is drawn by drawing an outline box
    // Then drawing a label over the top left corner of the box with the title
    // Then drawing a text box inside the outline

    auto bounds = getLocalBounds();

    // Fill the background
    g.fillAll(EclipsaColours::backgroundOffBlack);

    // Draw the outline
    auto cornerSize = 5.0f;
    // Add some buffering space for where the title will be higher then the
    // outline
    juce::Rectangle<int> boxBounds = bounds.withTrimmedTop(titleBuffer_);
    g.setColour(outlineColour_);
    g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize,
                           1.0f);

    // Draw the label over the outline
    // The label width requires some padding or it has issues with single
    // character labels. The padding is roughly one character wide.
    auto labelWidth =
        titleLabel_.getFont().getStringWidth(titleLabel_.getText()) + 8;
    auto labelHeight = titleLabel_.getFont().getHeight();
    juce::Rectangle<int> labelBounds = juce::Rectangle<int>(
        boxBounds.getX() + 10, (boxBounds.getY() - labelHeight / 2) + 3,
        labelWidth, labelHeight);
    titleLabel_.setBounds(labelBounds);
    addAndMakeVisible(titleLabel_);

    // Create bounds which encapusulate inside the outline
    boxBounds.reduce(10, 5);

    // Draw the text box centered inside the outline
    addAndMakeVisible(textEditor_);
    textEditor_.setBounds(boxBounds);
    // Add some leaway for text height as it doesn't account for characters like
    // g which go below the line
    auto textHeight = textEditor_.getFont().getHeight() + 6;
    auto boxBoundsTrim = ((boxBounds.getHeight() - textHeight) / 2) - 1;
    boxBounds.removeFromTop(boxBoundsTrim);
    textEditor_.setBounds(boxBounds.removeFromTop(textHeight + 15));
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

  void updateOutlineColour(juce::Colour colour) {
    outlineColour_ = colour;
    titleLabel_.setColour(juce::Label::textColourId, outlineColour_);
    repaint();
  }

  void dimLookAndFeel() {
    updateOutlineColour(EclipsaColours::tabTextGrey.withAlpha(0.4f));
  }

  void resetLookAndFeel() { updateOutlineColour(EclipsaColours::tabTextGrey); }

  bool textEditorIsFocused() { return textEditor_.hasKeyboardFocus(true); }

  void setReadOnly(const bool isReadOnly) {
    textEditor_.setAccessible(!isReadOnly);
    textEditor_.setReadOnly(isReadOnly);
    textEditor_.setCaretVisible(isReadOnly);
  }

  void reduceTitleBuffer(int amount) { titleBuffer_ -= amount; }
};
