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

#include "../src/RoomSetup.h"

#include <gtest/gtest.h>
#include <juce_data_structures/juce_data_structures.h>

#include "substream_rdr/substream_rdr_utils/Speakers.h"

TEST(test_room_setup, default_value) {
  RoomSetup setup;
  ASSERT_EQ(setup.getSpeakerLayout(), speakerLayoutConfigurationOptions[0]);
}

TEST(test_room_setup, from_value_tree) {
  const RoomLayout layout = RoomLayout(Speakers::kStereo, "stereo");
  const juce::ValueTree tree{
      RoomSetup::kTreeType,
      {{RoomSetup::kSpeakerLayout, layout.getDescription()}}};
  const RoomSetup roomSetup = RoomSetup::fromTree(tree);
  ASSERT_EQ(roomSetup.getSpeakerLayout(), layout);
}

TEST(test_room_setup, to_value_tree) {
  const RoomLayout layout = RoomLayout(Speakers::kStereo, "stereo");
  const RoomSetup setup(layout);
  const juce::ValueTree tree(setup.toValueTree());
  ASSERT_TRUE(tree.hasProperty(RoomSetup::kSpeakerLayout));
  ASSERT_EQ(tree.getType(), RoomSetup::kTreeType);
  ASSERT_EQ(tree[RoomSetup::kSpeakerLayout], layout.getDescription());
}

TEST(test_room_setup, daw_warning_dismissal_persistence) {
  // 1. Create RoomSetup with warning dismissed
  RoomSetup roomSetup1;
  ASSERT_FALSE(
      roomSetup1.getDawWarningDismissed());  // Should be false by default

  roomSetup1.setDawWarningDismissed(true);
  ASSERT_TRUE(roomSetup1.getDawWarningDismissed());

  // 2. Serialize to ValueTree
  juce::ValueTree stateTree = roomSetup1.toValueTree();

  // 3. Create new RoomSetup from serialized state
  RoomSetup roomSetup2 = RoomSetup::fromTree(stateTree);

  // 4. Verify the state was persisted
  ASSERT_TRUE(roomSetup2.getDawWarningDismissed());
}
// A new RoomSetup starts on the YouTube profile.
TEST(ExportProfile, default_is_youtube) {
  const RoomSetup setup;
  EXPECT_EQ(setup.getExportProfile(), ExportProfile::kYouTube);
  EXPECT_EQ(RoomSetup(speakerLayoutConfigurationOptions[2]).getExportProfile(),
            ExportProfile::kYouTube);
}

// Each profile survives a toValueTree/fromTree round trip.
TEST(ExportProfile, round_trip_preserves_value) {
  for (const ExportProfile profile :
       {ExportProfile::kYouTube, ExportProfile::kAdvanced}) {
    RoomSetup setup;
    setup.setExportProfile(profile);
    const juce::ValueTree tree = setup.toValueTree();
    EXPECT_EQ(tree[RoomSetup::kExportProfile].toString(),
              exportProfileToString(profile));
    EXPECT_EQ(RoomSetup::fromTree(tree).getExportProfile(), profile);
  }
}

// A tree saved before the profile existed loads as Advanced.
TEST(ExportProfile, missing_value_loads_advanced) {
  juce::ValueTree tree = RoomSetup().toValueTree();
  tree.removeProperty(RoomSetup::kExportProfile, nullptr);
  ASSERT_FALSE(tree.hasProperty(RoomSetup::kExportProfile));
  EXPECT_EQ(RoomSetup::fromTree(tree).getExportProfile(),
            ExportProfile::kAdvanced);
}

// An unrecognized stored string loads as Advanced.
TEST(ExportProfile, unknown_value_loads_advanced) {
  juce::ValueTree tree = RoomSetup().toValueTree();
  tree.setProperty(RoomSetup::kExportProfile, "Vimeo", nullptr);
  EXPECT_EQ(RoomSetup::fromTree(tree).getExportProfile(),
            ExportProfile::kAdvanced);
}

// Dropdown index 0 is YouTube and index 1 is Advanced.
TEST(ExportProfileOption, index_maps_to_profile) {
  EXPECT_EQ(exportProfileFromOptionIndex(0), ExportProfile::kYouTube);
  EXPECT_EQ(exportProfileFromOptionIndex(1), ExportProfile::kAdvanced);
}

// Each profile maps back to the dropdown index it came from.
TEST(ExportProfileOption, profile_maps_to_index) {
  EXPECT_EQ(optionIndexForExportProfile(ExportProfile::kYouTube), 0);
  EXPECT_EQ(optionIndexForExportProfile(ExportProfile::kAdvanced), 1);
  for (int i = 0; i < static_cast<int>(kExportProfileOptions.size()); ++i) {
    EXPECT_EQ(optionIndexForExportProfile(exportProfileFromOptionIndex(i)), i);
  }
}

// An index outside the dropdown, such as none selected, maps to Advanced.
TEST(ExportProfileOption, out_of_range_index_is_advanced) {
  EXPECT_EQ(exportProfileFromOptionIndex(-1), ExportProfile::kAdvanced);
  EXPECT_EQ(exportProfileFromOptionIndex(2), ExportProfile::kAdvanced);
}

// The layout buttons list Binaural first, then every other layout in
// configuration order.
TEST(test_room_setup, display_order_puts_binaural_first) {
  const std::vector<juce::String> expected = {"Binaural", "Stereo", "3.1.2",
                                              "5.1",      "5.1.2",  "5.1.4",
                                              "7.1",      "7.1.2",  "7.1.4"};
  std::vector<juce::String> actual;
  for (const RoomLayout& layout : speakerLayoutDisplayOrder()) {
    actual.push_back(layout.getDescription());
  }
  EXPECT_EQ(actual, expected);
}
