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

#include "RoomSetup.h"

RoomSetup::RoomSetup() : RepositoryItemBase({}), dawWarningDismissed_(false) {}

RoomSetup::RoomSetup(RoomLayout layout)
    : RepositoryItemBase({}),
      speakerLayout_(layout),
      dawWarningDismissed_(false) {}

RoomSetup::RoomSetup(const RoomLayout layout, const bool drawSpeakers,
                     const bool drawSpeakerLabels, const bool drawTracks,
                     const juce::String& currRoomView,
                     const bool dawWarningDismissed)
    : RepositoryItemBase({}),
      speakerLayout_(layout),
      drawSpeakers_(drawSpeakers),
      drawSpeakerLabels_(drawSpeakerLabels),
      drawTracks_(drawTracks),
      currentRoomView_(currRoomView),
      dawWarningDismissed_(dawWarningDismissed) {}

juce::ValueTree RoomSetup::toValueTree() const {
  return {{kTreeType},
          {
              {kSpeakerLayout, speakerLayout_.getDescription()},
              {kDrawSpeakers, drawSpeakers_},
              {kDrawSpeakerLabels, drawSpeakerLabels_},
              {kDrawTracks, drawTracks_},
              {kCurrRoomView, currentRoomView_},
              {kDawWarningDismissed, dawWarningDismissed_},
          }};
}

RoomSetup RoomSetup::fromTree(const juce::ValueTree tree) {
  jassert(tree.hasProperty(kSpeakerLayout));
  bool dawWarningDismissed = tree.getProperty(kDawWarningDismissed, false);
  return RoomSetup(fetchLayoutFromDescription(tree[kSpeakerLayout]),
                   tree[kDrawSpeakers], tree[kDrawSpeakerLabels],
                   tree[kDrawTracks], tree[kCurrRoomView], dawWarningDismissed);
}