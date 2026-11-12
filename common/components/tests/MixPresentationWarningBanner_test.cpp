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

// clang-format off
#include <components/components.h>

#include "components/src/MixPresentationWarningBanner.h"
// clang-format on

#include <gtest/gtest.h>

using Banner = MixPresentationWarningBanner;

// Under YouTube - Simple, more than one mix presentation warns.
TEST(MixPresentationWarning, youtube_simple_warns_above_one) {
  constexpr ExportProfile kSimple = ExportProfile::kYouTubeSimple;
  EXPECT_FALSE(Banner::shouldWarn(kSimple, 0));
  EXPECT_FALSE(Banner::shouldWarn(kSimple, 1));
  EXPECT_TRUE(Banner::shouldWarn(kSimple, 2));
  EXPECT_TRUE(Banner::shouldWarn(kSimple, 5));
}

// Under YouTube - Base, more than two mix presentations warn.
TEST(MixPresentationWarning, youtube_base_warns_above_two) {
  constexpr ExportProfile kBase = ExportProfile::kYouTubeBase;
  EXPECT_FALSE(Banner::shouldWarn(kBase, 1));
  EXPECT_FALSE(Banner::shouldWarn(kBase, 2));
  EXPECT_TRUE(Banner::shouldWarn(kBase, 3));
  EXPECT_TRUE(Banner::shouldWarn(kBase, 5));
}

// Under Open the banner never warns, however many mixes exist.
TEST(MixPresentationWarning, open_never_warns) {
  for (const int numMixes : {0, 1, 2, 5}) {
    EXPECT_FALSE(Banner::shouldWarn(ExportProfile::kOpen, numMixes));
  }
}

// The banner text names the selected profile and its limit.
TEST(MixPresentationWarning, message_text) {
  EXPECT_EQ(Banner::messageText(ExportProfile::kYouTubeSimple),
            "YouTube - Simple allows 1 mix presentation. Remove additional "
            "mix presentations before exporting.");
  EXPECT_EQ(Banner::messageText(ExportProfile::kYouTubeBase),
            "YouTube - Base allows 2 mix presentations. Remove additional "
            "mix presentations before exporting.");
}

// A false-to-true transition shows the banner.
TEST(MixPresentationWarningTransition, shows_on_rising_edge) {
  EXPECT_TRUE(Banner::nextVisibility(false, true, false));
}

// A true-to-false transition hides the banner.
TEST(MixPresentationWarningTransition, hides_on_falling_edge) {
  EXPECT_FALSE(Banner::nextVisibility(true, false, true));
  EXPECT_FALSE(Banner::nextVisibility(false, false, false));
}

// After a dismissal the banner stays hidden while the condition holds, and
// shows again once the condition clears and recurs.
TEST(MixPresentationWarningTransition, dismissal_holds_until_recurrence) {
  bool visible = Banner::nextVisibility(false, true, false);
  ASSERT_TRUE(visible);
  visible = false;  // Dismissed.
  visible = Banner::nextVisibility(true, true, visible);
  EXPECT_FALSE(visible);
  visible = Banner::nextVisibility(true, false, visible);
  EXPECT_FALSE(visible);
  visible = Banner::nextVisibility(false, true, visible);
  EXPECT_TRUE(visible);
}

// Switching from Open to YouTube with extra mixes is a rising edge, so a
// banner dismissed earlier shows again.
TEST(MixPresentationWarningTransition, youtube_reselected_shows_again) {
  const bool wasWarning = Banner::shouldWarn(ExportProfile::kOpen, 3);
  const bool isWarning = Banner::shouldWarn(ExportProfile::kYouTubeBase, 3);
  EXPECT_TRUE(Banner::nextVisibility(wasWarning, isWarning, false));
}
