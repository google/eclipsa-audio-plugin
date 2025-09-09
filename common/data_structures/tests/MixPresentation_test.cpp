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

#include "../src/MixPresentation.h"

#include <gtest/gtest.h>
#include <juce_data_structures/juce_data_structures.h>

#include "data_structures/src/MixPresentationLoudness.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

TEST(test_mix_presentation, validity) {
  std::unordered_map<std::string, std::string> tags = {{"artist", "Rick"},
                                                       {"producer", "Rubin"}};

  // Create a mix presentation
  MixPresentation presentation1(juce::Uuid::null(), "TestPresentation", 1,
                                LanguageData::MixLanguages::English, tags);

  juce::Uuid element1 = juce::Uuid();
  juce::Uuid element2 = juce::Uuid();
  presentation1.addAudioElement(element1, 1, "AE1");
  presentation1.addAudioElement(element2, 2, "AE2");
  presentation1.setLanguage(LanguageData::MixLanguages::Finnish);
  presentation1.addTagPair("year", "2008");
  tags.insert({"year", "2008"});

  // Update some of it's values
  presentation1.setName("UpdatedName");
  presentation1.setDefaultMixGain(element1, 3);

  ASSERT_EQ(presentation1.getTags(), tags);

  // Create a second presentation from the tree of the first
  MixPresentation presentation2 =
      MixPresentation::fromTree(presentation1.toValueTree());

  // Ensure that both presentations are equal
  ASSERT_EQ(presentation1, presentation2);
  ASSERT_EQ(presentation2.getTags(), tags);
}

TEST(test_mix_presentation, binaural_mode) {
  // Create a mix presentation
  MixPresentation presentation(juce::Uuid::null(), "BinauralTestPresentation",
                               1.0f);

  // Create audio element UUIDs
  juce::Uuid element1 = juce::Uuid();
  juce::Uuid element2 = juce::Uuid();

  // Add audio elements with different binaural settings
  presentation.addAudioElement(element1, 1.0f, "AE1", true);  // Binaural ON
  presentation.addAudioElement(element2, 1.0f, "AE2",
                               false);  // Binaural OFF

  // Check initial binaural states
  ASSERT_TRUE(presentation.isAudioElementBinaural(element1));
  ASSERT_FALSE(presentation.isAudioElementBinaural(element2));

  // Update binaural state of element1 to false
  presentation.setBinaural(element1, false);
  ASSERT_FALSE(presentation.isAudioElementBinaural(element1));

  // Update binaural state of element2 to true
  presentation.setBinaural(element2, true);
  ASSERT_TRUE(presentation.isAudioElementBinaural(element2));

  // Serialize and deserialize the presentation
  MixPresentation deserializedPresentation =
      MixPresentation::fromTree(presentation.toValueTree());

  // Check binaural states after deserialization
  ASSERT_FALSE(deserializedPresentation.isAudioElementBinaural(element1));
  ASSERT_TRUE(deserializedPresentation.isAudioElementBinaural(element2));
}

TEST(test_mix_presentation, headphone_rendering_mode_iamf) {
  // Create a mix presentation
  MixPresentation presentation(juce::Uuid::null(),
                               "RenderingModeTestPresentation", 1.0f);
  int presentationGain = 3;
  presentation.setGainFromdB(presentationGain);

  // Create a mix presentation loudness object
  MixPresentationLoudness mixPresentationLoudness =
      MixPresentationLoudness(presentation.getId());

  const float integratedLoudness = 5.0f;
  const float digitalPeak = 2.0f;
  const float truePeak = 1.0f;
  std::unordered_map<juce::Uuid, int> audioElementIDMap;
  mixPresentationLoudness.setLayoutIntegratedLoudness(Speakers::kStereo,
                                                      integratedLoudness);
  mixPresentationLoudness.setLayoutDigitalPeak(Speakers::kStereo, digitalPeak);
  mixPresentationLoudness.setLayoutTruePeak(Speakers::kStereo, truePeak);

  // Create audio element UUIDs
  juce::Uuid element1 = juce::Uuid();  // Binaural element
  audioElementIDMap[element1] = 1;
  juce::Uuid element2 = juce::Uuid();  // Stereo element
  audioElementIDMap[element2] = 2;

  // Add audio elements with binaural and stereo settings
  presentation.addAudioElement(element1, 1.0f, "BinauralAE",
                               true);  // Binaural ON
  presentation.addAudioElement(element2, 1.0f, "StereoAE",
                               false);  // Binaural OFF

  // Mock IAMF metadata objects
  iamf_tools_cli_proto::MixPresentationObuMetadata mpMetadata;
  iamf_tools_cli_proto::UserMetadata userMetadata;
  int sampleRate = 48000;
  int duration = 1000;
  uint32_t mixPresentationId = 1;

  // Populate IAMF metadata
  presentation.populateIamfMixPresentationMetadata(
      mixPresentationId, sampleRate, &mpMetadata, userMetadata,
      mixPresentationLoudness, audioElementIDMap);

  // Access the submix and audio elements from the generated metadata
  ASSERT_EQ(mpMetadata.sub_mixes_size(), 1);
  const auto& submix = mpMetadata.sub_mixes(0);
  ASSERT_EQ(submix.audio_elements_size(), 2);

  auto subMixLayout = submix.layouts(0);

  ASSERT_EQ(subMixLayout.has_loudness_layout(), true);
  ASSERT_EQ(subMixLayout.loudness().integrated_loudness(),
            convertToQ7Point8(integratedLoudness));
  ASSERT_EQ(subMixLayout.loudness().digital_peak(),
            convertToQ7Point8(digitalPeak));
  ASSERT_EQ(subMixLayout.loudness().true_peak(), convertToQ7Point8(truePeak));
  ASSERT_EQ(submix.output_mix_gain().default_mix_gain(),
            convertToQ7Point8(presentationGain));

  // Check rendering mode for the first audio element (BinauralAE - isBinaural =
  // true)
  const auto& ae1_metadata = submix.audio_elements(0);
  ASSERT_EQ(ae1_metadata.audio_element_id(), 1);
  ASSERT_TRUE(ae1_metadata.rendering_config().has_headphones_rendering_mode());
  ASSERT_EQ(ae1_metadata.rendering_config().headphones_rendering_mode(),
            iamf_tools_cli_proto::HeadPhonesRenderingMode::
                HEADPHONES_RENDERING_MODE_BINAURAL);

  // Check rendering mode for the second audio element (StereoAE - isBinaural =
  // false)
  const auto& ae2_metadata = submix.audio_elements(1);
  ASSERT_EQ(ae2_metadata.audio_element_id(), 2);
  ASSERT_TRUE(ae2_metadata.rendering_config().has_headphones_rendering_mode());
  ASSERT_EQ(ae2_metadata.rendering_config().headphones_rendering_mode(),
            iamf_tools_cli_proto::HeadPhonesRenderingMode::
                HEADPHONES_RENDERING_MODE_STEREO);
}