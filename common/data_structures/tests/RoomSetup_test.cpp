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