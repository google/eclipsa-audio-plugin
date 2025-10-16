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

#include "ControlKnobSkewed.h"

SkewedSliderLookAndFeel::SkewedSliderLookAndFeel(
    const double& normalizedMidPointValue, const double& currentNormalizedValue)
    : midpointNormalizedValue_(normalizedMidPointValue),
      initialNormalizedValue_(currentNormalizedValue) {
  setColour((int)KnobColourIds::dialFill, EclipsaColours::inactiveGrey);
  setColour((int)KnobColourIds::dialOutline, EclipsaColours::headingGrey);
  setColour((int)KnobColourIds::blueArc, EclipsaColours::controlBlue);
}

SkewedSliderLookAndFeel::~SkewedSliderLookAndFeel() {}

void SkewedSliderLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y,
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

  if (initializationCall_) {
    initializationCall_ = false;
    sliderPosProportional = initialNormalizedValue_;
  } else {
    initialNormalizedValue_ = sliderPosProportional;
  }

  float scaledSliderPosProportional =
      scaledSliderProportionalPos(sliderPosProportional);

  if (scaledSliderPosProportional > 1.f) {
    scaledSliderPosProportional = 1.f;
  }

  float endAngle =
      defaultAngle_ + angularDeflection(scaledSliderPosProportional);

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

double SkewedSliderLookAndFeel::angularDeflection(
    const double& normalizedValue) {
  return (normalizedValue - 0.5f) * (endAngle_ - startAngle_);
}

juce::Point<int> SkewedSliderLookAndFeel::getDialXY(const float& endAngle) {
  // ensure dial does not intersect the arc
  float multiple = 0.8f;
  int xOnArc = std::ceil(centerX_ + (multiple * radius_ * std::sin(endAngle)));
  int yOnArc = std::ceil(centerY_ - (multiple * radius_ * std::cos(endAngle)));
  return {xOnArc, yOnArc};
}

double SkewedSliderLookAndFeel::scaledSliderProportionalPos(
    const double& sliderPosProportional) {
  if (sliderPosProportional <= midpointNormalizedValue_) {
    double slope = 0.5f / midpointNormalizedValue_;
    return sliderPosProportional * slope;
  } else {
    double slope = (1.f - 0.5f) / (1.f - midpointNormalizedValue_);
    return (sliderPosProportional - midpointNormalizedValue_) * slope + 0.5f;
  }
}

ControlKnobSkewed::ControlKnobSkewed(const double& min, const double& max,
                                     const double& defaultValue,
                                     const double& currentValue,
                                     const juce::String& suffix)
    : min_(min),
      max_(max),
      defaultNormalizedValue_((defaultValue - min) / (max - min)),
      suffix_(suffix),
      lookAndFeel_(defaultNormalizedValue_, normalizeValue(currentValue)),
      juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag,
                   juce::Slider::NoTextBox) {
  setLookAndFeel(&lookAndFeel_);
  juce::Slider::setTextValueSuffix(suffix_);
  setRotaryParameters(startAngle_, endAngle_, true);
  juce::Slider::setRange(min_, max_, 1.f);
  juce::Slider::setValue(currentValue);
}

void ControlKnobSkewed::setValue(float newValue) {
  juce::Slider::setValue(newValue);
  repaint();
}

void ControlKnobSkewed::setValueUpdatedCallback(
    std::function<void(int)> callback) {
  valueChangedCallback_ = callback;
  valueUpdatedCallback_ = [this]() { valueChangedCallback_(getValue()); };
  juce::Slider::onValueChange = valueUpdatedCallback_;
}

float ControlKnobSkewed::normalizeValue(const float& value) {
  return (value - min_) / (max_ - min_);
}

void ControlKnobSkewed::mouseDown(const juce::MouseEvent& event) {
  if (event.mods.isAltDown() && isEnabled()) {
    // Option-click resets control to 0,
    juce::Slider::setValue(0);
    return;
  }
  juce::Slider::mouseDown(event);
}