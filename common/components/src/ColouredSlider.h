// Copyright 2025 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "components/src/EclipsaColours.h"

// Custom LookAndFeel (internal)
class BlueSliderLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  enum ThumbStyle { Circle, FlatBar };

  BlueSliderLookAndFeel(ThumbStyle style = Circle) : thumbStyle_(style) {}

  void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle,
                        juce::Slider& slider) override {
    auto trackHeight = 4.0f;
    auto thumbRadius = 4.0f;

    auto trackBounds =
        juce::Rectangle<float>((float)x, y + height * 0.5f - trackHeight * 0.5f,
                               (float)width, trackHeight);

    g.setColour(juce::Colours::darkgrey);
    g.fillRect(trackBounds);

    auto valueBounds = trackBounds.withWidth(sliderPos - (float)x);
    g.setColour(EclipsaColours::selectCyan);
    g.fillRect(valueBounds);

    // Draw shadow for thumb
    juce::DropShadow shadow(juce::Colours::black.withAlpha(0.6f), 6,
                            juce::Point<int>(0, 2));

    if (thumbStyle_ == FlatBar) {
      // Draw flat bar thumb (half the bounds height)
      auto barWidth = 3.0f;
      auto barHeight = height * 0.5f;
      auto barBounds = juce::Rectangle<float>(
          sliderPos - barWidth / 2.0f, y + height * 0.5f - barHeight / 2.0f,
          barWidth, barHeight);

      // Draw shadow for flat bar
      juce::Path shadowPath;
      shadowPath.addRoundedRectangle(barBounds.expanded(1.0f), 1.0f);
      shadow.drawForPath(g, shadowPath);

      // Draw flat bar thumb
      g.setColour(EclipsaColours::selectCyan);
      g.fillRoundedRectangle(barBounds, 1.0f);
    } else {
      // Draw shadow for circle
      auto circleBounds = juce::Rectangle<float>(
          sliderPos - thumbRadius, trackBounds.getCentreY() - thumbRadius,
          thumbRadius * 2.0f, thumbRadius * 2.0f);
      juce::Path shadowPath;
      shadowPath.addEllipse(circleBounds.expanded(1.0f));
      shadow.drawForPath(g, shadowPath);

      // Draw circle thumb
      g.setColour(EclipsaColours::selectCyan);
      g.fillEllipse(circleBounds);
    }
  }

 private:
  ThumbStyle thumbStyle_;
};

// Self-contained slider
class ColouredSlider : public juce::Slider {
 public:
  enum ThumbStyle { Circle, FlatBar };

  ColouredSlider(ThumbStyle style = Circle)
      : lookAndFeel(style == Circle ? BlueSliderLookAndFeel::Circle
                                    : BlueSliderLookAndFeel::FlatBar) {
    setSliderStyle(juce::Slider::LinearHorizontal);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setLookAndFeel(&lookAndFeel);
  }

  ~ColouredSlider() override { setLookAndFeel(nullptr); }

 private:
  BlueSliderLookAndFeel lookAndFeel;
};
