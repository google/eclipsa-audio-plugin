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
#include "EclipsaColours.h"
#include "TitledTextBox.h"

enum class KnobColourIds {
  dialFill = 0,
  dialOutline = 1,
  blueArc = 2,
};

class ControlKnobLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  ControlKnobLookAndFeel(const float& defaultNormalizedValue);
  void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                        float sliderPosProportional, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider& s) override;

  double ensureValueIsWithinRange(const double& normalizedValue) {
    if (normalizedValue < 0.0) {
      return 0.0;
    } else if (normalizedValue > 1.0) {
      return 1.0;
    } else if (std::isnan(normalizedValue)) {
      return 0.0;
    }
    return normalizedValue;
  }

 private:
  float angularDeflection(const float& normalizedValue);
  juce::Point<int> getDialXY(const float& endAngle);

  juce::Path knobPath_;
  const float defaultNormalizedValue_;
  const float startAngle_ = 9 * juce::MathConstants<float>::pi / 8;
  const float endAngle_ = 23 * juce::MathConstants<float>::pi / 8;
  const float defaultAngle_;
  float radius_;
  float centerX_;
  float centerY_;
};

class DimmedControlKnobLookAndFeel : public ControlKnobLookAndFeel {
 public:
  DimmedControlKnobLookAndFeel(const float& defaultNormalizedValue);
};
class ControlKnob : public juce::Slider {
 public:
  ControlKnob(const double& min, const double& max, const double& defaultValue,
              const double& currentValue = 0, const juce::String& suffix = "");
  ~ControlKnob();
  void setValue(float newValue);

  void setValueUpdatedCallback(std::function<void(int)> callback);

  void dimLookAndFeel() { setLookAndFeel(&dimmedLookAndFeel_); }

  void resetLookAndFeel() { setLookAndFeel(&lookAndFeel_); }

  void mouseDown(const juce::MouseEvent& event) override;

 private:
  const double startAngle_ = 9 * juce::MathConstants<double>::pi / 8;
  const double endAngle_ = 23 * juce::MathConstants<double>::pi / 8;
  const double max_;
  const double min_;
  const double defaultNormalizedValue_;

  ControlKnobLookAndFeel lookAndFeel_;
  DimmedControlKnobLookAndFeel dimmedLookAndFeel_;

  std::function<void(int)> valueChangedCallback_;
  std::function<void()> valueUpdatedCallback_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlKnob)
};