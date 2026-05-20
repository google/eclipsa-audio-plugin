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

#include "PresentationEditorViewPort.h"

#include <memory>

#include "components/src/MixAEContainer.h"
#include "data_structures/src/MixPresentation.h"

PresentationEditorViewPort::PresentationEditorViewPort(
    std::map<juce::Uuid, std::unique_ptr<MixAEContainer>>* container)
    : set_(container) {
  addAndMakeVisible(viewPort_);
  viewPort_.setViewedComponent(&set_);
  viewPort_.setScrollBarsShown(true, false);
}

void PresentationEditorViewPort::paint(juce::Graphics& g) {
  const auto bounds = getLocalBounds();
  viewPort_.setSize(bounds.getWidth(), bounds.getHeight());
  set_.setSize(bounds.getWidth(),
               std::max(bounds.getHeight(), set_.calculateContainerHeight()));
}
