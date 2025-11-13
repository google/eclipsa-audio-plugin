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

#include "components/icons/svg/SvgIconLookup.h"

class SvgIconComponent : public juce::Component {
 public:
  SvgIconComponent(const SvgMap::Icon icon)
      : icon_(icon),
        svgDrawable_(juce::Drawable::createFromSVG(
            *juce::parseXML(SvgMap::get(icon).data()))) {
    setInterceptsMouseClicks(false, false);
  };

  void paint(juce::Graphics& g) override {
    svgDrawable_->drawWithin(g, getLocalBounds().toFloat(),
                             juce::RectanglePlacement::centred, 1.0f);
  }

 private:
  SvgMap::Icon icon_;
  std::unique_ptr<juce::Drawable> svgDrawable_;
};