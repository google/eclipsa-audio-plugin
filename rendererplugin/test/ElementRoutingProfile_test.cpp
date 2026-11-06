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

#include <gtest/gtest.h>

#include "../src/RendererProcessor.h"
#include "../src/screens/ElementRoutingScreen.h"
#include "data_repository/implementation/ExportProfileListener.h"

namespace {
bool hasExpandedLayout(
    const std::vector<Speakers::AudioElementSpeakerLayout>& layouts) {
  for (const auto& layout : layouts) {
    if (layout.isExpandedLayout()) {
      return true;
    }
  }
  return false;
}

// Applies a RoomSetup edit through the repository, as the UI does.
template <typename Edit>
void editRoomSetup(RoomSetupRepository& repo, Edit edit) {
  RoomSetup setup = repo.get();
  edit(setup);
  repo.update(setup);
}
}  // namespace

// Under YouTube, Base Enhanced is capped to Base; lower profiles pass through.
TEST(EffectiveProfile, youtube_caps_at_base) {
  const ExportProfile kYt = ExportProfile::kYouTube;
  EXPECT_EQ(ElementRoutingScreen::effectiveProfile(kYt, FileProfile::SIMPLE),
            FileProfile::SIMPLE);
  EXPECT_EQ(ElementRoutingScreen::effectiveProfile(kYt, FileProfile::BASE),
            FileProfile::BASE);
  EXPECT_EQ(
      ElementRoutingScreen::effectiveProfile(kYt, FileProfile::BASE_ENHANCED),
      FileProfile::BASE);
}

// Under Advanced, every profile is returned unchanged.
TEST(EffectiveProfile, advanced_is_unchanged) {
  for (const FileProfile profile :
       {FileProfile::SIMPLE, FileProfile::BASE, FileProfile::BASE_ENHANCED}) {
    EXPECT_EQ(ElementRoutingScreen::effectiveProfile(ExportProfile::kAdvanced,
                                                     profile),
              profile);
  }
}

// Under YouTube with Base Enhanced selected, new audio elements get Base's
// limits: 18 channels, 2 audio elements and no expanded layouts.
TEST(RoutingLayoutFilter, youtube_base_enhanced_offers_base) {
  const FileProfile effective = ElementRoutingScreen::effectiveProfile(
      ExportProfile::kYouTube, FileProfile::BASE_ENHANCED);
  const auto layouts = ElementRoutingScreen::layoutsForProfile(effective);
  EXPECT_FALSE(hasExpandedLayout(layouts));
  EXPECT_EQ(layouts.size(),
            ElementRoutingScreen::layoutsForProfile(FileProfile::BASE).size());
  EXPECT_EQ(ElementRoutingScreen::remainingChannels(effective, 0), 18);
  EXPECT_EQ(ElementRoutingScreen::remainingAudioElements(effective, 0), 2);
  // One stereo element in the session.
  EXPECT_EQ(ElementRoutingScreen::remainingChannels(effective, 2), 16);
  EXPECT_EQ(ElementRoutingScreen::remainingAudioElements(effective, 1), 1);
}

// Under Advanced with Base Enhanced selected, the limits are today's: 28
// channels, 28 audio elements and the expanded layouts.
TEST(RoutingLayoutFilter, advanced_base_enhanced_is_unchanged) {
  const FileProfile effective = ElementRoutingScreen::effectiveProfile(
      ExportProfile::kAdvanced, FileProfile::BASE_ENHANCED);
  const auto layouts = ElementRoutingScreen::layoutsForProfile(effective);
  EXPECT_TRUE(hasExpandedLayout(layouts));
  EXPECT_EQ(layouts.size(), 26u);
  EXPECT_EQ(ElementRoutingScreen::remainingChannels(effective, 0), 28);
  EXPECT_EQ(ElementRoutingScreen::remainingAudioElements(effective, 0), 28);
  EXPECT_EQ(ElementRoutingScreen::remainingChannels(effective, 2), 26);
  EXPECT_EQ(ElementRoutingScreen::remainingAudioElements(effective, 1), 27);
}

// Room view and draw toggle edits on the room setup tree do not reach the
// export profile handler; an export profile edit does.
TEST(RoutingProfileListener, reacts_only_to_export_profile) {
  RoomSetupRepository repo(juce::ValueTree{RoomSetup::kTreeType});
  int calls = 0;
  ExportProfileListener listener(&repo, [&calls]() { ++calls; });

  editRoomSetup(repo, [](RoomSetup& s) { s.setCurrentRoomView("Top"); });
  editRoomSetup(repo, [](RoomSetup& s) { s.setDrawTracks(true); });
  EXPECT_EQ(calls, 0);

  editRoomSetup(
      repo, [](RoomSetup& s) { s.setExportProfile(ExportProfile::kAdvanced); });
  EXPECT_EQ(calls, 1);
}

// The filter also requires the room setup tree type, so an export_profile
// property on another tree is ignored.
TEST(RoutingProfileListener, filter_requires_room_setup_tree) {
  EXPECT_TRUE(ExportProfileListener::isExportProfileChange(
      juce::ValueTree{RoomSetup::kTreeType}, RoomSetup::kExportProfile));
  EXPECT_FALSE(ExportProfileListener::isExportProfileChange(
      juce::ValueTree{"other"}, RoomSetup::kExportProfile));
  EXPECT_FALSE(ExportProfileListener::isExportProfileChange(
      juce::ValueTree{RoomSetup::kTreeType}, RoomSetup::kCurrRoomView));
}
