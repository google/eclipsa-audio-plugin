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

#include "../src/screens/EditPresentationScreen.h"

#include <gtest/gtest.h>

#include "../src/RendererProcessor.h"

// Under YouTube, a mix presentation can be added only when none exist.
TEST(EditPresentationScreen, youtube_allows_only_first_mix) {
  EXPECT_TRUE(EditPresentationScreen::canAddMixPresentation(
      ExportProfile::kYouTube, 0));
  for (const int numMixes : {1, 2, 5}) {
    EXPECT_FALSE(EditPresentationScreen::canAddMixPresentation(
        ExportProfile::kYouTube, numMixes));
  }
}

// Under Advanced, mix presentations can be added up to the existing cap of 5.
TEST(EditPresentationScreen, advanced_keeps_cap_of_five) {
  for (const int numMixes : {0, 1, 4}) {
    EXPECT_TRUE(EditPresentationScreen::canAddMixPresentation(
        ExportProfile::kAdvanced, numMixes));
  }
  EXPECT_FALSE(EditPresentationScreen::canAddMixPresentation(
      ExportProfile::kAdvanced, 5));
}
