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

#include "components/src/LoudnessWarningBanner.h"
// clang-format on

#include <gtest/gtest.h>

#include <limits>

namespace {
constexpr ExportProfile kYt = ExportProfile::kYouTube;
constexpr ExportProfile kAdv = ExportProfile::kAdvanced;
}  // namespace

// Under YouTube, only a displayed value above -14.0 warns.
TEST(LoudnessWarning, youtube_threshold) {
  EXPECT_TRUE(LoudnessWarningBanner::shouldWarn(kYt, -13.9f, false));
  EXPECT_FALSE(LoudnessWarningBanner::shouldWarn(kYt, -14.0f, false));
  EXPECT_FALSE(LoudnessWarningBanner::shouldWarn(kYt, -14.1f, false));
  // -14.04 displays as -14.0, so it does not warn.
  EXPECT_FALSE(LoudnessWarningBanner::shouldWarn(kYt, -14.04f, false));
}

// NaN, infinity and a reset measurement never warn.
TEST(LoudnessWarning, youtube_invalid_or_reset) {
  EXPECT_FALSE(LoudnessWarningBanner::shouldWarn(
      kYt, std::numeric_limits<float>::quiet_NaN(), false));
  EXPECT_FALSE(LoudnessWarningBanner::shouldWarn(
      kYt, std::numeric_limits<float>::infinity(), false));
  EXPECT_FALSE(LoudnessWarningBanner::shouldWarn(kYt, -10.0f, true));
}

// Under Advanced the banner never warns, however loud the audio is.
TEST(LoudnessWarning, advanced_never_warns) {
  for (const float value : {-13.9f, -10.0f, 0.0f, -14.0f, -30.0f}) {
    EXPECT_FALSE(LoudnessWarningBanner::shouldWarn(kAdv, value, false));
    EXPECT_FALSE(LoudnessWarningBanner::shouldWarn(kAdv, value, true));
  }
}

// The banner text matches the agreed wording.
TEST(LoudnessWarning, message_text) {
  EXPECT_EQ(LoudnessWarningBanner::messageText(),
            "Audio louder than -14 LUFS will be normalized by YouTube. To "
            "ensure audio sounds correct, reduce integrated loudness to below "
            "-14 LUFS.");
}

// A false-to-true transition shows the banner.
TEST(LoudnessWarningTransition, shows_on_rising_edge) {
  EXPECT_TRUE(LoudnessWarningBanner::nextVisibility(false, true, false));
}

// A true-to-false transition hides the banner.
TEST(LoudnessWarningTransition, hides_on_falling_edge) {
  EXPECT_FALSE(LoudnessWarningBanner::nextVisibility(true, false, true));
  EXPECT_FALSE(LoudnessWarningBanner::nextVisibility(false, false, false));
}

// After a dismissal the banner stays hidden while the condition holds, and
// shows again once the condition clears and recurs.
TEST(LoudnessWarningTransition, dismissal_holds_until_recurrence) {
  bool visible = LoudnessWarningBanner::nextVisibility(false, true, false);
  ASSERT_TRUE(visible);
  visible = false;  // Dismissed.
  visible = LoudnessWarningBanner::nextVisibility(true, true, visible);
  EXPECT_FALSE(visible);
  visible = LoudnessWarningBanner::nextVisibility(true, false, visible);
  EXPECT_FALSE(visible);
  visible = LoudnessWarningBanner::nextVisibility(false, true, visible);
  EXPECT_TRUE(visible);
}

// While visible and still warning, the banner stays visible.
TEST(LoudnessWarningTransition, stays_visible_while_warning) {
  EXPECT_TRUE(LoudnessWarningBanner::nextVisibility(true, true, true));
}
