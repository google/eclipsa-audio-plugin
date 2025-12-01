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

#include "../src/AudioElement.h"

#include <gtest/gtest.h>
#include <juce_data_structures/juce_data_structures.h>

#include "audio_element.pb.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

TEST(test_audio_element, from_value_tree) {
  const juce::Uuid id;
  const juce::String name = "test";
  const juce::String desc = "desc";
  const Speakers::AudioElementSpeakerLayout config = Speakers::kStereo;
  const int firstChannel = 0;
  const AudioElement::AudioElement_t aeType =
      AudioElement::AudioElement_t::AUDIO_ELEMENT_CHANNEL_BASED;
  const AudioElement::LoudspeakerLayout_t layout =
      AudioElement::LoudspeakerLayout_t::LOUDSPEAKER_LAYOUT_STEREO;
  const int numSubstreams = 0;
  const int id32 = 0;

  const juce::ValueTree tree{AudioElement::kTreeType,
                             {
                                 {AudioElement::kId, id.toString()},
                                 {AudioElement::kName, name},
                                 {AudioElement::kDescription, desc},
                                 {AudioElement::kChannelConfig, (int)config},
                                 {AudioElement::kFirstChannel, firstChannel},
                             }};

  const AudioElement element = AudioElement::fromTree(tree);

  EXPECT_EQ(element.getName(), name);
  EXPECT_EQ(element.getId(), id);
  EXPECT_EQ(element.getChannelConfig(), config);
  EXPECT_EQ(element.getFirstChannel(), firstChannel);
}

TEST(test_audio_element, to_value_tree) {
  const juce::Uuid id;
  const juce::String name = "test";
  const juce::String desc = "desc";
  const Speakers::AudioElementSpeakerLayout config = Speakers::kStereo;
  const int firstChannel = 0;

  const AudioElement element(id, name, desc, config, firstChannel);

  const juce::ValueTree tree = element.toValueTree();

  EXPECT_EQ(tree[AudioElement::kId], id.toString());
  EXPECT_EQ(tree[AudioElement::kName], name);
  EXPECT_EQ(tree[AudioElement::kDescription], desc);
  // EXPECT_EQ(tree[AudioElement::kChannelConfig], (int)config);
  // EXPECT_EQ(tree[AudioElement::kFirstChannel], firstChannel);
}

TEST(test_audio_element, validity) {
  const juce::String name = "test";
  const Speakers::AudioElementSpeakerLayout config = Speakers::kStereo;
  const juce::Uuid id;
  const AudioElement element1{};
  const AudioElement element2{id};
  AudioElement element3{id};
  element3.setName(name);
  AudioElement element4{id};
  element4.setChannelConfig(config);
  const AudioElement element5{id, name, config, 0};

  ASSERT_FALSE(element1.isInitialized());
  ASSERT_FALSE(element2.isInitialized());
  ASSERT_TRUE(element3.isInitialized());
  ASSERT_FALSE(element4.isInitialized());
  ASSERT_TRUE(element5.isInitialized());
}

TEST(test_audio_element, equality) {
  const juce::String name = "test";
  const Speakers::AudioElementSpeakerLayout config = Speakers::kStereo;
  const juce::Uuid id;
  const AudioElement element1{id, name, config, 0};
  const AudioElement element2(element1);
  ASSERT_EQ(element1, element2);

  const AudioElement element3{id, "name", config, 0};
  ASSERT_NE(element1, element3);

  const AudioElement element4{{}, name, config, 0};
  ASSERT_NE(element1, element4);

  const AudioElement element5{id, name, Speakers::kMono, 0};
  ASSERT_NE(element1, element5);
}

TEST(test_audio_element, iamf_md_population_cb_ae) {
  const AudioElement::AudioElement_t kAeType =
      AudioElement::AudioElement_t::AUDIO_ELEMENT_CHANNEL_BASED;
  const Speakers::AudioElementSpeakerLayout kLayout = Speakers::kStereo;
  const int kNumSubstreams = 1;  // Stereo is 1 coupled substream
  const int kId32 = 0;
  int minSubstreamId = 0;

  // Configure an AudioElement.
  AudioElement ae;
  ae.setChannelConfig(kLayout);

  // Create a protobuf message and populate it with the AudioElement data.
  iamf_tools_cli_proto::AudioElementObuMetadata aeMD;
  ae.populateIamfAudioElementMetadata(&aeMD, kId32, minSubstreamId);

  // Validated the populated metadata.
  EXPECT_EQ(kAeType, aeMD.audio_element_type());
  EXPECT_EQ(iamf_tools_cli_proto::LOUDSPEAKER_LAYOUT_STEREO,
            aeMD.scalable_channel_layout_config()
                .channel_audio_layer_configs(0)
                .loudspeaker_layout());
  EXPECT_EQ(kNumSubstreams, aeMD.audio_substream_ids_size());
  EXPECT_EQ(kId32, aeMD.audio_element_id());
}

TEST(test_audio_element, iamf_md_population_sb_ae) {
  const AudioElement::AudioElement_t kAeType =
      AudioElement::AudioElement_t::AUDIO_ELEMENT_SCENE_BASED;
  const Speakers::AudioElementSpeakerLayout kLayout = Speakers::kHOA3;
  const int kNumSubstreams = Speakers::kHOA3.getNumChannels();
  const int kId32 = 42;
  int minSubstreamId = 0;

  // Configure an AudioElement with internal data.
  AudioElement ae;
  ae.setChannelConfig(kLayout);

  // Create a protobuf message and populate it with the AudioElement data.
  iamf_tools_cli_proto::AudioElementObuMetadata aeMD;
  ae.populateIamfAudioElementMetadata(&aeMD, kId32, minSubstreamId);

  // Validated the populated metadata.
  EXPECT_EQ(iamf_tools_cli_proto::AUDIO_ELEMENT_SCENE_BASED,
            aeMD.audio_element_type());
  EXPECT_EQ(kNumSubstreams, aeMD.audio_substream_ids_size());
  EXPECT_EQ(minSubstreamId, kNumSubstreams);
  EXPECT_EQ(kId32, aeMD.audio_element_id());
  EXPECT_EQ(
      kNumSubstreams,
      aeMD.ambisonics_config().ambisonics_mono_config().substream_count());
}