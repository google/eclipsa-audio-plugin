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

#include "../src/MixPresentationSoloMute.h"

#include <gtest/gtest.h>
#include <juce_data_structures/juce_data_structures.h>

TEST(test_mix_presentation_solo_mute, validity) {
  // Create a mix presentation
  MixPresentationSoloMute presentation1(juce::Uuid::null(), false);

  juce::Uuid element1 = juce::Uuid();
  juce::Uuid element2 = juce::Uuid();
  presentation1.addAudioElement(element1, 1, "AE1");
  presentation1.addAudioElement(element2, 2, "AE2");

  const bool element1Soloed = true;
  const bool element2Soloed = false;

  const bool element1Muted = false;
  const bool element2Muted = true;

  presentation1.setAudioElementSolo(element1, element1Soloed);
  presentation1.setAudioElementSolo(element2, element2Soloed);

  presentation1.setAudioElementMute(element1, element1Muted);
  presentation1.setAudioElementMute(element2, element2Muted);

  // Create a second presentation from the tree of the first
  MixPresentationSoloMute presentation2 =
      MixPresentationSoloMute::fromTree(presentation1.toValueTree());

  // Ensure that both presentations are equal
  ASSERT_EQ(presentation1, presentation2);

  ASSERT_EQ(presentation2.isAudioElementSoloed(element1), element1Soloed);
  ASSERT_EQ(presentation2.isAudioElementSoloed(element2), element2Soloed);

  ASSERT_EQ(presentation2.isAudioElementMuted(element1), element1Muted);
  ASSERT_EQ(presentation2.isAudioElementMuted(element2), element2Muted);
}