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
#include "ControlKnob.h"

class SkewedSliderLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  SkewedSliderLookAndFeel(const double& normalizedMidPointValue,
                          const double& currentNormalizedValue);
  ~SkewedSliderLookAndFeel();
  void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                        float sliderPosProportional, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider& s) override;

 private:
  double scaledSliderProportionalPos(const double& sliderPosProportional);
  double angularDeflection(const double& normalizedValue);
  juce::Point<int> getDialXY(const float& endAngle);

  juce::Path knobPath_;
  const float startAngle_ = 9 * juce::MathConstants<float>::pi / 8;
  const float endAngle_ = 23 * juce::MathConstants<float>::pi / 8;
  const float defaultAngle_ = startAngle_ + (0.5f * (endAngle_ - startAngle_));
  const double midpointNormalizedValue_;
  double initialNormalizedValue_;
  bool initializationCall_ = true;
  float radius_;
  float centerX_;
  float centerY_;
};

class ControlKnobSkewed : public juce::Slider {
 public:
  ControlKnobSkewed(const double& min, const double& max,
                    const double& defaultValue, const double& currentValue,
                    const juce::String& suffix = "");
  ~ControlKnobSkewed() { setLookAndFeel(nullptr); }

  void setValue(float newValue);

  void setValueUpdatedCallback(std::function<void(int)> callback);

  void mouseDown(const juce::MouseEvent& event) override;

 private:
  float normalizeValue(const float& value);
  const double startAngle_ = 9 * juce::MathConstants<double>::pi / 8;
  const double endAngle_ = 23 * juce::MathConstants<double>::pi / 8;
  const double max_;
  const double min_;
  const double defaultNormalizedValue_;
  const juce::String suffix_;
  SkewedSliderLookAndFeel lookAndFeel_;

  std::function<void(int)> valueChangedCallback_;
  std::function<void()> valueUpdatedCallback_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlKnobSkewed)
};