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

// Viewport class which updates a secondard viewport as it scrolls
class LinkedViewport : public juce::Viewport {
  juce::Viewport* linkedViewport_;

 public:
  LinkedViewport(juce::Viewport* linkedComponent)
      : juce::Viewport(), linkedViewport_(linkedComponent) {}

  void visibleAreaChanged(const juce::Rectangle<int>& newVisibleArea) override {
    linkedViewport_->setViewPosition(getViewPositionX(), getViewPositionY());
  }
};

// Viewport container class which allows adding components for rendering
// rather than using addAndMakeVisible to create child components, which
// seems to break viewports for some reason. Components added using
// addAndMakeVisible don't appear to scroll at the same rate as non
// addAndMakeVisible components, so this is needed when linking viewports
class VerticalComponent {
 public:
  juce::Component* component;
  int topBound;

  VerticalComponent(juce::Component* component, int topBound = 0)
      : component(component), topBound(topBound) {}
};

class VerticalViewportContainer : public juce::Component {
  std::vector<VerticalComponent> components_;

 public:
  void clear() { components_.clear(); }

  void addComponent(juce::Component* component, int topBound) {
    components_.push_back(VerticalComponent(component, topBound));
  }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();
    for (int i = 0; i < components_.size(); i++) {
      // If requested bounds == 0 then just give all remaining bounds
      juce::Rectangle<int> componentBound =
          components_[i].topBound == 0
              ? bounds
              : bounds.removeFromTop(components_[i].topBound);

      // If null, just add padding by removing bounds
      if (components_[i].component != nullptr) {
        this->addAndMakeVisible(components_[i].component);
        components_[i].component->setBounds(componentBound);
      }
    }
  }
};

// Viewport container class which allows adding components for rendering
// rather than using addAndMakeVisible to create child components, which
// seems to break viewports for some reason. Components added using
// addAndMakeVisible don't appear to scroll at the same rate as non
// addAndMakeVisible components, so this is needed when linking viewports
struct HorizontalComponent {
  juce::Component* component;
  int leftBound;
  bool centered;

  HorizontalComponent(juce::Component* component, int leftBound = 0,
                      bool centered = false)
      : component(component), leftBound(leftBound), centered(centered) {}
};

class HorizontalViewportContainer : public juce::Component {
  std::vector<HorizontalComponent> components_;

 public:
  void clear() { components_.clear(); }

  void addComponent(juce::Component* component, int leftBound,
                    bool centered = false) {
    components_.push_back(HorizontalComponent(component, leftBound, centered));
  }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();

    for (int i = 0; i < components_.size(); i++) {
      // If requested area is 0, then just give all remaning bounds
      juce::Rectangle<int> componentBound =
          components_[i].leftBound == 0
              ? bounds
              : bounds.removeFromLeft(components_[i].leftBound);

      // If requested centered, then center the component
      if (components_[i].centered) {
        componentBound =
            componentBound.removeFromTop(bounds.getHeight() / 7 * 5)
                .removeFromBottom(bounds.getHeight() / 7 * 3);
      }

      // If the component is null, just use the removed bounds as padding
      if (components_[i].component != nullptr) {
        this->addAndMakeVisible(components_[i].component);
        components_[i].component->setBounds(componentBound);
      }
    }
  }
};