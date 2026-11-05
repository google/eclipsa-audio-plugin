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

juce::String exportProfileToString(const ExportProfile profile) {
  switch (profile) {
    case ExportProfile::kYouTube:
      return "YouTube";
    case ExportProfile::kAdvanced:
      return "Advanced";
  }
  return "Advanced";
}

ExportProfile exportProfileFromString(const juce::String& str) {
  for (const ExportProfile profile : kExportProfileOptions) {
    if (exportProfileToString(profile) == str) {
      return profile;
    }
  }
  return ExportProfile::kAdvanced;
}

ExportProfile exportProfileFromOptionIndex(const int index) {
  if (index < 0 || index >= static_cast<int>(kExportProfileOptions.size())) {
    return ExportProfile::kAdvanced;
  }
  return kExportProfileOptions[index];
}

int optionIndexForExportProfile(const ExportProfile profile) {
  for (int i = 0; i < static_cast<int>(kExportProfileOptions.size()); ++i) {
    if (kExportProfileOptions[i] == profile) {
      return i;
    }
  }
  jassertfalse;
  return 0;
}

RoomSetup::RoomSetup() : RepositoryItemBase({}), dawWarningDismissed_(false) {}

RoomSetup::RoomSetup(RoomLayout layout)
    : RepositoryItemBase({}),
      speakerLayout_(layout),
      dawWarningDismissed_(false) {}

RoomSetup::RoomSetup(const RoomLayout layout, const bool drawSpeakers,
                     const bool drawSpeakerLabels, const bool drawTracks,
                     const juce::String& currRoomView,
                     const bool dawWarningDismissed,
                     const ExportProfile exportProfile)
    : RepositoryItemBase({}),
      speakerLayout_(layout),
      drawSpeakers_(drawSpeakers),
      drawSpeakerLabels_(drawSpeakerLabels),
      drawTracks_(drawTracks),
      currentRoomView_(currRoomView),
      dawWarningDismissed_(dawWarningDismissed),
      exportProfile_(exportProfile) {}

juce::ValueTree RoomSetup::toValueTree() const {
  return {{kTreeType},
          {
              {kSpeakerLayout, speakerLayout_.getDescription()},
              {kDrawSpeakers, drawSpeakers_},
              {kDrawSpeakerLabels, drawSpeakerLabels_},
              {kDrawTracks, drawTracks_},
              {kCurrRoomView, currentRoomView_},
              {kDawWarningDismissed, dawWarningDismissed_},
              {kExportProfile, exportProfileToString(exportProfile_)},
          }};
}

RoomSetup RoomSetup::fromTree(const juce::ValueTree tree) {
  jassert(tree.hasProperty(kSpeakerLayout));
  bool dawWarningDismissed = tree.getProperty(kDawWarningDismissed, false);
  const ExportProfile exportProfile =
      exportProfileFromString(tree.getProperty(kExportProfile, {}).toString());
  return RoomSetup(fetchLayoutFromDescription(tree[kSpeakerLayout]),
                   tree[kDrawSpeakers], tree[kDrawSpeakerLabels],
                   tree[kDrawTracks], tree[kCurrRoomView], dawWarningDismissed,
                   exportProfile);
}