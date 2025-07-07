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

#include "MixPresentationViewPort.h"

MixPresentationViewPort::MixPresentationViewPort(
    const juce::Uuid mixPresID, RepositoryCollection repos,
    ChannelMonitorData& channelMonitorData)
    : kmixPresID_(mixPresID),
      mixPresentationRepository_(&repos.mpRepo_),
      mixPresentationSoloMuteRepository_(&repos.mpSMRepo_),
      tab_(mixPresID, repos, channelMonitorData) {
  addAndMakeVisible(viewPort_);
  viewPort_.setViewedComponent(&tab_);
  viewPort_.setScrollBarsShown(true, false);
}

MixPresentationViewPort::~MixPresentationViewPort(){};

void MixPresentationViewPort::paint(juce::Graphics& g) {
  const auto bounds = getLocalBounds();
  viewPort_.setSize(bounds.getWidth(), bounds.getHeight());
  if (tab_.getNumOfAEStrips() <= tab_.kAEStripScrollThreshold) {
    tab_.setSize(bounds.getWidth(), bounds.getHeight());
  } else {
    tab_.setSize(bounds.getWidth(), tab_.calculateHeight());
  }
}

void MixPresentationViewPort::updateActiveMixPresentation() {
  tab_.updateActiveMixPresentation();
}