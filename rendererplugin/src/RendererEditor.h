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

#include <components/components.h>

#include "RendererProcessor.h"
#include "components/src/DAWWarningBanner.h"
#include "screens/MonitorScreen.h"

class CustomLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  CustomLookAndFeel();

  void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool isMouseOverButton, bool isButtonDown) override;
};

//==============================================================================
class RendererEditor final : public MainEditor {
 public:
  explicit RendererEditor(RendererProcessor& p);
  ~RendererEditor() override;

  //==============================================================================
  void paint(juce::Graphics&) override;
  void resized() override;

  //==============================================================================
  void setScreen(juce::Component& screen) override;
  void resetScreen() override;

 private:
  CustomLookAndFeel customLookAndFeel_;

  juce::Label titleLabel_;
  DAWWarningBanner dawWarningBanner_;
  MonitorScreen monitorScreen_;
  juce::Component* currentScreen_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RendererEditor)
};
