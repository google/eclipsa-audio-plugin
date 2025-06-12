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
#include <components/components.h>

#include "MixMonitoringScreen.h"
#include "PresentationMonitorScreen.h"
#include "RoomMonitoringScreen.h"

class MonitorScreen : public juce::Component {
  PresentationMonitorScreen presentationMonitorScreen_;
  RoomMonitoringScreen roomMonitoringScreen_;
  MixMonitoringScreen mixMonitoringScreen_;

 public:
  MonitorScreen(RepositoryCollection repos, SpeakerMonitorData& data,
                MainEditor& editor,
                ChannelMonitorProcessor* channelMonitorProcessor,
                int totalChannelCount)
      : presentationMonitorScreen_(
            editor, &repos.aeRepo_, &repos.audioElementSpatialLayoutRepo_,
            &repos.mpRepo_, &repos.mpSMRepo_, &repos.chGainRepo_,
            &repos.activeMPRepo_, channelMonitorProcessor, &repos.fioRepo_,
            totalChannelCount),
        roomMonitoringScreen_(repos, data, editor),
        mixMonitoringScreen_(repos, data) {}

  void paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    // Split the bounds into the room view and monitoring view, drawing a
    // seperator line between them
    g.setColour(juce::Colour(63, 73, 72));
    auto roomBounds = bounds.removeFromLeft((bounds.getWidth() / 2) - 1);
    g.drawRect(bounds.removeFromLeft(2));
    auto monitoringBounds = bounds;

    // Seperate the monitoring view into the mix presentation and speaker
    // monitoring sections
    auto speakerMonitoringBounds = bounds.removeFromTop(bounds.getHeight() / 2);
    speakerMonitoringBounds.removeFromBottom(
        10);  // for gap between the two monitoring screens
    auto mixPresentationBounds = bounds;

    // Call the various sub-screens to draw their components
    addAndMakeVisible(presentationMonitorScreen_);
    presentationMonitorScreen_.setBounds(mixPresentationBounds);

    addAndMakeVisible(mixMonitoringScreen_);
    mixMonitoringScreen_.setBounds(speakerMonitoringBounds);

    addAndMakeVisible(roomMonitoringScreen_);
    roomMonitoringScreen_.setBounds(roomBounds);
  }
};