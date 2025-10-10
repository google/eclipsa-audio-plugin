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

#include "ControlKnob.h"

ControlKnobLookAndFeel::ControlKnobLookAndFeel(
    const float& defaultNormalizedValue)
    : defaultNormalizedValue_(ensureValueIsWithinRange(defaultNormalizedValue)),
      defaultAngle_(startAngle_ +
                    (defaultNormalizedValue_ * (endAngle_ - startAngle_))) {
  setColour((int)KnobColourIds::dialFill, EclipsaColours::inactiveGrey);
  setColour((int)KnobColourIds::dialOutline, EclipsaColours::headingGrey);
  setColour((int)KnobColourIds::blueArc, EclipsaColours::controlBlue);
}
void ControlKnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y,
                                              int width, int height,
                                              float sliderPosProportional,
                                              float rotaryStartAngle,
                                              float rotaryEndAngle,
                                              juce::Slider& s) {
  // Draw the knob
  auto bounds = s.getLocalBounds();
  radius_ = 0.95f * bounds.getWidth() / 2.f;
  centerX_ = bounds.getCentreX();
  centerY_ = bounds.getCentreY();

  auto lineThickness = 2.0f;

  g.setColour(findColour((int)KnobColourIds::dialFill));
  g.fillEllipse(bounds.toFloat());

  // Define the path for the arc
  juce::Path arcPath;
  arcPath.addCentredArc(centerX_, centerY_, radius_, radius_, 0,
                        rotaryStartAngle, rotaryEndAngle, true);

  g.setColour(findColour((int)KnobColourIds::dialOutline));
  g.strokePath(arcPath,
               juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // Draw the blue arc
  g.setColour(findColour((int)KnobColourIds::blueArc));
  arcPath.clear();
  float endAngle = defaultAngle_ + angularDeflection(sliderPosProportional);
  arcPath.addCentredArc(centerX_, centerY_, radius_, radius_, 0, defaultAngle_,
                        endAngle, true);
  g.strokePath(arcPath, juce::PathStrokeType(lineThickness * 1.75f,
                                             juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

  // Draw the dial
  g.setColour(findColour((int)KnobColourIds::dialOutline));
  auto dialXY = getDialXY(endAngle);
  g.drawLine(centerX_, centerY_, dialXY.x, dialXY.y, lineThickness);
}

float ControlKnobLookAndFeel::angularDeflection(const float& normalizedValue) {
  return (normalizedValue - defaultNormalizedValue_) *
         (endAngle_ - startAngle_);
}

juce::Point<int> ControlKnobLookAndFeel::getDialXY(const float& endAngle) {
  // ensure dial does not intersect the arc
  float multiple = 0.8f;
  int xOnArc = std::ceil(centerX_ + (multiple * radius_ * std::sin(endAngle)));
  int yOnArc = std::ceil(centerY_ - (multiple * radius_ * std::cos(endAngle)));
  return {xOnArc, yOnArc};
}

DimmedControlKnobLookAndFeel::DimmedControlKnobLookAndFeel(
    const float& defaultNormalizedValue)
    : ControlKnobLookAndFeel(defaultNormalizedValue) {
  float alpha = 0.4f;
  setColour((int)KnobColourIds::dialFill,
            EclipsaColours::inactiveGrey.withAlpha(alpha));
  setColour((int)KnobColourIds::dialOutline,
            EclipsaColours::headingGrey.withAlpha(alpha));
  setColour((int)KnobColourIds::blueArc,
            EclipsaColours::controlBlue.withAlpha(alpha));
}

ControlKnob::ControlKnob(const double& min, const double& max,
                         const double& defaultValue, const double& currValue,
                         const juce::String& suffix)
    : min_(min),
      max_(max),
      defaultNormalizedValue_((defaultValue - min) / (max - min)),
      lookAndFeel_(defaultNormalizedValue_),
      dimmedLookAndFeel_(defaultNormalizedValue_),
      juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag,
                   juce::Slider::NoTextBox) {
  setLookAndFeel(&lookAndFeel_);
  setRotaryParameters(startAngle_, endAngle_, true);
  juce::Slider::setRange(min_, max_, 1.f);
  juce::Slider::setValue(currValue);
}

ControlKnob::~ControlKnob() { setLookAndFeel(nullptr); }

void ControlKnob::setValue(float newValue) { juce::Slider::setValue(newValue); }

void ControlKnob::setValueUpdatedCallback(std::function<void(int)> callback) {
  valueChangedCallback_ = callback;
  valueUpdatedCallback_ = [this]() { valueChangedCallback_(getValue()); };
  juce::Slider::onValueChange = valueUpdatedCallback_;
}
