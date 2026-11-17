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

#include "../src/AudioElementSpatialLayout.h"

#include <gtest/gtest.h>

#include "substream_rdr/substream_rdr_utils/Speakers.h"

TEST(test_audioElementSpatialLayout, from_value_tree) {
  const juce::Uuid id;
  const juce::String name = "test2";
  const juce::Uuid audioElementId;
  const juce::int32 firstChannel = 0;
  const juce::int32 totalChannels = 1;

  const juce::ValueTree tree{
      AudioElementSpatialLayout::kTreeType,
      {
          {AudioElementSpatialLayout::kId, id.toString()},
          {AudioElementSpatialLayout::kName, name},
          {AudioElementSpatialLayout::kaudioElement, audioElementId.toString()},
          {AudioElementSpatialLayout::kFirstChannel, firstChannel},
          {AudioElementSpatialLayout::kLayout, totalChannels},
      }};

  const AudioElementSpatialLayout audioElementSpatialLayout =
      AudioElementSpatialLayout::fromTree(tree);

  EXPECT_EQ(audioElementSpatialLayout.getName(), name);
  EXPECT_EQ(audioElementSpatialLayout.getId(), id);
  EXPECT_EQ(audioElementSpatialLayout.getAudioElementId(), audioElementId);
  EXPECT_EQ(audioElementSpatialLayout.getFirstChannel(), firstChannel);
  EXPECT_EQ((int)audioElementSpatialLayout.getChannelLayout(), totalChannels);
  // default elevation value should be none
  EXPECT_EQ(audioElementSpatialLayout.getElevation(),
            AudioElementSpatialLayout::Elevation::kNone);

  const AudioElementSpatialLayout::Elevation elevation =
      AudioElementSpatialLayout::Elevation::kDome;
  const juce::ValueTree tree_2{
      AudioElementSpatialLayout::kTreeType,
      {
          {AudioElementSpatialLayout::kId, id.toString()},
          {AudioElementSpatialLayout::kName, name},
          {AudioElementSpatialLayout::kaudioElement, audioElementId.toString()},
          {AudioElementSpatialLayout::kFirstChannel, firstChannel},
          {AudioElementSpatialLayout::kLayout, totalChannels},
          {AudioElementSpatialLayout::kElevation, static_cast<int>(elevation)},
      }};

  const AudioElementSpatialLayout audioElementSpatialLayout2 =
      AudioElementSpatialLayout::fromTree(tree_2);
  EXPECT_EQ(audioElementSpatialLayout2.getElevation(), elevation);
}

TEST(test_audioElementSpatialLayout, to_value_tree) {
  const juce::Uuid id;
  const juce::String name = "test";
  const juce::Uuid audioElementId;
  const int firstChannel = 0;
  const int totalChannels = 1;
  const AudioElementSpatialLayout::Elevation elevation =
      AudioElementSpatialLayout::Elevation::kArch;

  const AudioElementSpatialLayout audioElementSpatialLayout(
      id, name, audioElementId, firstChannel,
      Speakers::AudioElementSpeakerLayout(totalChannels), true, elevation);

  const juce::ValueTree tree = audioElementSpatialLayout.toValueTree();

  EXPECT_EQ(tree[AudioElementSpatialLayout::kId], id.toString());
  EXPECT_EQ(tree[AudioElementSpatialLayout::kName], name);
  EXPECT_EQ(tree[AudioElementSpatialLayout::kaudioElement],
            audioElementId.toString());
  EXPECT_EQ(static_cast<int>(tree[AudioElementSpatialLayout::kFirstChannel]),
            firstChannel);
  EXPECT_EQ(static_cast<int>(tree[AudioElementSpatialLayout::kLayout]),
            totalChannels);
  EXPECT_EQ((bool)tree[AudioElementSpatialLayout::kPanningEnabled], true);
  EXPECT_EQ(static_cast<int>(tree[AudioElementSpatialLayout::kElevation]),
            static_cast<int>(elevation));
}

TEST(test_audioElementSpatialLayout, equality) {
  const juce::Uuid id;
  const juce::String name = "test";
  const juce::Uuid audioElementId;
  const int firstChannel = 0;
  const Speakers::AudioElementSpeakerLayout layout = Speakers::kStereo;
  const AudioElementSpatialLayout::Elevation elevation =
      AudioElementSpatialLayout::Elevation::kArch;

  const AudioElementSpatialLayout audioElementSpatialLayout1(
      id, name, audioElementId, firstChannel, layout, true, elevation);
  const AudioElementSpatialLayout audioElementSpatialLayout2(
      id, name, audioElementId, firstChannel, layout, true, elevation);

  EXPECT_EQ(audioElementSpatialLayout1, audioElementSpatialLayout2);

  const juce::Uuid id2;
  const AudioElementSpatialLayout audioElementSpatialLayout3(
      id2, name, audioElementId, firstChannel, layout, false, elevation);
  EXPECT_NE(audioElementSpatialLayout1, audioElementSpatialLayout3);

  const juce::Uuid audioElementId2;
  const AudioElementSpatialLayout audioElementSpatialLayout4(
      id, name, audioElementId2, firstChannel, layout, false, elevation);
  EXPECT_NE(audioElementSpatialLayout3, audioElementSpatialLayout4);
}
// kNameIsCustom survives toValueTree then fromTree in both states.
TEST(test_audioElementSpatialLayout, name_is_custom_round_trips) {
  for (const bool nameIsCustom : {true, false}) {
    AudioElementSpatialLayout layout(juce::Uuid(), "Lead Vocal", juce::Uuid(),
                                     0, Speakers::kStereo);
    layout.setNameIsCustom(nameIsCustom);

    const juce::ValueTree tree = layout.toValueTree();
    EXPECT_EQ(static_cast<bool>(tree[AudioElementSpatialLayout::kNameIsCustom]),
              nameIsCustom);
    EXPECT_EQ(AudioElementSpatialLayout::fromTree(tree).isNameCustom(),
              nameIsCustom);
  }
}

// A tree saved before kNameIsCustom existed loads as not custom.
TEST(test_audioElementSpatialLayout, name_is_custom_defaults_false) {
  const juce::ValueTree tree{
      AudioElementSpatialLayout::kTreeType,
      {
          {AudioElementSpatialLayout::kId, juce::Uuid().toString()},
          {AudioElementSpatialLayout::kName, "Legacy"},
          {AudioElementSpatialLayout::kaudioElement, juce::Uuid().toString()},
          {AudioElementSpatialLayout::kFirstChannel, 0},
          {AudioElementSpatialLayout::kLayout, 1},
      }};
  ASSERT_FALSE(tree.hasProperty(AudioElementSpatialLayout::kNameIsCustom));

  EXPECT_FALSE(AudioElementSpatialLayout::fromTree(tree).isNameCustom());
}

// copyValuesFrom carries kNameIsCustom across.
TEST(test_audioElementSpatialLayout, copy_values_keeps_name_is_custom) {
  AudioElementSpatialLayout source(juce::Uuid(), "Custom", juce::Uuid(), 0,
                                   Speakers::kStereo);
  source.setNameIsCustom(true);
  AudioElementSpatialLayout target;

  target.copyValuesFrom(source);

  EXPECT_TRUE(target.isNameCustom());
}
