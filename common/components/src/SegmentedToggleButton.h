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
#include <juce_gui_basics/juce_gui_basics.h>

#include "components/src/EclipsaColours.h"
#include "components/src/Icons.h"

class STBLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  STBLookAndFeel();

  void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool isMouseOverButton, bool isButtonDown) override;

  void drawButtonText(juce::Graphics& g, juce::TextButton& btn,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;
};

class SegmentedToggleButton : public juce::Component {
 public:
  SegmentedToggleButton(const std::initializer_list<juce::String>& opts,
                        const bool singularToggle);

  ~SegmentedToggleButton() { setLookAndFeel(nullptr); }

  void paint(juce::Graphics& g);

  void setToggleable(const juce::String& opt, const bool enable);
  void toggleOn(const juce::String& opt);
  bool getOption(const juce::String& opt) const;
  void setOption(const juce::String& opt, const bool state);
  std::vector<std::pair<juce::String, bool>> getState() const;
  std::vector<juce::String> getToggled() const;
  void setEnabledForOption(const juce::String& opt, bool enabled);
  bool isOptionEnabled(const juce::String& opt) const;
  int getSelectedIndex() const;

  void onChange(std::function<void()> func) { parentCallback_ = func; }

 private:
  void configureButtons();
  // Toggle button and alert listener.
  void toggleButton(juce::Button* btn);

  const juce::Image kCheckImg_ = IconStore::getInstance().getCheckmarkIcon();
  const bool kSingularToggle_ = false;
  std::vector<std::unique_ptr<juce::TextButton>> buttons_;
  std::function<void()> parentCallback_;
  STBLookAndFeel lookAndFeel_;
};