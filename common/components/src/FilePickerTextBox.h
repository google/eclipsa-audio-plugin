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

#include "TitledTextBox.h"

// Appends a default file extension only when appropriate:
//  * If the previous user text was empty and the user types the first
//  character(s), append immediately.
//  * Once user text is non-empty, allow free editing; append extension only
//  upon commit if missing.
//  * If the user clears the field back to empty, the next typed character will
//  trigger immediate append again.
class FilePickerTextBox : public TitledTextBox {
 public:
  FilePickerTextBox(const juce::String title,
                    const juce::String defaultExtension)
      : TitledTextBox(title),
        defaultExtension_(defaultExtension),
        userText_(""),
        isUpdating_(false) {
    TitledTextBox::onTextChanged([this]() { handleTextChange(); });
    setOnReturnCallback([this]() { commitValue(); });
    setOnFocusLostCallback([this]() { commitValue(); });
  }

  void setDefaultExtension(const juce::String ext) { defaultExtension_ = ext; }

  juce::String getText() { return TitledTextBox::getText(); }

  // Programmatic set should mirror committed behavior: append if non-empty &
  // missing.
  void setText(juce::String text) {
    if (!text.isEmpty() && !text.endsWithIgnoreCase(defaultExtension_)) {
      text += defaultExtension_;
    }
    isUpdating_ = true;
    userText_ = stripExtension(text);
    TitledTextBox::setText(text);
    isUpdating_ = false;
  }

  void onValueCommitted(std::function<void()> callback) {
    onValueCommittedCallback_ = callback;
  }

 private:
  void handleTextChange() {
    if (isUpdating_) return;
    isUpdating_ = true;

    // Capture previous user text length before updating.
    juce::String previousUserText = userText_;
    juce::String currentText = TitledTextBox::getText();
    userText_ = stripExtension(currentText);

    bool wasEmptyBefore = previousUserText.isEmpty();
    bool isNowNonEmpty = userText_.isNotEmpty();

    // Only auto-append if transitioning from empty -> non-empty.
    if (wasEmptyBefore && isNowNonEmpty &&
        !currentText.endsWithIgnoreCase(defaultExtension_)) {
      const juce::TextEditor* editor = getTextEditor();
      int caretPosition = editor ? editor->getCaretPosition() : -1;
      juce::String newText = userText_ + defaultExtension_;
      TitledTextBox::setText(newText);
      if (editor) {
        // Keep caret within user portion (before extension).
        const_cast<juce::TextEditor*>(editor)->setCaretPosition(
            juce::jmin(caretPosition, userText_.length()));
      }
    }

    isUpdating_ = false;
  }

  void commitValue() {
    juce::String current = TitledTextBox::getText();
    juce::String bare = stripExtension(current);
    if (bare.isNotEmpty() && !current.endsWithIgnoreCase(defaultExtension_)) {
      TitledTextBox::setText(bare + defaultExtension_);
    }
    if (onValueCommittedCallback_) onValueCommittedCallback_();
  }

  juce::String stripExtension(const juce::String& text) const {
    if (text.endsWithIgnoreCase(defaultExtension_)) {
      return text.substring(0, text.length() - defaultExtension_.length());
    }
    return text;
  }

  juce::String defaultExtension_;
  juce::String userText_;
  std::function<void()> onValueCommittedCallback_;
  bool isUpdating_;
};