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
#ifdef _WIN32
#include <errno.h>
#else
#include <sys/errno.h>
#endif

#include "../components.h"

class SliderButton : public juce::ToggleButton {
 public:
  SliderButton() {}

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds().toFloat();
    auto isOn = getToggleState();

    // Colours to be used for the on/off states
    juce::Colour backgroundColour =
        isOn ? juce::Colour(128, 213, 212) : juce::Colour(47, 54, 54);
    juce::Colour toggleColour =
        isOn ? juce::Colour(0, 55, 55) : juce::Colour(136, 147, 146);
    juce::Colour edgeColour =
        isOn ? juce::Colour(128, 213, 212) : juce::Colour(136, 147, 146);
    juce::Colour checkmarkColour =
        isOn ? juce::Colour(128, 213, 212) : juce::Colour(47, 54, 54);

    // Draw background
    g.setColour(backgroundColour);
    g.fillRoundedRectangle(bounds, bounds.getHeight() / 2);
    g.setColour(edgeColour);
    g.drawRoundedRectangle(bounds, bounds.getHeight() / 2, 2.0f);

    // Draw toggle
    auto toggleBounds = bounds.reduced(bounds.getHeight() * 0.1f);
    auto toggleDiameter = toggleBounds.getHeight();
    auto toggleX =
        isOn ? (bounds.getRight() - toggleDiameter) : bounds.getX() + 2;
    auto toggleY = toggleBounds.getY();

    g.setColour(toggleColour);
    g.fillEllipse(toggleX, toggleY, toggleDiameter, toggleDiameter);

    // Draw checkmark if toggle is on and X if toggle is off
    auto checkmarkBounds = juce::Rectangle<float>(
        toggleX, toggleY, toggleDiameter, toggleDiameter);

    if (isOn) {
      g.setColour(checkmarkColour);
      g.drawLine(checkmarkBounds.getX() + checkmarkBounds.getWidth() * 0.3f,
                 checkmarkBounds.getY() + checkmarkBounds.getHeight() * 0.5f,
                 checkmarkBounds.getX() + checkmarkBounds.getWidth() * 0.45f,
                 checkmarkBounds.getY() + checkmarkBounds.getHeight() * 0.7f,
                 2.0f);
      g.drawLine(checkmarkBounds.getX() + checkmarkBounds.getWidth() * 0.45f,
                 checkmarkBounds.getY() + checkmarkBounds.getHeight() * 0.7f,
                 checkmarkBounds.getX() + checkmarkBounds.getWidth() * 0.7f,
                 checkmarkBounds.getY() + checkmarkBounds.getHeight() * 0.3f,
                 2.0f);
    } else {
      g.setColour(checkmarkColour);
      g.drawLine(checkmarkBounds.getX() + checkmarkBounds.getWidth() * 0.3f,
                 checkmarkBounds.getY() + checkmarkBounds.getHeight() * 0.3f,
                 checkmarkBounds.getX() + checkmarkBounds.getWidth() * 0.7f,
                 checkmarkBounds.getY() + checkmarkBounds.getHeight() * 0.7f,
                 2.0f);
      g.drawLine(checkmarkBounds.getX() + checkmarkBounds.getWidth() * 0.3f,
                 checkmarkBounds.getY() + checkmarkBounds.getHeight() * 0.7f,
                 checkmarkBounds.getX() + checkmarkBounds.getWidth() * 0.7f,
                 checkmarkBounds.getY() + checkmarkBounds.getHeight() * 0.3f,
                 2.0f);
    }
  }

  juce::Rectangle<int> getLocalBounds() const {
    auto bounds = ToggleButton::getLocalBounds();
    return bounds.reduced(bounds.getHeight() * 0.1f);
  }
};
