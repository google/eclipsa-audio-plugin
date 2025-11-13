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

class LoudnessLevelBar : public juce::Component, public juce::Timer {
 public:
  LoudnessLevelBar();

  void paint(juce::Graphics& g) override;

  void timerCallback() override;

  void resetResidualPeak() {
    resPeak_.counterToDecay_ = kRefreshRate_ * kDecayPeriod_;
    resPeak_.level_ = 70;
  }
  void setLoudness(const float loudness) { loudness_ = loudness; }
  int getbarWidth() { return barWidth_; }

  static const int kRefreshRate_ = 10;  // Refresh rate in Hz.
  static const int kDecayPeriod_ = 3;   // Decay period in s.

 private:
  struct ResidualPeak {
    int counterToDecay_ = kRefreshRate_ * kDecayPeriod_;
    int level_ = 70;
  };

  bool isValidLoudness(const float loudness) {
    return !(std::isnan(loudness) || std::isinf(loudness) || loudness == 0.0f);
  };
  void updateResidualPeak(const int level);
  void drawResidualPeak(const std::pair<int, int> range,
                        juce::Rectangle<int>& bounds, juce::Graphics& g);

  int fillBar(const int level, const std::pair<int, int> range,
              juce::Rectangle<int>& bounds, juce::Graphics& g);

  // Loudness level colors.
  const juce::Colour kGreen_ = juce::Colour(153, 247, 104);
  const juce::Colour kYellow_ = juce::Colour(254, 252, 118);
  const juce::Colour kOrange_ = juce::Colour(224, 162, 78);
  const juce::Colour kRed_ = juce::Colour(216, 68, 50);
  const juce::Colour kGray_ = juce::Colour(49, 54, 54);

  // Loudness level colour ranges.
  const int kGreenStart_ = 60;
  const int kGreenEnd_ = 20;
  const int kYellowEnd_ = 6;
  const int kOrangeEnd_ = 2;
  const int kRedEnd_ = 1;

  int barWidth_ = 0;
  float loudness_ = -60.0f;
  ResidualPeak resPeak_;
};