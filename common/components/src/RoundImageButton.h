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

#include <memory>

#include "components/icons/svg/SvgIconComponent.h"
#include "components/src/EclipsaColours.h"

class RoundImageButton : public juce::Button {
 public:
  RoundImageButton(const juce::String& buttonName, SvgMap::Icon svgIcon)
      : juce::Button(buttonName),
        icon_(std::make_unique<SvgIconComponent>(svgIcon)) {
    setClickingTogglesState(false);  // only acts like a push button
    addAndMakeVisible(icon_.get());
  }

  void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                   bool shouldDrawButtonAsDown) override {
    auto bounds = getLocalBounds().toFloat();
    auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto circleBounds = bounds.withSizeKeepingCentre(diameter, diameter);

    // Shadow
    auto shadowBounds = circleBounds.expanded(2.0f);
    juce::Path shadowPath;
    shadowPath.addEllipse(shadowBounds);
    juce::DropShadow shadow(EclipsaColours::backgroundOffBlack.withAlpha(0.6f),
                            6, juce::Point<int>(0, 1.5f));
    shadow.drawForPath(g, shadowPath);

    juce::Colour baseColour = EclipsaColours::selectCyan;
    if (isEnabled()) {
      if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker(0.4f);
      else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.4f);
    } else {
      baseColour = juce::Colours::darkgrey;
    }

    g.setColour(baseColour);
    g.fillEllipse(circleBounds);
  }

  void resized() override {
    auto outerBounds = getLocalBounds();
    auto diameter = juce::jmin(outerBounds.getWidth(), outerBounds.getHeight());
    int iconSize = static_cast<int>(diameter * 0.5f);
    juce::Rectangle<int> iconBounds(iconSize, iconSize);
    iconBounds = iconBounds.withCentre(outerBounds.getCentre());

    icon_->setBounds(iconBounds);
  }

 private:
  std::unique_ptr<SvgIconComponent> icon_;
};
