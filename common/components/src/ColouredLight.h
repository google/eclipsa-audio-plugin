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
#include "components/src/EclipsaColours.h"
#include "components/src/MainEditor.h"

class ColouredLight : public juce::Component {
  std::vector<juce::Colour> colours_;
  int currentColour;

 public:
  ColouredLight() : juce::Component(), currentColour(0) {}

  ColouredLight(juce::Colour colour) : juce::Component(), currentColour(0) {
    colours_.push_back(colour);
  }

  ~ColouredLight() override { setLookAndFeel(nullptr); }

  void addColour(juce::Colour colour) { colours_.push_back(colour); }

  void setColour(int index) { currentColour = index; }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();
    bounds.reduce(bounds.getWidth() * kRadialReduction,
                  bounds.getHeight() * kRadialReduction);

    // Draw a small circle in the centre of the component
    g.setColour(colours_[currentColour]);
    g.fillEllipse(bounds.toFloat());

    // Draw an outline
    g.setColour(EclipsaColours::buttonOutlineGrey);
    g.drawEllipse(bounds.toFloat(), 2.0f);
  }

 private:
  const float kRadialReduction = 0.05f;
};