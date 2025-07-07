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

#include "../implementation/MixPresentationSoloMuteRepository.h"

#include <data_structures/data_structures.h>
#include <gtest/gtest.h>

#include "data_structures/src/MixPresentationSoloMute.h"

// Just a sanity check -- the test_base_repository suite should cover all
// meaningful repository logic
TEST(test_mix_presentation_solo_mute_repository, update) {
  juce::ValueTree test{"test"};
  MixPresentationSoloMuteRepository repositoryInstance(test);

  juce::Uuid presentationUuid = juce::Uuid();
  MixPresentationSoloMute presentation(presentationUuid, false);
  repositoryInstance.add(presentation);

  juce::Uuid element = juce::Uuid();
  presentation.addAudioElement(element, 1, "AE1");

  const bool element1Soloed = true;
  const bool element1Muted = false;

  presentation.setAudioElementSolo(element, element1Soloed);
  presentation.setAudioElementMute(element, element1Muted);

  repositoryInstance.update(presentation);

  ASSERT_EQ(repositoryInstance.getItemCount(), 1);
  juce::OwnedArray<MixPresentationSoloMute> repos;
  repositoryInstance.getAll(repos);

  ASSERT_EQ(repos[0]->isAudioElementMuted(element), element1Muted);
  ASSERT_EQ(repos[0]->isAudioElementSoloed(element), element1Soloed);
}