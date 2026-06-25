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

#include <memory>

#include "MixAEContainer.h"

// this class is used to display audio elements within the MixPresentation
// Editor Screen's viewport
class AEContainerSet : public juce::Component {
 public:
  AEContainerSet(
      std::map<juce::Uuid, std::unique_ptr<MixAEContainer>>* containers)
      : containers_(containers) {}

  ~AEContainerSet() {}

  int getNumContainers() { return containers_->size(); }

  int calculateContainerHeight() {
    int total = 0;
    for (auto& pair : *containers_)
      total += kMixContainerSpacing_ + pair.second->getPreferredHeight();
    return total;
  }

  const int getViewPortMaxHeight() {
    return kMaxContainerThreshold *
           (kMixContainerSpacing_ + kMixContainerHeight_);
  }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();
    bounds.removeFromRight(16);
    for (auto& pair : *containers_) {
      bounds.removeFromTop(kMixContainerSpacing_);
      addAndMakeVisible(pair.second.get());
      pair.second.get()->setBounds(
          bounds.removeFromTop(pair.second->getPreferredHeight()));
    }
  }

  const int kMaxContainerThreshold = 5;

 private:
  const int kMixContainerSpacing_ = 20;
  const int kMixContainerHeight_ = 32;
  std::map<juce::Uuid, std::unique_ptr<MixAEContainer>>* containers_;
};