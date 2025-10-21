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
#include <data_structures/src/SpeakerMonitorData.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "components/src/TitledLabel.h"

class LoudnessStats : public juce::Component,
                      juce::ImageButton::Listener,
                      juce::Timer {
 public:
  LoudnessStats(SpeakerMonitorData& data);

  void paint(juce::Graphics& g);

  void buttonClicked(juce::Button* btn) override;

  void timerCallback() override;

 private:
  using LabelWithVal = std::pair<juce::Label, juce::Label>;

  void configureLabels(LabelWithVal& label);

  void drawStatValues(const int labelHeight, juce::Rectangle<int>& bounds);

  const int kNumMeasurements_ = 5;
  const int kLabelOffset_ = 5;
  const int kDecimalPlaces_ = 1;
  const juce::Colour kTextClr_ = EclipsaColours::tabTextGrey;
  const juce::String kInvalid_ = "--";
  const juce::Image kResetImg_ = IconStore::getInstance().getResetIcon();

  TitledLabel statsToDisp_;
  juce::ImageButton resetButton_;
  // Loudness stats to display.
  LabelWithVal momentary_, shortTerm_, integrated_, peak_, range_, target_;
  // Loudness stats source.
  SpeakerMonitorData& rtData_;
};