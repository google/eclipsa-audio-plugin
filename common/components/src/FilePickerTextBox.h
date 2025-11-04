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

// Automatically appends a default file extension to the `TitledTextBox` value
class FilePickerTextBox : public TitledTextBox {
 public:
  FilePickerTextBox(const juce::String title,
                    const juce::String defaultExtension)
      : TitledTextBox(title),
        defaultExtension_(defaultExtension),
        userText_(""),
        isUpdating_(false) {
    // Override the text change callback to handle extension appending
    TitledTextBox::onTextChanged([this]() { handleTextChange(); });

    // Set up callbacks for committing the value
    setOnReturnCallback([this]() { commitValue(); });
    setOnFocusLostCallback([this]() { commitValue(); });
  }

  void setDefaultExtension(const juce::String ext) { defaultExtension_ = ext; }

  juce::String getText() { return TitledTextBox::getText(); }

  void setText(juce::String text) {
    isUpdating_ = true;
    userText_ = stripExtension(text);
    TitledTextBox::setText(appendExtension(userText_));
    isUpdating_ = false;
  }

  void onValueCommitted(std::function<void()> callback) {
    onValueCommittedCallback_ = callback;
  }

 private:
  juce::String appendExtension(const juce::String& text) {
    // Only append extension if there's user text
    if (text.isEmpty()) {
      return text;
    }
    return text + defaultExtension_;
  }

  void handleTextChange() {
    if (isUpdating_) return;

    isUpdating_ = true;

    // Get the current text from the editor
    juce::String currentText = TitledTextBox::getText();

    // Strip the extension to get the user text
    userText_ = stripExtension(currentText);

    // Update the text editor with the extension appended
    const juce::TextEditor* editor = getTextEditor();
    if (editor != nullptr) {
      int caretPosition = editor->getCaretPosition();
      juce::String newText = appendExtension(userText_);

      // Only update if different to avoid cursor jumping
      if (currentText != newText) {
        TitledTextBox::setText(newText);

        // Restore caret position (but not beyond user text length)
        const_cast<juce::TextEditor*>(editor)->setCaretPosition(
            juce::jmin(caretPosition, userText_.length()));
      }
    }

    isUpdating_ = false;
  }

  void commitValue() {
    // Call the user-provided callback when value is committed
    if (onValueCommittedCallback_) {
      onValueCommittedCallback_();
    }
  }

  juce::String stripExtension(const juce::String& text) {
    // If the text ends with the default extension, remove it
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